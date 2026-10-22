#pragma once

#include <QWidget>

/**
 * Halbkreis-Zifferblatt fuer einen Motorwinkel, der direkt (ohne Regelung)
 * an einen Motor geht. Gestaltet nach dem Kompass auf der Titelfolie:
 * Skala mit Teilstrichen, goldener Schraegstrich als Zeiger.
 *
 * 0 Grad liegt oben, positive Winkel rechts, negative links. So dreht der
 * Zeiger in dieselbe Richtung wie der Drehgeber.
 *
 * Bedienung:
 *   Drehgeber 4        -> ueber Controller::turnKnob (stepBy)
 *   Ziehen mit der Maus -> Winkel direkt zeigen
 *   Mausrad            -> 1 Grad, mit Strg 10 Grad
 *   Pfeiltasten        -> 1 Grad, Bild auf/ab 10 Grad
 *   Doppelklick, Pos1  -> zurueck auf 0 Grad
 */
class MotorDial : public QWidget
{
    Q_OBJECT

public:
    explicit MotorDial(QWidget *parent = nullptr);

    double value() const { return m_value; }
    double minimum() const { return m_min; }
    double maximum() const { return m_max; }

    void setRange(double minimum, double maximum);

    /** Um delta Grad verstellen (Drehgeber, Tasten, Mausrad). */
    void stepBy(double delta);

    /** Grobmodus anzeigen (x10 nach Druck auf den Drehgeber). */
    void setCoarse(bool coarse);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void setValue(double value);

signals:
    /** Neuer Winkel in Grad; wird sofort an den Motor weitergegeben. */
    void valueChanged(double degrees);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    struct Geometry {
        QPointF pivot;
        qreal radius;
    };
    Geometry geometry() const;
    double valueAt(const QPointF &pos) const;

    double m_value = 0.0;
    double m_min = -90.0;
    double m_max = 90.0;
    bool m_coarse = false;
};
