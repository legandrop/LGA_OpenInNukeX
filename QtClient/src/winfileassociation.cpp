#include "winfileassociation.h"

#include <QtGlobal>

#if defined(Q_OS_WIN)

#include "logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QThread>
#include <objbase.h>
#include <sddl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wincrypt.h>

#include <array>
#include <cstring>
#include <memory>
#include <vector>

namespace {

constexpr wchar_t kProgId[] = L"LGA.NukeScript.1";
constexpr wchar_t kExtension[] = L".nk";
constexpr wchar_t kAppKey[] = L"OpenInNukeX";
constexpr wchar_t kCapabilitiesPath[] = L"Software\\OpenInNukeX\\Capabilities";
constexpr wchar_t kRegisteredAppsPath[] = L"Software\\RegisteredApplications";

constexpr wchar_t kUserExperience[] =
    L"User Choice set via Windows User Experience "
    L"{D18B6DD5-6124-4341-9318-804003BAFA0B}";

// Chromium: IOpenWithLauncher para abrir el selector nativo de app por defecto.
// El IID se pasa a mano en launchOpenWithPicker(); __declspec(uuid) solo aplica en MSVC
// y MinGW lo ignora con warning.
struct IOpenWithLauncher : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Launch(HWND hWndParent, const wchar_t *lpszPath,
                                             int flags) = 0;
};

// ─── Registry helpers ────────────────────────────────────────────────────────

std::wstring toW(const QString &value)
{
    return value.toStdWString();
}

bool writeString(HKEY root, const QString &subKey, const QString &valueName, const QString &data)
{
    const std::wstring sub = toW(subKey);
    HKEY key = nullptr;
    const LONG rc = RegCreateKeyExW(root, sub.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                                  KEY_WRITE, nullptr, &key, nullptr);
    if (rc != ERROR_SUCCESS) {
        Logger::logError(QString("WinAssoc: no se pudo abrir/crear %1 (rc=%2)")
                             .arg(subKey)
                             .arg(rc));
        return false;
    }

    const std::wstring name = toW(valueName);
    const std::wstring value = toW(data);
    const DWORD bytes = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    const LONG setRc = RegSetValueExW(key, valueName.isEmpty() ? nullptr : name.c_str(), 0,
                                      REG_SZ, reinterpret_cast<const BYTE *>(value.c_str()),
                                      bytes);
    RegCloseKey(key);
    if (setRc != ERROR_SUCCESS) {
        Logger::logError(QString("WinAssoc: no se pudo escribir %1\\%2 (rc=%3)")
                             .arg(subKey, valueName)
                             .arg(setRc));
        return false;
    }
    return true;
}

QString readString(HKEY root, const QString &subKey, const QString &valueName)
{
    const std::wstring sub = toW(subKey);
    const std::wstring name = toW(valueName);

    DWORD type = 0;
    DWORD size = 0;
    LONG rc = RegGetValueW(root, sub.c_str(), valueName.isEmpty() ? nullptr : name.c_str(),
                           RRF_RT_REG_SZ, &type, nullptr, &size);
    if (rc != ERROR_SUCCESS || size == 0)
        return {};

    std::vector<wchar_t> buffer(size / sizeof(wchar_t), L'\0');
    rc = RegGetValueW(root, sub.c_str(), valueName.isEmpty() ? nullptr : name.c_str(),
                      RRF_RT_REG_SZ, &type, buffer.data(), &size);
    if (rc != ERROR_SUCCESS)
        return {};

    while (!buffer.empty() && buffer.back() == L'\0')
        buffer.pop_back();
    return QString::fromStdWString(buffer.data());
}

bool deleteTree(HKEY root, const QString &subKey)
{
    const std::wstring sub = toW(subKey);
    const LONG rc = RegDeleteTreeW(root, sub.c_str());
    return rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND;
}

void notifyAssociationChanged()
{
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

// ─── SID del usuario actual ──────────────────────────────────────────────────

QString currentUserSid()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return {};

  std::unique_ptr<void, decltype(&CloseHandle)> tokenGuard(token, &CloseHandle);

    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    if (size == 0)
        return {};

    std::vector<BYTE> buffer(size);
    if (!GetTokenInformation(token, TokenUser, buffer.data(), size, &size))
        return {};

    const TOKEN_USER *user = reinterpret_cast<const TOKEN_USER *>(buffer.data());
    LPWSTR sidString = nullptr;
    if (!ConvertSidToStringSidW(user->User.Sid, &sidString))
        return {};

    const QString sid = QString::fromWCharArray(sidString);
    LocalFree(sidString);
    return sid;
}

bool isUserChoiceLatestActive()
{
    const QString sid = currentUserSid();
    if (sid.isEmpty())
        return false;

    const QString path = QStringLiteral(
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SystemProtectedUserData\\%1\\AnyoneRead\\"
        "AppDefaults")
                           .arg(sid);
    const QString value = readString(HKEY_LOCAL_MACHINE, path, QStringLiteral("HashVersion"));
    return value == QStringLiteral("1");
}

// ─── Hash UserChoice (port MIT de PS-SFTA / DanysysTeam) ─────────────────────

int shiftRightSigned(int value, int count)
{
    if ((value & 0x80000000) != 0)
        return static_cast<int>(static_cast<unsigned>(value) >> count) ^
               static_cast<int>(0xFFFF0000u);
    return value >> count;
}

int readInt32LE(const BYTE *bytes, int index)
{
    return static_cast<int>(bytes[index]) | (static_cast<int>(bytes[index + 1]) << 8) |
           (static_cast<int>(bytes[index + 2]) << 16) | (static_cast<int>(bytes[index + 3]) << 24);
}

int readInt32LE(const std::vector<BYTE> &bytes, int index)
{
    if (index + 4 > static_cast<int>(bytes.size()))
        return 0;
    return readInt32LE(bytes.data(), index);
}

QString computeUserChoiceHash(const std::vector<BYTE> &bytesBaseInfo, int lengthBase)
{
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        return {};

    if (!CryptCreateHash(provider, CALG_MD5, 0, 0, &hash)) {
        CryptReleaseContext(provider, 0);
        return {};
    }

    if (!CryptHashData(hash, bytesBaseInfo.data(), static_cast<DWORD>(bytesBaseInfo.size()), 0)) {
        CryptDestroyHash(hash);
        CryptReleaseContext(provider, 0);
        return {};
    }

    BYTE md5[16] = {};
    DWORD md5Len = sizeof(md5);
    if (!CryptGetHashParam(hash, HP_HASHVAL, md5, &md5Len, 0)) {
        CryptDestroyHash(hash);
        CryptReleaseContext(provider, 0);
        return {};
    }

    CryptDestroyHash(hash);
    CryptReleaseContext(provider, 0);

    const bool condition = ((lengthBase & 4) <= 1);
    const int length = (condition ? 1 : 0) + (lengthBase >> 2) - 1;
    if (length <= 1)
        return {};

    int pdata = 0;
    int cache = 0;
    int counter = 0;
    int outHash1 = 0;
    int outHash2 = 0;
    int h1 = (readInt32LE(md5, 0) | 1) + 0x69FB0000;
    int h2 = (readInt32LE(md5, 4) | 1) + 0x13DB0000;
    counter = shiftRightSigned(length - 2, 1) + 1;

    while (counter > 0) {
        const int m0 = readInt32LE(bytesBaseInfo, pdata) + outHash1;
        const int m1 = readInt32LE(bytesBaseInfo, pdata + 4);
        pdata += 8;
        const int m3 = (h1 * m0) - (0x10FA9605 * shiftRightSigned(m0, 16));
        const int m4 = (0x79F8A395 * m3) + (0x689B6B9F * shiftRightSigned(m3, 16));
        const int m5 = (0xEA970001 * m4) - (0x3C101569 * shiftRightSigned(m4, 16));
        const int m6 = m5 + m1;
        const int m10 = (h2 * m6) - (0x3CE8EC25 * shiftRightSigned(m6, 16));
        const int m11 = (0x59C3AF2D * m10) - (0x2232E0F1 * shiftRightSigned(m10, 16));
        outHash1 = (0x1EC90001 * m11) + (0x35BD1EC9 * shiftRightSigned(m11, 16));
        outHash2 = outHash1 + cache + m5;
        cache = outHash2;
        --counter;
    }

    BYTE outHash[16] = {};
    std::memcpy(outHash, &outHash1, sizeof(int));
    std::memcpy(outHash + 4, &outHash2, sizeof(int));

    pdata = 0;
    cache = 0;
    outHash1 = 0;
    h1 = readInt32LE(md5, 0) | 1;
    h2 = readInt32LE(md5, 4) | 1;
    counter = shiftRightSigned(length - 2, 1) + 1;

    while (counter > 0) {
        const int m0 = readInt32LE(bytesBaseInfo, pdata) + outHash1;
        pdata += 8;
        const int m1x = m0 * h1;
        const int m2 = (0xB1110000 * m1x) - (0x30674EEF * shiftRightSigned(m1x, 16));
        const int m3x = (0x5B9F0000 * m2) - (0x78F7A461 * shiftRightSigned(m2, 16));
        const int m4x = (0x12CEB96D * shiftRightSigned(m3x, 16)) - (0x46930000 * m3x);
        const int m5x = (0x1D830000 * m4x) + (0x257E1D83 * shiftRightSigned(m4x, 16));
        const int m6 = h2 * (m5x + readInt32LE(bytesBaseInfo, pdata - 4));
        const int m7 = (0x16F50000 * m6) - (0x5D8BE90B * shiftRightSigned(m6, 16));
        const int m8 = (0x96FF0000 * m7) - (0x2C7C6901 * shiftRightSigned(m7, 16));
        const int m9 = (0x2B890000 * m8) + (0x7C932B89 * shiftRightSigned(m8, 16));
        outHash1 = (0x9F690000 * m9) - (0x405B6097 * shiftRightSigned(m9, 16));
        outHash2 = outHash1 + cache + m5x;
        cache = outHash2;
        --counter;
    }

    std::memcpy(outHash + 8, &outHash1, sizeof(int));
    std::memcpy(outHash + 12, &outHash2, sizeof(int));

    const int hashValue1 = readInt32LE(outHash, 8) ^ readInt32LE(outHash, 0);
    const int hashValue2 = readInt32LE(outHash, 12) ^ readInt32LE(outHash, 4);

    BYTE outHashBase[8] = {};
    std::memcpy(outHashBase, &hashValue1, sizeof(int));
    std::memcpy(outHashBase + 4, &hashValue2, sizeof(int));

    DWORD encodedLen = 0;
    if (!CryptBinaryToStringW(outHashBase, sizeof(outHashBase),
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &encodedLen))
        return {};

    std::vector<wchar_t> encoded(encodedLen, L'\0');
    if (!CryptBinaryToStringW(outHashBase, sizeof(outHashBase),
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, encoded.data(), &encodedLen))
        return {};

    return QString::fromWCharArray(encoded.data()).trimmed();
}

QString hexDateTimeForCurrentMinute()
{
    SYSTEMTIME now{};
    GetLocalTime(&now);
    SYSTEMTIME rounded = now;
    rounded.wSecond = 0;
    rounded.wMilliseconds = 0;

    FILETIME fileTime{};
    SystemTimeToFileTime(&rounded, &fileTime);
    const ULONGLONG value =
        (static_cast<ULONGLONG>(fileTime.dwHighDateTime) << 32) | fileTime.dwLowDateTime;
    return QStringLiteral("%1%2")
        .arg(static_cast<quint32>(value >> 32), 8, 16, QChar('0'))
        .arg(static_cast<quint32>(value & 0xFFFFFFFFu), 8, 16, QChar('0'));
}

QString buildUserChoiceBaseInfo(const QString &extension, const QString &progId, const QString &sid)
{
    const QString base = extension + sid + progId + hexDateTimeForCurrentMinute() +
                         QString::fromWCharArray(kUserExperience) + QChar('\0');
    return base.toLower();
}

std::vector<BYTE> utf16BytesWithTerminator(const QString &text)
{
    const std::wstring wide = text.toStdWString();
    std::vector<BYTE> bytes((wide.size() + 1) * sizeof(wchar_t));
    std::memcpy(bytes.data(), wide.data(), wide.size() * sizeof(wchar_t));
    bytes[wide.size() * sizeof(wchar_t)] = 0;
    bytes[wide.size() * sizeof(wchar_t) + 1] = 0;
    return bytes;
}

bool writeLegacyUserChoice(const QString &extension, const QString &progId)
{
    const QString sid = currentUserSid();
    if (sid.isEmpty())
        return false;

    const QString baseInfo = buildUserChoiceBaseInfo(extension, progId, sid);
    const auto bytes = utf16BytesWithTerminator(baseInfo);
    const QString hash = computeUserChoiceHash(bytes, static_cast<int>(baseInfo.size()) * 2 + 2);
    if (hash.isEmpty()) {
        Logger::logError("WinAssoc: no se pudo calcular el hash UserChoice");
        return false;
    }

    const QString choicePath = QStringLiteral(
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%1\\UserChoice")
                                   .arg(extension);
    deleteTree(HKEY_CURRENT_USER, choicePath);
    bool ok = true;
    ok &= writeString(HKEY_CURRENT_USER, choicePath, QStringLiteral("Hash"), hash);
    ok &= writeString(HKEY_CURRENT_USER, choicePath, QStringLiteral("ProgId"), progId);
  Logger::logInfo(QString("WinAssoc: UserChoice escrito para %1 -> %2").arg(extension, progId));
    return ok;
}

// ─── Registro de la app ──────────────────────────────────────────────────────

bool cleanConflictingKeys()
{
    const QStringList keys = {
        QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.nk\\"
                       "UserChoice"),
        QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.nk\\"
                       "UserChoiceLatest"),
        QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.nk"),
        QStringLiteral("Software\\Classes\\.nk"),
        QStringLiteral("Software\\Classes\\NukeScript"),
        QStringLiteral("Software\\Classes\\nuke_auto_file"),
        QStringLiteral("Software\\Classes\\Foundry.Nuke.Script"),
        QStringLiteral("Software\\Classes\\LGA.NukeScript"),
        QStringLiteral("Software\\Classes\\LGA.NukeScript.1"),
        QStringLiteral("Software\\Classes\\Applications\\LGA_OpenInNukeX.exe"),
    };

    bool ok = true;
    for (const QString &key : keys)
        ok &= deleteTree(HKEY_CURRENT_USER, key);
    return ok;
}

bool registerProgId()
{
    const QString exePath =
        QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString iconPath =
        QDir::toNativeSeparators(QDir(QCoreApplication::applicationDirPath()).filePath("app_icon.ico"));
    const QString progId = QString::fromWCharArray(kProgId);

    bool ok = true;
    ok &= writeString(HKEY_CURRENT_USER, QStringLiteral("Software\\Classes\\%1").arg(progId),
                      QString(), QStringLiteral("Nuke Script File"));
    ok &= writeString(HKEY_CURRENT_USER,
                      QStringLiteral("Software\\Classes\\%1\\shell\\open\\command").arg(progId),
                      QString(),
                      QStringLiteral("\"%1\" \"%2\"").arg(exePath, QStringLiteral("%1")));
    if (QFile::exists(iconPath)) {
        ok &= writeString(HKEY_CURRENT_USER,
                          QStringLiteral("Software\\Classes\\%1\\DefaultIcon").arg(progId),
                          QString(), QStringLiteral("\"%1\",0").arg(iconPath));
    }
    return ok;
}

bool registerExtensionClass(const QString &extension, const QString &progId)
{
    return writeString(HKEY_CURRENT_USER,
                      QStringLiteral("Software\\Classes\\%1").arg(extension),
                      QString(),
                      progId);
}

bool invokeSetFtaHelper(const QString &extension, const QString &progId)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString helperPath = QDir(appDir).filePath(QStringLiteral("LGA_WinSetFTA.exe"));
    if (!QFile::exists(helperPath)) {
        Logger::logError(QString("WinAssoc: no se encontro LGA_WinSetFTA.exe en %1").arg(appDir));
        return false;
    }

    QProcess process;
    process.setProgram(helperPath);
    process.setArguments({extension, progId});
    process.setWorkingDirectory(appDir);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start();
    if (!process.waitForStarted(5000)) {
        Logger::logError("WinAssoc: no se pudo iniciar LGA_WinSetFTA");
        return false;
    }
    if (!process.waitForFinished(60000)) {
        process.kill();
        Logger::logError("WinAssoc: LGA_WinSetFTA expiro");
        return false;
    }
    if (process.exitCode() != 0) {
        Logger::logError(QString("WinAssoc: LGA_WinSetFTA fallo (%1): %2")
                             .arg(process.exitCode())
                             .arg(QString::fromUtf8(process.readAllStandardOutput())));
        return false;
    }
    Logger::logInfo(QString("WinAssoc: LGA_WinSetFTA OK para %1 -> %2").arg(extension, progId));
    return true;
}

bool registerDefaultAppCapabilities()
{
    const QString exePath =
        QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString iconRef = QStringLiteral("\"%1\",0").arg(exePath);
    const QString progId = QString::fromWCharArray(kProgId);
    const QString appKey = QString::fromWCharArray(kAppKey);
    const QString capsPath = QString::fromWCharArray(kCapabilitiesPath);

    bool ok = true;
    ok &= writeString(HKEY_CURRENT_USER, capsPath, QStringLiteral("ApplicationName"),
                      QStringLiteral("LGA OpenInNukeX"));
    ok &= writeString(HKEY_CURRENT_USER, capsPath, QStringLiteral("ApplicationDescription"),
                      QStringLiteral("Opens Nuke scripts (.nk) with your preferred NukeX."));
    ok &= writeString(HKEY_CURRENT_USER, capsPath, QStringLiteral("ApplicationIcon"), iconRef);
    ok &= writeString(HKEY_CURRENT_USER, capsPath + QStringLiteral("\\FileAssociations"),
                      QString::fromWCharArray(kExtension), progId);
    ok &= writeString(HKEY_CURRENT_USER, QString::fromWCharArray(kRegisteredAppsPath), appKey,
                      capsPath);
    return ok;
}

// ─── Fallbacks de UI del sistema ───────────────────────────────────────────

bool launchOpenWithPicker(HWND parentHwnd)
{
    const HRESULT comRc = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool comOwned = SUCCEEDED(comRc);

    bool launched = false;
    CLSID clsid{};
    const IID kIID_IOpenWithLauncher = {
        0x6a283fe2, 0xecfa, 0x4599, {0x91, 0xc4, 0xe8, 0x09, 0x57, 0x13, 0x7b, 0x26}};
    if (SUCCEEDED(CLSIDFromString(L"{e44e9428-bdbc-4987-a099-40dc8fd255e7}", &clsid))) {
        IOpenWithLauncher *launcher = nullptr;
        if (SUCCEEDED(CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER, kIID_IOpenWithLauncher,
                                       reinterpret_cast<void **>(&launcher)))) {
            CoAllowSetForegroundWindow(launcher, nullptr);
            const HRESULT hr = launcher->Launch(parentHwnd, kExtension, 0x2004);
            launched = SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_CANCELLED);
            launcher->Release();
            Logger::logInfo(QString("WinAssoc: IOpenWithLauncher hr=0x%1")
                                .arg(static_cast<quint32>(hr), 8, 16, QChar('0')));
        }
    }

    if (comOwned)
        CoUninitialize();
    return launched;
}

bool openDefaultAppsSettings()
{
    const QString deepLink =
        QStringLiteral("ms-settings:defaultapps?registeredAppUser=%1")
            .arg(QString::fromWCharArray(kAppKey));
    const std::wstring url = deepLink.toStdWString();
    const HINSTANCE rc =
        ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<intptr_t>(rc) > 32)
        return true;

    Logger::logInfo("WinAssoc: deep link fallo, abriendo pagina generica de Apps predeterminadas");
    const HINSTANCE fallback =
        ShellExecuteW(nullptr, L"open", L"ms-settings:defaultapps", nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(fallback) > 32;
}

QString readProgIdFromKey(const QString &subKey)
{
    return readString(HKEY_CURRENT_USER, subKey, QStringLiteral("ProgId"));
}

} // namespace

namespace WinFileAssociation {

QString currentNkProgId()
{
    const QString latestPath = QStringLiteral(
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.nk\\UserChoiceLatest\\"
        "ProgId");
    const QString latestProgId = readProgIdFromKey(latestPath);

    if (isUserChoiceLatestActive())
        return latestProgId;

    if (!latestProgId.isEmpty())
        return latestProgId;

    const QString legacyPath = QStringLiteral(
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.nk\\UserChoice");
    return readProgIdFromKey(legacyPath);
}

bool isNkAssociatedWithUs()
{
    return currentNkProgId().compare(QString::fromWCharArray(kProgId), Qt::CaseInsensitive) == 0;
}

ApplyOutcome apply(HWND parentHwnd)
{
    ApplyOutcome outcome;
    Logger::logInfo("WinAssoc: iniciando asociacion nativa de .nk");

    if (!cleanConflictingKeys()) {
        outcome.errors << QStringLiteral("Error al limpiar el registro");
        Logger::logError("WinAssoc: limpieza incompleta");
    }

    QThread::msleep(500);

    if (!registerProgId()) {
        outcome.errors << QStringLiteral("Error al registrar ProgID");
        Logger::logError("WinAssoc: registro de ProgID fallo");
    }

    if (!registerDefaultAppCapabilities()) {
        outcome.errors << QStringLiteral("Error al registrar la app en Apps predeterminadas");
        Logger::logError("WinAssoc: Capabilities/RegisteredApplications fallo");
    }

    const QString progId = QString::fromWCharArray(kProgId);
    const QString extension = QString::fromWCharArray(kExtension);
    const bool latestActive = isUserChoiceLatestActive();
    Logger::logInfo(QString("WinAssoc: UserChoiceLatest %1")
                        .arg(latestActive ? QStringLiteral("activo") : QStringLiteral("inactivo")));

    if (!registerExtensionClass(extension, progId)) {
        outcome.errors << QStringLiteral("Error al registrar la extension");
        Logger::logError("WinAssoc: registro de extension fallo");
    }

    bool associationWritten = false;
    if (invokeSetFtaHelper(extension, progId)) {
        associationWritten = true;
    } else if (!latestActive) {
        if (writeLegacyUserChoice(extension, progId)) {
            associationWritten = true;
        } else {
            outcome.errors << QStringLiteral("Error al escribir UserChoice");
        }
    } else {
        outcome.errors << QStringLiteral(
            "No se pudo escribir UserChoiceLatest (falta LGA_WinSetFTA.exe)");
        Logger::logError("WinAssoc: UserChoiceLatest activo pero falta el helper");
    }

    notifyAssociationChanged();

    if (associationWritten && isNkAssociatedWithUs()) {
        outcome.result = ApplyResult::Success;
        Logger::logInfo("WinAssoc: asociacion verificada");
        return outcome;
    }

    Logger::logInfo("WinAssoc: hash silencioso no alcanzo; pidiendo confirmacion al sistema");
    const bool pickerLaunched = launchOpenWithPicker(parentHwnd);
    QThread::msleep(800);
    notifyAssociationChanged();

    if (isNkAssociatedWithUs()) {
        outcome.result = ApplyResult::Success;
        Logger::logInfo("WinAssoc: asociacion confirmada desde el selector del sistema");
        return outcome;
    }

    if (!openDefaultAppsSettings())
        outcome.errors << QStringLiteral("No se pudo abrir Apps predeterminadas de Windows");

    if (!outcome.errors.isEmpty() && !pickerLaunched)
        outcome.result = ApplyResult::Failed;
    else
        outcome.result = ApplyResult::NeedsUserConfirmation;

    return outcome;
}

} // namespace WinFileAssociation

#endif // defined(Q_OS_WIN)
