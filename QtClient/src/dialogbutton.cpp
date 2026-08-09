#include "dialogbutton.h"

#include "dialogstyle.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStylePainter>

DialogButton::DialogButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent)
{
    parseMnemonic(text);
}

void DialogButton::setText(const QString& text)
{
    QPushButton::setText(text);
    parseMnemonic(text);
    update();
}

// El '&' marca el mnemonico; "&&" es un ampersand literal. Se recorre a mano en vez de
// usar QKeySequence::mnemonic() porque hace falta el INDICE dentro del texto ya limpio,
// no la tecla.
void DialogButton::parseMnemonic(const QString& raw)
{
    m_plainText.clear();
    m_plainText.reserve(raw.size());
    m_mnemonicIndex = -1;

    for (int i = 0; i < raw.size(); ++i) {
        if (raw.at(i) != QLatin1Char('&')) {
            m_plainText.append(raw.at(i));
            continue;
        }
        if (i + 1 < raw.size() && raw.at(i + 1) == QLatin1Char('&')) {
            m_plainText.append(QLatin1Char('&')); // "&&" => '&' literal
            ++i;
            continue;
        }
        if (i + 1 < raw.size() && m_mnemonicIndex < 0) {
            m_mnemonicIndex = m_plainText.size(); // el proximo caracter es el subrayado
        }
    }
}

void DialogButton::paintEvent(QPaintEvent* /*event*/)
{
    QStylePainter painter(this);
    QStyleOptionButton opt;
    initStyleOption(&opt);

    // Primero el bezel SIN texto: si lo dibujara el estilo, en Windows quedaria un
    // subrayado nativo ademas del que pintamos abajo, y en mac ninguno.
    const QString label = m_plainText;
    opt.text.clear();
    painter.drawControl(QStyle::CE_PushButton, opt);

    if (label.isEmpty()) {
        return;
    }

    // El rect de contenido lo resuelve el estilo, asi que respeta el padding que venga
    // del stylesheet (el borde violeta de la marca lo agrega).
    const QRect contents = style()->subElementRect(QStyle::SE_PushButtonContents, &opt, this);
    const QFontMetrics metrics(font());
    const int textWidth = metrics.horizontalAdvance(label);
    const int x = contents.x() + (contents.width() - textWidth) / 2;
    const int baseline = contents.y() + (contents.height() + metrics.ascent() - metrics.descent()) / 2;

    // El color NO sale de la paleta del tema: como el texto lo dibuja este widget y no el
    // stylesheet, tiene que tomarlo del mismo lugar centralizado que el resto del look.
    painter.setPen(QColor(QLatin1String(isEnabled() ? DialogStyle::kButtonTextColor
                                                    : DialogStyle::kButtonTextColorDisabled)));
    painter.drawText(x, baseline, label);

    if (m_mnemonicIndex < 0 || m_mnemonicIndex >= label.size()) {
        return;
    }
    const int underlineX = x + metrics.horizontalAdvance(label.left(m_mnemonicIndex));
    const int underlineW = metrics.horizontalAdvance(label.mid(m_mnemonicIndex, 1));
    const int underlineY = baseline + metrics.underlinePos();
    painter.drawLine(underlineX, underlineY, underlineX + underlineW, underlineY);
}
