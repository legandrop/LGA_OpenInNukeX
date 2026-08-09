#include "dialogs.h"

#include "dialogbutton.h"
#include "dialogstyle.h"

#include <QApplication>
#include <QDialog>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QPointer>
#include <QRegularExpression>
#include <QStyle>
#include <QTextDocument>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVector>

#include <cmath>

namespace {

// Los botones devuelven su indice via QDialog::done(). done(0) es Rejected, que queda
// reservado para Escape / cerrar la ventana, asi que los indices se corren.
constexpr int kResultOffset = 100;

/**
 * Navegacion por teclado y marca violeta de la fila de botones. Generalizado a N botones
 * (hay dialogos de dos y de tres).
 *
 *  - Flechas ←→↑↓ y Tab/Shift+Tab: mueven el foco con wrap-around.
 *    Las flechas hay que filtrarlas porque Qt no navega con flechas entre QPushButtons
 *    sueltos. Y el Tab tambien, aunque Qt SI lo maneje: en macOS el foco por Tab sobre
 *    botones depende de "Keyboard navigation" del sistema, que viene apagado por defecto,
 *    asi que sin esto el Tab funciona en Windows y no en mac.
 *  - FocusIn: mueve la marca al boton que recibe el foco, para que la marca indique
 *    siempre que hace Enter. NO se consume el evento: el boton necesita su focusInEvent
 *    para volverse el default.
 *
 * Sin Q_OBJECT: un event filter no necesita moc.
 */
class DialogButtonNavFilter : public QObject
{
public:
    DialogButtonNavFilter(const QVector<DialogButton*>& buttons,
                          const QVector<QGraphicsDropShadowEffect*>& glows,
                          QObject* parent)
        : QObject(parent)
    {
        // QPointer para no quedar con punteros colgados si el dialogo se destruye antes.
        for (DialogButton* btn : buttons) {
            m_buttons.append(btn);
        }
        for (QGraphicsDropShadowEffect* glow : glows) {
            m_glows.append(glow);
        }
    }

    // Pinta la marca en el boton `index` y la saca de todos los demas.
    void markButton(int index)
    {
        for (int i = 0; i < m_buttons.size(); ++i) {
            if (!m_buttons.at(i)) {
                continue;
            }
            const bool marked = (i == index);
            m_buttons.at(i)->setStyleSheet(marked ? DialogStyle::markedButtonStyle()
                                                  : DialogStyle::plainButtonStyle());
            // Un widget admite UN solo QGraphicsEffect, asi que el glow no se puede mudar
            // de boton: hay uno por boton y se prende el que corresponde.
            if (i < m_glows.size() && m_glows.at(i)) {
                m_glows.at(i)->setEnabled(marked);
            }
        }
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::FocusIn) {
            markButton(indexOf(watched));
            return false; // NO consumir
        }
        if (event->type() != QEvent::KeyPress) {
            return QObject::eventFilter(watched, event);
        }
        const int key = static_cast<QKeyEvent*>(event)->key();
        int delta = 0;
        if (key == Qt::Key_Right || key == Qt::Key_Down || key == Qt::Key_Tab) {
            delta = 1;
        } else if (key == Qt::Key_Left || key == Qt::Key_Up || key == Qt::Key_Backtab) {
            delta = -1;
        } else {
            return QObject::eventFilter(watched, event);
        }
        const int current = indexOf(watched);
        if (current < 0 || m_buttons.size() < 2) {
            return QObject::eventFilter(watched, event);
        }
        const int next = (current + delta + m_buttons.size()) % m_buttons.size();
        if (m_buttons.at(next)) {
            m_buttons.at(next)->setFocus(Qt::TabFocusReason);
        }
        return true; // consumido: no llega al boton
    }

private:
    int indexOf(const QObject* watched) const
    {
        for (int i = 0; i < m_buttons.size(); ++i) {
            if (m_buttons.at(i) == watched) {
                return i;
            }
        }
        return -1;
    }

    QVector<QPointer<DialogButton>> m_buttons;
    QVector<QPointer<QGraphicsDropShadowEffect>> m_glows;
};

// Nombre de objeto de cada boton. Es como Handle::setButtonText lo encuentra sin que el
// header tenga que exponer los widgets.
QString buttonObjectName(int index)
{
    return QStringLiteral("lgaDialogButton%1").arg(index);
}

// Texto del toggle del bloque de detalle. Es UI, o sea ingles.
const QLatin1String kShowDetailsText("Show &Details");
const QLatin1String kHideDetailsText("Hide &Details");

// Traduce el icono del API al pixmap estandar del estilo. -1 = sin icono.
int standardPixmapFor(Dialogs::Icon icon)
{
    switch (icon) {
    case Dialogs::Icon::Warning:
        return static_cast<int>(QStyle::SP_MessageBoxWarning);
    case Dialogs::Icon::Information:
        return static_cast<int>(QStyle::SP_MessageBoxInformation);
    case Dialogs::Icon::Critical:
        return static_cast<int>(QStyle::SP_MessageBoxCritical);
    case Dialogs::Icon::None:
        break;
    }
    return -1;
}

/**
 * Arma el dialogo y lo devuelve SIN mostrar, para que el llamador elija exec() (modal) o
 * show() (no modal). Todo lo que hay que mantener vivo cuelga del QDialog, asi que quien
 * lo destruye se lleva todo.
 *
 * Devuelve nullptr si `buttons` viene vacia.
 */
QDialog* buildDialog(QWidget* parent,
                     const QString& title,
                     const QString& message,
                     const QList<Dialogs::Button>& buttons,
                     int defaultIndex,
                     const Dialogs::Options& options)
{
    if (buttons.isEmpty()) {
        return nullptr;
    }
    defaultIndex = qBound(0, defaultIndex, buttons.size() - 1);
    const int icon = standardPixmapFor(options.icon);
    const Qt::TextFormat textFormat = options.textFormat;

    // El dialogo va al heap para poder devolverlo sin mostrar. Los lambdas capturan `dlg`
    // POR VALOR: capturar la referencia `dialog` los dejaria apuntando a una variable local
    // de esta funcion, que muere al volver.
    QDialog* const dlg = new QDialog(parent);
    QDialog& dialog = *dlg;
    dialog.setObjectName(QStringLiteral("lgaDialog"));
    dialog.setWindowTitle(title);
    dialog.setMinimumWidth(DialogStyle::kDialogMinimumWidth);
    // Fondo y color de texto salen de DialogStyle, no del QSS global de la app: el
    // componente se comparte entre apps y cada repo tiene su propio dark_theme.qss.
    dialog.setStyleSheet(DialogStyle::dialogStyle(dialog.objectName()));

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(DialogStyle::kDialogMargin, DialogStyle::kDialogMargin,
                                   DialogStyle::kDialogMargin, DialogStyle::kDialogMargin);
    // El espaciado entre bloques lo pone el codigo con addSpacing, no el layout: asi el
    // aire entre mensaje y botones es un unico valor de DialogStyle y no la suma de
    // ese valor con el spacing por defecto de Qt.
    mainLayout->setSpacing(0);

    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setSpacing(DialogStyle::kIconTextSpacing);
    if (icon >= 0) {
        QLabel* iconLabel = new QLabel();
        iconLabel->setPixmap(QApplication::style()
                                 ->standardIcon(static_cast<QStyle::StandardPixmap>(icon))
                                 .pixmap(DialogStyle::kIconSize, DialogStyle::kIconSize));
        iconLabel->setFixedSize(DialogStyle::kIconSize, DialogStyle::kIconSize);
        // El icono se centra VERTICALMENTE contra el bloque de texto, no se ancla arriba.
        // Con una linea los dos quedan centrados entre si; con varias, el icono queda a la
        // altura del medio del bloque. Anclarlo arriba deja el texto "colgando" hacia
        // abajo y solo se ve bien con una linea.
        topLayout->addWidget(iconLabel, 0, Qt::AlignVCenter);
    } else {
        // Sin icono se reserva igual su ancho: un dialogo sin triangulo no tiene por que
        // arrancar el texto en otro margen que los demas. Vistos uno detras de otro, ese
        // salto se nota mas que el icono faltante.
        topLayout->addSpacing(DialogStyle::kIconSize);
    }
    QLabel* textLabel = new QLabel(message);
    textLabel->setWordWrap(true);
    textLabel->setTextFormat(textFormat);
    if (options.selectableText) {
        // Solo donde el texto es algo que el usuario necesita copiar (un error que va a
        // pegar en un reporte). En una pregunta normal no aporta y deja el cursor de texto.
        textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }
    // El texto va CENTRADO dentro de su label, no anclado arriba.
    // Con varias lineas da igual (el label mide lo mismo que su contenido), pero con UNA
    // el label se estira a la altura de la fila —que la fija el icono, mas alto— y anclar
    // arriba deja la linea 3 px por encima del centro del icono. Se ve, y era el motivo
    // por el que "los textos quedaban altos".
    // OJO: el centrado va en el LABEL (setAlignment), no en el addWidget. Pasarle una
    // alineacion al layout le saca al label su altura estirada y con ella el
    // heightForWidth, asi que el texto envuelto se calcula corto y se ve CORTADO.
    textLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    textLabel->setContentsMargins(0, 0, 0, DialogStyle::kTextOpticalLift * 2);
    // La columna de texto tiene ancho FIJO y su altura se calcula a mano.
    // Dos razones. Una: el heightForWidth de un QLabel con wordWrap + RichText se
    // subestima al propagarse por layouts anidados, y el resultado es texto CORTADO en la
    // ultima linea — un bug que venia tapado por el aire de mas que habia entre el mensaje
    // y los botones. Y dos: con el ancho fijo todos los dialogos miden exactamente lo
    // mismo, que es la mitad de que se vean como uno solo.
    const int textWidth = DialogStyle::kDialogMinimumWidth
                          - 2 * DialogStyle::kDialogMargin
                          - DialogStyle::kIconSize
                          - DialogStyle::kIconTextSpacing;
    textLabel->setFixedWidth(textWidth);
    // La altura se mide con un QTextDocument propio, no con QLabel::heightForWidth: ese
    // se queda corto con RichText (aparecio con un path de Windows, donde el wrap cae
    // distinto) y el sintoma es la ULTIMA LINEA CORTADA. Se toma el mayor de los dos por
    // si algun estilo agrega su propio alto.
    QTextDocument measure;
    measure.setDefaultFont(textLabel->font());
    const bool asRichText = (textFormat == Qt::RichText)
                            || (textFormat == Qt::AutoText && Qt::mightBeRichText(message));
    if (asRichText) {
        measure.setHtml(message);
    } else {
        measure.setPlainText(message);
    }
    measure.setTextWidth(textWidth);
    const int measuredHeight = static_cast<int>(std::ceil(measure.size().height()));
    textLabel->setMinimumHeight(qMax(measuredHeight, textLabel->heightForWidth(textWidth))
                                + DialogStyle::kTextOpticalLift * 2);
    topLayout->addWidget(textLabel, 1);
    mainLayout->addLayout(topLayout);
    mainLayout->addSpacing(DialogStyle::kMessageToButtonsSpacing);

    // Bloque de detalle tecnico: arranca COLAPSADO y no ocupa lugar mientras lo esta (un
    // widget oculto mide 0 en el layout), asi que el cartel se ve igual de corto que
    // cualquier otro hasta que alguien pida el detalle.
    // Va envuelto en un contenedor cuyo margen inferior es el aire hasta los botones: asi
    // ese aire desaparece junto con el detalle en vez de quedar como un hueco vacio.
    QWidget* detailBox = nullptr;
    if (!options.detailedText.isEmpty()) {
        detailBox = new QWidget(&dialog);
        auto* detailLayout = new QVBoxLayout(detailBox);
        detailLayout->setContentsMargins(0, 0, 0, DialogStyle::kMessageToButtonsSpacing);
        detailLayout->setSpacing(0);

        auto* detailEdit = new QTextEdit(detailBox);
        detailEdit->setObjectName(QStringLiteral("lgaDialogDetail"));
        detailEdit->setReadOnly(true);
        // Sin wrap: el detalle es salida de un proceso o un traceback, donde el corte de
        // linea original es informacion. Si no entra, se scrollea en horizontal.
        detailEdit->setLineWrapMode(QTextEdit::NoWrap);
        detailEdit->setPlainText(options.detailedText);
        detailEdit->setMinimumHeight(DialogStyle::kDetailMinimumHeight);
        detailEdit->setMaximumHeight(DialogStyle::kDetailMaximumHeight);
        detailEdit->setStyleSheet(DialogStyle::detailStyle(detailEdit->objectName()));
        detailLayout->addWidget(detailEdit);

        detailBox->hide();
        mainLayout->addWidget(detailBox);
    }

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(DialogStyle::kButtonSpacing);
    // El glow pinta FUERA del rect del boton, asi que sin este margen queda recortado.
    buttonLayout->setContentsMargins(DialogStyle::kGlowMargin, DialogStyle::kGlowMargin,
                                     DialogStyle::kGlowMargin, DialogStyle::kGlowMargin);

    QVector<DialogButton*> widgets; // en orden VISUAL: el toggle del detalle va primero
    QVector<QGraphicsDropShadowEffect*> glows;
    widgets.reserve(buttons.size() + 1);
    glows.reserve(buttons.size() + 1);

    // Fabrica comun: todos los botones del dialogo se arman igual, incluido el del detalle.
    const auto makeButton = [&](const QString& text, Qt::Key shortcut) {
        auto* btn = new DialogButton(text);
        if (shortcut != Qt::Key_unknown) {
            // Tecla suelta, sin Alt: es como se operan estos dialogos. El subrayado lo
            // pinta DialogButton a partir del '&' del texto.
            btn->setShortcut(QKeySequence(shortcut));
        }
        // autoDefault en TODOS: el boton enfocado pasa a ser el default, o sea que Enter
        // sigue al foco. Es condicion para que la marca violeta no mienta.
        btn->setAutoDefault(true);

        auto* glow = new QGraphicsDropShadowEffect(btn);
        glow->setBlurRadius(DialogStyle::kGlowBlurRadius);
        glow->setColor(DialogStyle::kGlowColor);
        glow->setOffset(0, 0);
        btn->setGraphicsEffect(glow);

        buttonLayout->addWidget(btn);
        widgets.append(btn);
        glows.append(glow);
        return btn;
    };

    // El toggle del detalle va a la IZQUIERDA, separado del resto por el stretch: no es una
    // respuesta a la pregunta del cartel, asi que no se mezcla con los botones que la
    // contestan.
    if (detailBox) {
        DialogButton* toggle = makeButton(kShowDetailsText, Qt::Key_D);
        QObject::connect(toggle, &QPushButton::clicked, &dialog, [dlg, detailBox, toggle]() {
            const bool expanded = !detailBox->isVisible();
            detailBox->setVisible(expanded);
            toggle->setText(expanded ? kHideDetailsText : kShowDetailsText);
            // Sin activate() el layout todavia reporta la geometria vieja y adjustSize()
            // deja el dialogo del alto anterior.
            dlg->layout()->activate();
            dlg->adjustSize();
        });
    }
    buttonLayout->addStretch();

    const int userOffset = widgets.size(); // 0, o 1 si hay toggle de detalle
    for (int i = 0; i < buttons.size(); ++i) {
        const Dialogs::Button& spec = buttons.at(i);
        DialogButton* btn = makeButton(spec.text, spec.shortcut);
        btn->setObjectName(buttonObjectName(i));
        if (spec.role == Dialogs::ButtonRole::Action) {
            // Un boton de accion NO cierra: ejecuta lo suyo y deja el cartel como estaba.
            QObject::connect(btn, &QPushButton::clicked, &dialog, [dlg, spec]() {
                if (spec.onAction) {
                    Dialogs::Handle handle(dlg);
                    spec.onAction(handle);
                }
            });
        } else {
            QObject::connect(btn, &QPushButton::clicked, &dialog,
                             [dlg, i]() { dlg->done(kResultOffset + i); });
        }
    }
    mainLayout->addLayout(buttonLayout);

    auto* navFilter = new DialogButtonNavFilter(widgets, glows, &dialog);
    for (DialogButton* btn : widgets) {
        btn->installEventFilter(navFilter);
    }
    // La marca arranca en el boton default, antes de que llegue ningun FocusIn.
    navFilter->markButton(userOffset + defaultIndex);
    widgets.at(userOffset + defaultIndex)->setDefault(true);
    widgets.at(userOffset + defaultIndex)->setFocus();

    return &dialog;
}

// Corre el dialogo MODAL y traduce el codigo de QDialog al indice del boton.
// Escape y la X de la ventana disparan reject() => QDialog::Rejected.
int runModal(QDialog* dialog)
{
    if (!dialog) {
        return Dialogs::kEscape;
    }
    // Se destruye a mano: WA_DeleteOnClose con exec() dejaria el puntero colgado justo
    // cuando hay que leerle el resultado.
    const int result = dialog->exec();
    delete dialog;
    if (result < kResultOffset) {
        return Dialogs::kEscape;
    }
    return result - kResultOffset;
}

// Espacio de ancho cero. Va despues de cada separador para darle a Qt un punto donde
// cortar la linea: corta en '/' pero NO en '\\', asi que un path de Windows es UNA sola
// palabra impartible y, si no entra en el ancho del dialogo, se sale y queda CORTADA.
const QLatin1String kZeroWidthSpace("&#8203;");

// Como se muestra un path: el prefijo que va antes del primer segmento (el esquema
// "s3://", o la barra inicial de un path absoluto de POSIX) y el separador que se dibuja
// entre segmentos.
struct PathParts {
    QString prefix;
    QString separator;
    QStringList segments;
};

// Parte un path en segmentos. El prefijo queda aparte para que no se cuente como un
// directorio ni se coloree como tal, pero NO se descarta: sin la barra inicial un path
// absoluto de POSIX se muestra como si fuera relativo.
// Se parte por las dos barras porque en Windows los paths vienen con '\\' (los devuelve
// QDir::toNativeSeparators), y se redibuja la que el path traia.
PathParts splitPathSegments(const QString& path)
{
    PathParts parts;
    QString rest = path;

    const int schemeEnd = rest.indexOf(QStringLiteral("://"));
    if (schemeEnd > 0) {
        parts.prefix = rest.left(schemeEnd + 3);
        rest = rest.mid(schemeEnd + 3);
    }

    const bool usesBackslash = rest.contains(QLatin1Char('\\')) && !rest.contains(QLatin1Char('/'));
    parts.separator = usesBackslash ? QStringLiteral("\\") : QStringLiteral("/");
    if (parts.prefix.isEmpty() && (rest.startsWith(QLatin1Char('/')) || rest.startsWith(QLatin1Char('\\')))) {
        parts.prefix = parts.separator; // path absoluto de POSIX (o UNC de Windows)
    }

    parts.segments = rest.split(QRegularExpression(QStringLiteral("[/\\\\]")), Qt::SkipEmptyParts);
    return parts;
}

// Color del segmento `index` de un path SUELTO: arranca en el color comun (el lavanda con
// el que arranca un par) y sigue con la paleta en orden, volviendo a empezar al final. Un
// path solo y la parte comun de un par se ven iguales: un unico lenguaje de color.
const char* singlePathColor(int index)
{
    constexpr int total = 1 + static_cast<int>(DialogStyle::kPathPalette.size());
    const int step = index % total;
    return (step == 0) ? DialogStyle::kPathCommonColor
                       : DialogStyle::kPathPalette[step - 1];
}

// Arma el HTML de UN lado del par. `commonCount` es cuantos segmentos iniciales comparte
// con el otro lado: hasta ahi los dos van del color comun, y de ahi en adelante se recorre
// la paleta en orden. Los DOS lados la recorren en el MISMO sentido, asi que el color pasa
// a marcar el NIVEL —igual que en el Activity Tab— y no de que lado del par esta cada
// directorio. Lo que separa origen de destino es el arranque del color comun, no el orden.
QString buildColoredPath(const PathParts& parts, int commonCount)
{
    const QStringList& segments = parts.segments;
    const QString sep = QStringLiteral("<span style='color:%1'>%2</span>%3")
                            .arg(QLatin1String(DialogStyle::kPathSeparatorColor),
                                 parts.separator.toHtmlEscaped(), kZeroWidthSpace);
    QString html;
    if (!parts.prefix.isEmpty()) {
        // El prefijo va con el color de la parte COMUN: "s3://" es lo mismo en los dos
        // lados, asi que pintarlo distinto lo separaria del primer directorio, que
        // tambien es comun.
        html += QStringLiteral("<span style='color:%1'>%2</span>%3")
                    .arg(QLatin1String(DialogStyle::kPathCommonColor),
                         parts.prefix.toHtmlEscaped(), kZeroWidthSpace);
    }
    constexpr int paletteSize = static_cast<int>(DialogStyle::kPathPalette.size());
    for (int i = 0; i < segments.size(); ++i) {
        if (i > 0) {
            html += sep;
        }
        const char* color = DialogStyle::kPathCommonColor;
        if (i >= commonCount) {
            color = DialogStyle::kPathPalette[(i - commonCount) % paletteSize];
        }
        html += QStringLiteral("<span style='color:%1'>%2</span>")
                    .arg(QLatin1String(color), segments.at(i).toHtmlEscaped());
    }
    return html;
}

void showSingleOk(QWidget* parent, const QString& title, const QString& message,
                  Dialogs::Icon icon)
{
    Dialogs::Options options;
    options.icon = icon;
    runModal(buildDialog(parent, title, message, {{QStringLiteral("&OK"), Qt::Key_O}}, 0, options));
}

} // namespace

namespace Dialogs {

Handle::Handle() = default;

Handle::Handle(QDialog* dialog) : m_dialog(dialog) {}

QDialog* Handle::dialog() const
{
    return qobject_cast<QDialog*>(m_dialog.data());
}

void Handle::setButtonText(int index, const QString& text)
{
    QDialog* dlg = dialog();
    if (!dlg) {
        return;
    }
    if (auto* btn = dlg->findChild<DialogButton*>(buttonObjectName(index))) {
        btn->setText(text);
    }
}

void Handle::close(int buttonIndex)
{
    QDialog* dlg = dialog();
    if (!dlg) {
        return;
    }
    if (buttonIndex < 0) {
        dlg->reject();
    } else {
        dlg->done(kResultOffset + buttonIndex);
    }
}

int ask(QWidget* parent,
        const QString& title,
        const QString& message,
        const QList<Button>& buttons,
        int defaultIndex,
        Icon icon,
        Qt::TextFormat textFormat)
{
    Options options;
    options.icon = icon;
    options.textFormat = textFormat;
    return ask(parent, title, message, buttons, defaultIndex, options);
}

int ask(QWidget* parent,
        const QString& title,
        const QString& message,
        const QList<Button>& buttons,
        int defaultIndex,
        const Options& options)
{
    return runModal(buildDialog(parent, title, message, buttons, defaultIndex, options));
}

Handle show(QWidget* parent,
            const QString& title,
            const QString& message,
            const QList<Button>& buttons,
            int defaultIndex,
            const Options& options,
            std::function<void(int)> onClosed)
{
    QDialog* dialog = buildDialog(parent, title, message, buttons, defaultIndex, options);
    if (!dialog) {
        return Handle();
    }
    // No modal DE VERDAD: ni siquiera bloquea a su propia ventana padre. El usuario sigue
    // trabajando y contesta cuando quiere, que es la razon por la que estos carteles no son
    // ask().
    dialog->setWindowModality(Qt::NonModal);
    // Se limpia solo: nadie tiene un puntero al que volver despues de un show().
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    if (onClosed) {
        QObject::connect(dialog, &QDialog::finished, dialog, [onClosed](int result) {
            onClosed(result < kResultOffset ? kEscape : result - kResultOffset);
        });
    }
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
    return Handle(dialog);
}

bool askYesNo(QWidget* parent,
              const QString& title,
              const QString& message,
              bool defaultYes,
              Icon icon,
              Qt::TextFormat textFormat)
{
    const QList<Button> buttons = {
        {QStringLiteral("&No"), Qt::Key_N},
        {QStringLiteral("&Yes"), Qt::Key_Y},
    };
    // Escape devuelve kEscape, que no es 1 => No. Es el comportamiento que ya tenia el
    // Yes/No historico (Escape => No).
    return ask(parent, title, message, buttons, defaultYes ? 1 : 0, icon, textFormat) == 1;
}

QString colorizePath(const QString& path)
{
    const PathParts parts = splitPathSegments(path);
    const QString sep = QStringLiteral("<span style='color:%1'>%2</span>%3")
                            .arg(QLatin1String(DialogStyle::kPathSeparatorColor),
                                 parts.separator.toHtmlEscaped(), kZeroWidthSpace);
    QString html;
    if (!parts.prefix.isEmpty()) {
        html += QStringLiteral("<span style='color:%1'>%2</span>%3")
                    .arg(QLatin1String(singlePathColor(0)), parts.prefix.toHtmlEscaped(),
                         kZeroWidthSpace);
    }
    for (int i = 0; i < parts.segments.size(); ++i) {
        if (i > 0) {
            html += sep;
        }
        html += QStringLiteral("<span style='color:%1'>%2</span>")
                    .arg(QLatin1String(singlePathColor(i)), parts.segments.at(i).toHtmlEscaped());
    }
    return html;
}

ColoredPathPair colorizePathPair(const QString& fromPath, const QString& toPath)
{
    const PathParts from = splitPathSegments(fromPath);
    const PathParts to = splitPathSegments(toPath);

    int commonCount = 0;
    while (commonCount < from.segments.size() && commonCount < to.segments.size()
           && from.segments.at(commonCount) == to.segments.at(commonCount)) {
        ++commonCount;
    }

    return {buildColoredPath(from, commonCount), buildColoredPath(to, commonCount)};
}

void info(QWidget* parent, const QString& title, const QString& message)
{
    showSingleOk(parent, title, message, Icon::Information);
}

void warn(QWidget* parent, const QString& title, const QString& message)
{
    showSingleOk(parent, title, message, Icon::Warning);
}

void error(QWidget* parent, const QString& title, const QString& message)
{
    showSingleOk(parent, title, message, Icon::Critical);
}

} // namespace Dialogs
