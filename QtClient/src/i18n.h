#pragma once

#include <QString>

/**
 * Capa bilingue de la ventana de config.
 *
 * Por que NO es `tr()` + .ts
 * --------------------------
 * El mecanismo de Qt elige el idioma por el locale del sistema y necesita compilar `.qm` y
 * cargarlos al arrancar. Aca el idioma lo elige el USUARIO con un switch al pie de la
 * ventana y cambia en caliente, que es justo lo que `tr()` no hace: los strings ya
 * traducidos estan dentro de los widgets y hay que volver a escribirlos igual. Con una tabla
 * propia el switch es una linea y no hay artefactos de build que mantener sincronizados.
 *
 * Como se agrega un string
 * ------------------------
 * 1. Una entrada nueva en `Str`.
 * 2. Su fila en la tabla de `i18n.cpp`, en EL MISMO ORDEN (hay un static_assert que lo
 *    verifica: la tabla se indexa por el enum, no se busca por clave).
 *
 * El idioma activo se persiste en AppSettings; el default es ingles.
 */
namespace I18n {

enum class Lang { En, Es };

/// Todo string visible que cambia con el idioma. El orden ES el indice de la tabla.
enum class Str {
    // ── descripciones de los tres bloques ────────────────────────────────────
    DescFileAssociation,
    DescNukeVersion,
    DescNukeBridge,

    // ── estado del bridge ────────────────────────────────────────────────────
    ChipNotInstalled,
    ChipInstalled,          ///< %1 = version instalada
    ChipNeedsUpdate,        ///< %1 = version instalada
    ChipInstalledUnknown,   ///< instalado pero sin poder leer que version

    // ── botones ──────────────────────────────────────────────────────────────
    BtnApply,
    BtnBrowse,
    BtnSave,
    BtnInstall,
    BtnReinstall,
    BtnExport,
    BtnCopyLine,
    BtnCopied,

    // ── bridge: hints y panel manual ─────────────────────────────────────────
    HintFolderFound,        ///< %1 = carpeta detectada
    HintFolderNotFound,
    LinkManualShow,
    LinkManualHide,
    ManualStep1,
    ManualStep2,
    ManualStep3,

    // ── scanner de versiones ─────────────────────────────────────────────────
    ScanChoose,
    ScanScanning,
    ScanScanningPath,       ///< %1 = path acortado
    ScanFound,              ///< %1 = cantidad
    ScanNone,

    // ── placeholders y titulos de file dialogs ───────────────────────────────
    PlaceholderNukePath,
    PlaceholderNukeDir,
    PickNukeExecutable,
    PickNukeFolder,
    PickExportFolder,

    // ── carteles ─────────────────────────────────────────────────────────────
    TitleError,
    TitleWarning,
    MsgPathEmpty,
    MsgPathMissing,
    MsgNotNukeExecutable,
    TitleSaved,
    MsgSaved,               ///< %1 = path coloreado

    TitleBridgeInstalled,
    MsgBridgeInstalled,     ///< %1 = path coloreado
    TitleBridgeFailed,
    TitleBridgeExported,
    MsgBridgeExported,      ///< %1 = path coloreado
    MsgBridgeSourceIsTarget,
    MsgBridgeDirMissing,
    MsgBridgePayloadMissing,
    MsgBridgeWriteFailed,

    TitleAssocDone,
    MsgAssocDone,
    TitleAssocAlmost,
    MsgAssocAlmost,
    TitleAssocWarnings,

    Count_,
};

/// Idioma activo. Al arrancar sale de AppSettings.
Lang lang();

/// Cambia el idioma y lo persiste. No repinta nada: eso lo hace quien llama.
void setLang(Lang lang);

/// El string en el idioma activo.
QString t(Str id);

} // namespace I18n

/// Azucar: `TR(DescNukeBridge)` en vez de `I18n::t(I18n::Str::DescNukeBridge)`.
#define TR(id) I18n::t(I18n::Str::id)
