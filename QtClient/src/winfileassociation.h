#pragma once

#include <QtGlobal>

#ifdef Q_OS_WIN

#include <QString>
#include <QStringList>

struct HWND__;
using HWND = HWND__ *;

namespace WinFileAssociation {

enum class ApplyResult {
    Success,
    NeedsUserConfirmation,
    Failed,
};

struct ApplyOutcome {
    ApplyResult result = ApplyResult::Failed;
    QStringList errors;
};

/// Registra el ProgID, las Capabilities y escribe la asociacion de `.nk`.
/// En Windows 11 con UserChoiceLatest intenta primero el hash silencioso y, si no alcanza,
/// abre el selector nativo de Windows o la pagina de Apps predeterminadas.
ApplyOutcome apply(HWND parentHwnd = nullptr);

/// Lee el ProgId actual de `.nk` desde UserChoiceLatest o UserChoice.
QString currentNkProgId();

/// True si el ProgId actual de `.nk` es el de OpenInNukeX.
bool isNkAssociatedWithUs();

} // namespace WinFileAssociation

#endif // Q_OS_WIN
