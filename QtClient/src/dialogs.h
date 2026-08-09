#pragma once

#include <QList>
#include <QPointer>
#include <QString>
#include <Qt>

#include <functional>

class QDialog;
class QWidget;

/**
 * Dialogos de eleccion de la app. COMPORTAMIENTO POR DEFECTO: todo cartel con mas de un
 * camino se arma con ask(); no se usa QMessageBox suelto (ver docs/Doc_Dialogs.md).
 *
 * El look sale de DialogStyle.h, que es el unico lugar donde tocarlo.
 *
 * Todo dialogo armado con ask() garantiza, siempre igual:
 *  - La marca violeta NO miente: autoDefault en todos los botones + la marca sigue al
 *    foco, asi que el boton marcado es siempre el que ejecuta Enter.
 *  - Navegacion con Tab/Shift+Tab (nativo) y flechas ←→↑↓ con wrap-around.
 *  - Atajos de tecla suelta (sin modificador) con la letra subrayada en mac y Windows.
 *  - Escape o cerrar la ventana => kEscape, nunca un boton por accidente.
 */
namespace Dialogs {

class Handle;

/**
 * Icono del dialogo. El default es None A PROPOSITO.
 *
 * El triangulo amarillo solo sirve si es raro: cuando lo tienen TODOS los carteles deja
 * de informar y se vuelve decoracion, y ademas le pone carga de alarma a preguntas que
 * no la tienen (un upload, un rename, elegir entre Skip y Overwrite). Va unicamente
 * donde la accion DESTRUYE algo o no se puede deshacer.
 */
enum class Icon {
    None,         ///< preguntas: transferencias, rename, move, conflictos
    Warning,      ///< borrados y todo lo irreversible o que descarta trabajo
    // Estos dos NO son para preguntas: son el icono de info()/error(), donde el icono ES
    // la informacion (que tan grave es lo que paso) y no un adorno sobre una pregunta.
    // Estan expuestos para poder armar esos mismos carteles en modo no modal con show().
    Information,  ///< avisos: lo mismo que muestra info()
    Critical,     ///< fallos: lo mismo que muestra error()
};

/**
 * Que hace el boton con el dialogo.
 *
 * `Close` es el caso normal: el boton cierra y su indice es lo que devuelve ask() (o lo
 * que recibe el callback de show()). `Action` es un boton que ejecuta algo y DEJA el
 * dialogo abierto —el "Copy" del ErrorReporter, que copia el detalle al portapapeles sin
 * hacer desaparecer justo lo que se esta copiando—; su trabajo lo hace `onAction`.
 */
enum class ButtonRole {
    Close,   ///< cierra el dialogo (default)
    Action,  ///< ejecuta onAction y deja el dialogo abierto
};

/// Un boton del dialogo.
struct Button {
    /// Texto con '&' adelante de la letra del atajo: "&Yes" subraya la Y.
    QString text;
    /// Tecla suelta, sin modificadores. Qt::Key_unknown = sin atajo.
    Qt::Key shortcut = Qt::Key_unknown;
    /// Close (default) o Action.
    ButtonRole role = ButtonRole::Close;
    /**
     * Solo para ButtonRole::Action. Recibe el dialogo vivo, asi se le puede cambiar el
     * texto al propio boton ("Copy" => "Copied") o cerrarlo desde adentro.
     */
    std::function<void(Handle&)> onAction;
};

/**
 * Handle al dialogo mientras esta abierto. Lo recibe el callback de un boton de accion,
 * que es el unico momento en que el dialogo existe y todavia no se cerro.
 *
 * Todo lo que expone tolera que el dialogo ya no exista (QPointer): un callback diferido
 * no tiene por que saber si el usuario cerro la ventana mientras tanto.
 */
class Handle
{
public:
    Handle();
    explicit Handle(QDialog* dialog);

    /// Cambia el texto de un boton por su indice dentro de la lista original.
    void setButtonText(int index, const QString& text);
    /// Cierra el dialogo como si se hubiera apretado el boton `buttonIndex` (o kEscape).
    void close(int buttonIndex = -1);
    /// El QDialog, por si hace falta parentear algo. Puede ser nullptr.
    QDialog* dialog() const;

private:
    // QObject y no QDialog: QPointer necesita el tipo COMPLETO, y este header lo unico que
    // tiene de QDialog es la forward declaration. El cast vive en el .cpp.
    QPointer<QObject> m_dialog;
};

/**
 * Opciones de presentacion. Todas opcionales: `{}` es el dialogo de siempre.
 */
struct Options {
    /// Ver Icon: por defecto NO va icono.
    Icon icon = Icon::None;
    /// Como se interpreta `message`.
    Qt::TextFormat textFormat = Qt::AutoText;
    /**
     * Detalle tecnico —traceback, salida de un proceso, estado previo/actual— detras de un
     * boton "Show Details". Arranca COLAPSADO: el cartel sigue siendo corto y el detalle
     * esta ahi para el que lo necesita. Equivale al setDetailedText de QMessageBox, y su
     * area es siempre seleccionable.
     */
    QString detailedText;
    /**
     * Deja seleccionar el mensaje con el mouse. Va donde el texto es algo que el usuario
     * necesita copiar (un error que va a pegar en un reporte), no en preguntas normales.
     */
    bool selectableText = false;
};

/// Valor que devuelve ask() cuando el usuario sale con Escape o cierra la ventana.
inline constexpr int kEscape = -1;

/**
 * Dialogo de eleccion generico. Devuelve el INDICE del boton apretado dentro de
 * `buttons`, o kEscape.
 *
 * `defaultIndex` es el boton que arranca marcado y con el foco: por convencion, la
 * opcion NO destructiva.
 */
int ask(QWidget* parent,
        const QString& title,
        const QString& message,
        const QList<Button>& buttons,
        int defaultIndex,
        Icon icon = Icon::None,
        Qt::TextFormat textFormat = Qt::AutoText);

/**
 * Igual que ask(), con Options en vez de los dos parametros sueltos: es la forma de pedir
 * detalle colapsable, texto seleccionable o botones de accion.
 *
 * `Options` NO tiene valor por defecto a proposito: con default esta sobrecarga se volveria
 * ambigua contra la de arriba en las llamadas de cinco argumentos.
 */
int ask(QWidget* parent,
        const QString& title,
        const QString& message,
        const QList<Button>& buttons,
        int defaultIndex,
        const Options& options);

/**
 * Version NO MODAL: muestra el cartel y vuelve en el acto. El usuario sigue trabajando y
 * decide despues; `onClosed` recibe el indice del boton que cerro, o kEscape.
 *
 * El dialogo se destruye solo (WA_DeleteOnClose), asi que no hay nada que borrar. Devuelve
 * un Handle por si hace falta cerrarlo desde afuera; ignorarlo es lo normal.
 *
 * OJO: no modal NO es "modal pero comodo". Va donde el aviso no bloquea nada de lo que el
 * usuario esta haciendo. Si el flujo no puede seguir sin la respuesta, va ask().
 */
Handle show(QWidget* parent,
            const QString& title,
            const QString& message,
            const QList<Button>& buttons,
            int defaultIndex,
            const Options& options = {},
            std::function<void(int)> onClosed = {});

/**
 * Yes/No. Devuelve true si el usuario eligio Yes. Escape => false (No).
 * Atajos: Y => Yes, N => No.
 */
bool askYesNo(QWidget* parent,
              const QString& title,
              const QString& message,
              bool defaultYes = true,
              Icon icon = Icon::None,
              Qt::TextFormat textFormat = Qt::AutoText);

/// Par de paths ya coloreados, listos para meter en un mensaje RichText.
struct ColoredPathPair {
    QString from;
    QString to;
};

/**
 * Colorea un par origen/destino para que la diferencia entre los dos se vea sin leer.
 *
 * La parte COMUN de los dos paths —incluido el esquema ("s3://")— va en lavanda: es el
 * mismo color en los dos lados porque es lo mismo, y donde ese color se corta es donde
 * los paths se separan. De ahi en adelante se recorre la paleta del Activity Tab en su
 * orden, IGUAL en los dos lados, asi que el color marca el NIVEL y los dos paths se leen
 * como una sola grilla en vez de como dos escalas distintas.
 *
 * Los segmentos se escapan para HTML: una key de S3 puede traer & o <.
 */
ColoredPathPair colorizePathPair(const QString& fromPath, const QString& toPath);

/**
 * Colorea un path SUELTO (sin par contra el cual compararlo).
 *
 * Recorre la paleta del Activity Tab en orden, arrancando por el lavanda —el mismo color
 * con el que arranca un par— y volviendo a empezar cuando se acaba. Asi un path suelto y
 * la parte comun de un par se ven iguales, que es lo que hace que la app tenga UN solo
 * lenguaje de color para rutas.
 *
 * REGLA: un path en un mensaje va SIEMPRE en su propia linea, debajo del texto. Metido
 * en el medio de una oracion se corta por donde cae el wrap y se vuelve ilegible.
 */
QString colorizePath(const QString& path);

// Un solo boton OK, marcado. Reemplazan a QMessageBox::information/warning/critical.
void info(QWidget* parent, const QString& title, const QString& message);
void warn(QWidget* parent, const QString& title, const QString& message);
void error(QWidget* parent, const QString& title, const QString& message);

} // namespace Dialogs
