#pragma once

#include <QColor>

#include <array>
#include <QLatin1String>
#include <QString>

/**
 * Punto UNICO de ajuste del look de los dialogos de eleccion. TODO lo visual sale de
 * aca: colores de texto, fondos, bordes, esquinas, espaciados, glow y anchos. Ningun
 * .cpp de dialogos define un color ni una medida propia.
 *
 * Los colores estan en C++ y no en un .qss a proposito: cuando el componente se comparte
 * entre apps (en FileManagerS3 vive en la lib que consume PipeSync), cada repo tiene su
 * propio dark_theme.qss y su propio .qrc, asi que habria que duplicarlos — justo el "un
 * solo lugar" que se quiere conservar.
 *
 * Los valores por defecto reproducen el look que los dialogos ya tenian heredado del
 * tema oscuro de la app; al fijarlos aca dejan de depender de el.
 */
namespace DialogStyle {

// ---------------------------------------------------------------- colores del dialogo
inline constexpr const char* kDialogBackground = "#1E1E1E";
// El cuerpo del mensaje va en gris: asi el blanco queda libre para DESTACAR las pocas
// palabras que importan (cuantos archivos, donde, que no se puede deshacer). Con todo el
// texto en blanco no se puede jerarquizar nada.
inline constexpr const char* kMessageTextColor = "#A9A9AE";
inline constexpr const char* kEmphasisTextColor = "#F2F2F4";

// Colores del par from/to. La parte COMUN de los dos paths —incluido el esquema
// ("s3://")— va en lavanda: es el mismo color en los dos lados porque es lo mismo.
inline constexpr const char* kPathCommonColor = "#c56cf0";
inline constexpr const char* kPathSeparatorColor = "#6A6A6E";

// De la divergencia en adelante se recorre la paleta del Activity Tab en su orden
// (el FilePathColorDelegate de FileManagerS3), IGUAL en los dos lados: el color marca el NIVEL,
// no de que lado del par esta cada directorio. Lo que separa origen de destino es donde se
// corta el color comun.
inline constexpr std::array<const char*, 12> kPathPalette = {
    "#ffff66", // amarillo
    "#28b5b5", // verde cian
    "#ff9a8a", // naranja pastel
    "#0088ff", // azul
    "#ffd369", // amarillo mostaza
    "#28b5b5", // verde cian
    "#ff9a8a", // naranja pastel
    "#6bc9ff", // celeste
    "#ffd369", // amarillo mostaza
    "#28b5b5", // verde cian
    "#ff9a8a", // naranja pastel
    "#6bc9ff", // celeste
};

// ---------------------------------------------- bloque de detalle ("Show Details")
// Detalle tecnico colapsable: traceback, salida de un proceso, estado previo/actual. Va
// mas oscuro que el dialogo a proposito, para que se lea como un bloque de datos pegado y
// no como una segunda parte del mensaje.
inline constexpr const char* kDetailBackground = "#141414";
inline constexpr const char* kDetailTextColor = "#A9A9AE";
inline constexpr const char* kDetailBorderColor = "#3A3A3A";
inline constexpr int kDetailBorderRadius = 4;
inline constexpr int kDetailPadding = 8;
// Alto del bloque desplegado. El maximo existe para que un traceback largo no empuje el
// dialogo fuera de la pantalla: de ahi en adelante scrollea.
inline constexpr int kDetailMinimumHeight = 90;
inline constexpr int kDetailMaximumHeight = 200;

// ------------------------------------------------------------------ colores del boton
// 🟣 Marca del boton que ejecuta Enter: borde violeta claro de la app + glow suave.
inline constexpr const char* kMarkedBorderColor = "#3B316A";
inline const QColor kGlowColor = QColor(QStringLiteral("#4C3078"));
inline constexpr int kGlowBlurRadius = 16;

// Borde del boton SIN marcar. Los dos estados dibujan la misma caja y solo cambia el
// color del borde: en cuanto el stylesheet define `border`, QStyleSheetStyle deja de
// dibujar el bezel NATIVO y pinta su propia caja con fondo transparente. Si el marcado
// define borde y el otro no, el marcado queda con el fondo del dialogo y el otro con el
// gris del bezel — o sea, la marca terminaba cambiando el color de fondo sin querer.
inline constexpr const char* kPlainBorderColor = "#4A4A4A";

inline constexpr const char* kButtonBackground = "#3A3A3A";
inline constexpr const char* kButtonBackgroundHover = "#454545";
inline constexpr const char* kButtonBackgroundPressed = "#2F2F2F";

// El texto del boton NO lo pinta el stylesheet: lo dibuja DialogButton a mano para
// poder subrayar el mnemonico en mac. Toma estos dos colores.
inline constexpr const char* kButtonTextColor = "#E6E6E6";
inline constexpr const char* kButtonTextColorDisabled = "#7A7A7A";

// -------------------------------------------------------------------------- geometria
inline constexpr int kButtonPaddingV = 6;
inline constexpr int kButtonPaddingH = 18;
inline constexpr int kButtonBorderRadius = 4;
inline constexpr int kButtonSpacing = 8;

// Un unico ancho minimo para todos: antes cada dialogo elegia el suyo (360 / 380 / 400 /
// 420) y se notaba al verlos seguidos.
inline constexpr int kDialogMinimumWidth = 400;
inline constexpr int kDialogMargin = 18;
inline constexpr int kIconSize = 32;
// Separacion horizontal entre el icono y el texto.
inline constexpr int kIconTextSpacing = 14;
// Correccion OPTICA: sube el bloque de texto estos pixeles respecto del centro geometrico
// del icono. Centrado matematicamente exacto, el texto se lee un poco bajo — la caja de
// TINTA de una linea no coincide con su caja de LINEA, porque la linea reserva descenso
// para las letras con cola aunque el renglon no las tenga. Se aplica como margen inferior
// del doble, que con AlignVCenter equivale a subir la mitad.
inline constexpr int kTextOpticalLift = 2;
// Separacion vertical entre el bloque de mensaje y la fila de botones: una linea en
// blanco, para que los botones no queden pegados al texto.
inline constexpr int kMessageToButtonsSpacing = 20;
// Margen extra alrededor de la fila de botones para que el glow no quede recortado por
// el borde del layout (QGraphicsDropShadowEffect pinta fuera del rect del widget).
inline constexpr int kGlowMargin = 6;

// ------------------------------------------------------------------------------ helpers
/**
 * Destaca en blanco un pedazo del mensaje. El cuerpo va gris, asi que esto es lo unico
 * que se lee de un vistazo: usarlo SOLO en lo que decide la respuesta (cuantos archivos,
 * donde, y la advertencia que no se puede deshacer). Si se destaca todo, no se destaca
 * nada.
 */
inline QString emphasis(const QString& text)
{
    return QStringLiteral("<span style='color:%1'>%2</span>")
        .arg(QLatin1String(kEmphasisTextColor), text);
}

// ------------------------------------------------------------------------------ estilos
// Fondo y color de texto del dialogo. Va con selector por objectName para que no se
// derrame a los botones, que tienen su propia caja.
inline QString dialogStyle(const QString& objectName)
{
    return QStringLiteral("QDialog#%1 { background-color: %2; }"
                          "QDialog#%1 QLabel { color: %3; background: transparent; }")
        .arg(objectName, QLatin1String(kDialogBackground), QLatin1String(kMessageTextColor));
}

// Bloque de detalle colapsable. Selector por objectName, igual que el dialogo: el QSS del
// dialogo pinta sus QLabel, y esto pinta el QTextEdit sin derramarse a nada mas.
inline QString detailStyle(const QString& objectName)
{
    return QStringLiteral("QTextEdit#%1 { background-color: %2; color: %3;"
                          " border: 1px solid %4; border-radius: %5px; padding: %6px; }")
        .arg(objectName, QLatin1String(kDetailBackground), QLatin1String(kDetailTextColor),
             QLatin1String(kDetailBorderColor))
        .arg(kDetailBorderRadius)
        .arg(kDetailPadding);
}

// Caja comun de los dos estados del boton: lo UNICO que cambia entre marcado y sin
// marcar es el color del borde.
inline QString buttonStyle(const char* borderColor)
{
    return QStringLiteral("QPushButton { border: 1px solid %1; border-radius: %2px;"
                          " padding: %3px %4px; background-color: %5; }"
                          "QPushButton:hover { background-color: %6; }"
                          "QPushButton:pressed { background-color: %7; }")
        .arg(QLatin1String(borderColor))
        .arg(kButtonBorderRadius)
        .arg(kButtonPaddingV)
        .arg(kButtonPaddingH)
        .arg(QLatin1String(kButtonBackground),
             QLatin1String(kButtonBackgroundHover),
             QLatin1String(kButtonBackgroundPressed));
}

// Estilo del boton MARCADO (el que activa Enter).
inline QString markedButtonStyle()
{
    return buttonStyle(kMarkedBorderColor);
}

// Estilo del boton sin marcar.
inline QString plainButtonStyle()
{
    return buttonStyle(kPlainBorderColor);
}

} // namespace DialogStyle
