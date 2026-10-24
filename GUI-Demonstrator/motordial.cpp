#include "motordial.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <cmath>

namespace {

// DARE-Farbpalette
const QColor kEloxal700(0x53, 0x5A, 0x66);
const QColor kEloxal500(0x68, 0x6F, 0x7A);
const QColor kEloxal300(0x9D, 0xA4, 0xAE);
const QColor kKapton500(0xCC, 0x93, 0x00);

constexpr double kPi = 3.14159265358979323846;

/** Zahl mit Vorzeichen und echtem Minuszeichen. */
QString signedNumber(double value, int decimals)
{
    if (std::abs(value) < 0.5 * std::pow(10.0, -decimals))
        value = 0.0;
    QString text = QString::number(std::abs(value), 'f', decimals);
    if (value > 0.0)
        text.prepend(QLatin1Char('+'));
    else if (value < 0.0)
        text.prepend(QChar(0x2212));
    return text;
}

/** Punkt auf dem Kreis. Winkel in Grad wie auf dem Zifferblatt: 0 oben, + rechts. */
QPointF onCircle(const QPointF &pivot, qreal radius, double dialDegrees)
{
    const double rad = dialDegrees * kPi / 180.0;
    return pivot + QPointF(radius * std::sin(rad), -radius * std::cos(rad));
}

} // namespace

MotorDial::MotorDial(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(tr("Motorwinkel"));
    setToolTip(tr("Motorwinkel direkt einstellen: ziehen, Mausrad oder Drehgeber 4.\n"
                  "Doppelklick setzt auf 0\u00B0."));
}

QSize MotorDial::sizeHint() const
{
    return QSize(240, 118);
}

QSize MotorDial::minimumSizeHint() const
{
    return QSize(200, 104);
}

void MotorDial::setRange(double minimum, double maximum)
{
    m_min = qMin(minimum, maximum);
    m_max = qMax(minimum, maximum);
    setValue(m_value);
    update();
}

void MotorDial::setValue(double value)
{
    // Auf eine Nachkommastelle runden, so wie der Wert auch angezeigt wird.
    value = std::round(qBound(m_min, value, m_max) * 10.0) / 10.0;
    if (qFuzzyCompare(value + 1000.0, m_value + 1000.0))
        return;

    m_value = value;
    update();
    emit valueChanged(m_value);
}

void MotorDial::stepBy(double delta)
{
    setValue(m_value + delta);
}

void MotorDial::setCoarse(bool coarse)
{
    m_coarse = coarse;
    update();
}

MotorDial::Geometry MotorDial::geometry() const
{
    // Unten bleibt Platz fuer die Endbeschriftungen, seitlich fuer die
    // ueberstehenden Teilstriche.
    constexpr qreal bottomSpace = 16.0;
    constexpr qreal topSpace = 20.0;
    constexpr qreal sideSpace = 14.0;

    const qreal radius = qMax<qreal>(20.0, qMin(width() / 2.0 - sideSpace,
                                                height() - bottomSpace - topSpace));
    const QPointF pivot(width() / 2.0, height() - bottomSpace);
    return {pivot, radius};
}

double MotorDial::valueAt(const QPointF &pos) const
{
    const Geometry g = geometry();
    const QPointF d = pos - g.pivot;

    // Unterhalb der Drehachse zaehlt die Seite, auf der man ist.
    double degrees = std::atan2(d.x(), -d.y()) * 180.0 / kPi;
    if (d.y() > 0.0)
        degrees = d.x() >= 0.0 ? m_max : m_min;

    return qBound(m_min, degrees, m_max);
}

void MotorDial::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const Geometry g = geometry();
    const qreal r = g.radius;
    const bool focused = hasFocus();

    // Skalenbogen. Mit Fokus golden, damit man sieht, was der Drehgeber
    // gerade verstellt.
    const QRectF circle(g.pivot.x() - r, g.pivot.y() - r, 2 * r, 2 * r);
    const double startDial = m_min;
    const double spanDial = m_max - m_min;
    // QPainter::drawArc: 0 = 3 Uhr, gegen den Uhrzeigersinn, in 1/16 Grad.
    auto qtAngle = [](double dial) { return 90.0 - dial; };

    p.setPen(QPen(focused ? kKapton500 : kEloxal700, 1.2));
    p.drawArc(circle, int(qtAngle(startDial + spanDial) * 16), int(spanDial * 16));

    // Teilstriche: alle 5 Grad fein, alle 15 Grad kraeftig, wie auf dem Kompass.
    for (int deg = int(std::ceil(m_min)); deg <= int(std::floor(m_max)); ++deg) {
        if (deg % 5)
            continue;
        const bool major = deg % 15 == 0;
        const qreal inner = r - (major ? 9.0 : 5.0);
        p.setPen(QPen(major ? kEloxal500 : kEloxal700, major ? 1.4 : 1.0));
        p.drawLine(onCircle(g.pivot, inner, deg), onCircle(g.pivot, r, deg));
    }

    // Ausschlag von 0 bis zum aktuellen Winkel als goldener Bogen.
    if (std::abs(m_value) > 0.05) {
        const QRectF band = circle.adjusted(-3, -3, 3, 3);
        p.setPen(QPen(kKapton500, 3.0, Qt::SolidLine, Qt::FlatCap));
        const double from = qMin(0.0, m_value);
        p.drawArc(band, int(qtAngle(from + std::abs(m_value)) * 16), int(std::abs(m_value) * 16));
    }

    // Zeiger: goldener Schraegstrich wie im Kompass der Titelfolie,
    // als Parallelogramm mit der Schraege aus dem Logo.
    auto slash = [&](double dial, qreal from, qreal to, qreal halfWidth, const QColor &color) {
        const double rad = dial * kPi / 180.0;
        const QPointF dir(std::sin(rad), -std::cos(rad));
        const QPointF normal(-dir.y(), dir.x());
        const qreal shear = halfWidth * 0.9; // Schraege wie im Logo
        QPainterPath path;
        path.moveTo(g.pivot + dir * from - normal * halfWidth);
        path.lineTo(g.pivot + dir * from + normal * halfWidth);
        path.lineTo(g.pivot + dir * to + normal * halfWidth + normal * shear);
        path.lineTo(g.pivot + dir * to - normal * halfWidth + normal * shear);
        path.closeSubpath();
        p.fillPath(path, color);
    };
    slash(m_value, r - 22.0, r + 6.0, 3.2, kKapton500);

    // Endbeschriftungen und Null oben, in der Grundschrift. font() traegt
    // Oswald fuer die grosse Zahl (setzt der Controller).
    QFont small = QApplication::font();
    small.setPixelSize(10);
    p.setFont(small);
    p.setPen(kEloxal500);
    const QString minText = signedNumber(m_min, 0);
    const QString maxText = signedNumber(m_max, 0);
    p.drawText(QRectF(g.pivot.x() - r - 20, g.pivot.y() + 2, 40, 14), Qt::AlignCenter, minText);
    p.drawText(QRectF(g.pivot.x() + r - 20, g.pivot.y() + 2, 40, 14), Qt::AlignCenter, maxText);
    p.drawText(QRectF(g.pivot.x() - 20, g.pivot.y() - r - 21, 40, 14), Qt::AlignCenter,
               QStringLiteral("0"));

    // Wert gross in der Mitte, darunter was er bedeutet.
    QFont big = font();
    big.setPixelSize(qBound(20, int(r * 0.36), 34));
    big.setBold(true);
    p.setFont(big);
    p.setPen(Qt::white);
    const QString valueText = signedNumber(m_value, 1) + QStringLiteral("\u00B0");
    const qreal bigHeight = QFontMetricsF(big).height();
    const QRectF valueBox(g.pivot.x() - r, g.pivot.y() - 18 - bigHeight * 0.92, 2 * r, bigHeight);
    p.drawText(valueBox, Qt::AlignHCenter | Qt::AlignBottom, valueText);

    QFont caption = QApplication::font();
    caption.setPixelSize(11);
    p.setFont(caption);
    const QRectF captionBox(g.pivot.x() - r, g.pivot.y() - 17, 2 * r, 15);
    if (m_coarse) {
        p.setPen(kKapton500);
        p.drawText(captionBox, Qt::AlignCenter, tr("grob \u00D710"));
    } else {
        p.setPen(kEloxal300);
        p.drawText(captionBox, Qt::AlignCenter, tr("direkt, ohne Regelung"));
    }

}

void MotorDial::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return QWidget::mousePressEvent(event);
    setFocus(Qt::MouseFocusReason);
    setValue(valueAt(event->position()));
}

void MotorDial::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        setValue(valueAt(event->position()));
}

void MotorDial::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        setValue(0.0);
}

void MotorDial::wheelEvent(QWheelEvent *event)
{
    const int notches = event->angleDelta().y() / 120;
    if (notches == 0)
        return;
    const double step = (event->modifiers() & Qt::ControlModifier) ? 10.0 : 1.0;
    stepBy(notches * step);
    event->accept();
}

void MotorDial::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Right:
    case Qt::Key_Up:
        stepBy(1.0);
        break;
    case Qt::Key_Left:
    case Qt::Key_Down:
        stepBy(-1.0);
        break;
    case Qt::Key_PageUp:
        stepBy(10.0);
        break;
    case Qt::Key_PageDown:
        stepBy(-10.0);
        break;
    case Qt::Key_Home:
        setValue(0.0);
        break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    event->accept();
}
