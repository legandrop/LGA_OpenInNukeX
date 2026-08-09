#pragma once

#include <QPushButton>
#include <QString>

/**
 * Boton de los dialogos de eleccion, con el subrayado del mnemonico visible en TODAS las
 * plataformas.
 *
 * El problema: Qt en macOS NO dibuja el subrayado del '&'. La letra se consume igual (el
 * texto se ve sin el '&') pero no queda subrayada, asi que el usuario no tiene forma de
 * saber que existe el atajo. En Windows si se subraya. Resultado: el mismo dialogo se ve
 * distinto en cada plataforma.
 *
 * La solucion: no dejar que el estilo dibuje el texto. Se le pide que pinte el bezel con
 * `opt.text` vacio y despues se dibuja el label a mano con QPainter, subrayando el
 * caracter del mnemonico. Un solo subrayado, igual en mac y en Windows.
 */
class DialogButton : public QPushButton
{
    Q_OBJECT

public:
    /**
     * `text` va con el '&' adelante de la letra del atajo, igual que en Qt: "&Yes"
     * subraya la Y. Un "&&" literal escapa al ampersand.
     */
    explicit DialogButton(const QString& text, QWidget* parent = nullptr);

    void setText(const QString& text); // no es virtual en QPushButton: oculta a proposito

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    // Parsea `raw` y deja el texto sin '&' en m_plainText y la posicion del mnemonico
    // (dentro de m_plainText) en m_mnemonicIndex; -1 si no hay.
    void parseMnemonic(const QString& raw);

    QString m_plainText;
    int m_mnemonicIndex = -1;
};
