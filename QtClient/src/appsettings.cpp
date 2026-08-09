#include "appsettings.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

namespace {

/**
 * INI y no el formato nativo: en macOS el nativo es un `.plist` binario que ademas queda
 * cacheado por `cfprefsd`, asi que editarlo a mano no funciona y verlo tampoco. La
 * convencion LGA para settings no sensitivos es un INI que el usuario pueda abrir.
 */
QSettings& store()
{
    static QSettings settings(
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
            .filePath(QStringLiteral("settings.ini")),
        QSettings::IniFormat);
    return settings;
}

constexpr const char* kLanguageKey = "ui/language";

} // namespace

namespace AppSettings {

QString language()
{
    const QString code = store().value(QLatin1String(kLanguageKey)).toString().toLower();
    return code == QLatin1String("es") ? QStringLiteral("es") : QStringLiteral("en");
}

void setLanguage(const QString& code)
{
    store().setValue(QLatin1String(kLanguageKey),
                     code.toLower() == QLatin1String("es") ? QStringLiteral("es")
                                                           : QStringLiteral("en"));
    store().sync();
}

} // namespace AppSettings
