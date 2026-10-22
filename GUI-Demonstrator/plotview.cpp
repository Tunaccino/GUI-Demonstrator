#include "plotview.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QResizeEvent>

#include <cmath>

namespace {

// Farben aus der DARE-Farbpalette fuer den dunklen Grund (Eloxal 900).
const PlotColors kColors{
    QColor("#22262D"),             // Eloxal 900, Foliengrund
    QColor(0x53, 0x5A, 0x66, 140), // Eloxal 700, so leise wie das Raster der Folien
    QColor("#9DA4AE"),             // Eloxal 300, Beschriftungen
    QColor("#686F7A"),             // Eloxal 500, Bahnenden
    QColor("#CC9300"),             // Kapton 500, die Marke: Sollposition
    QColor("#FFFFFF"),             // Weiss, Ballposition
};

/** Liefert eine "runde" Schrittweite (1, 2, 5 x 10^n) fuer die Zeitachse. */
double niceStep(double range, int targetTicks)
{
    if (range <= 0.0 || targetTicks < 1)
        return 1.0;

    const double raw = range / targetTicks;
    const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
    const double normalized = raw / magnitude;

    double step = 10.0;
    if (normalized <= 1.0)
        step = 1.0;
    else if (normalized <= 2.0)
        step = 2.0;
    else if (normalized <= 5.0)
        step = 5.0;

    return step * magnitude;
}

QGraphicsSimpleTextItem *makeLabel(QGraphicsScene *scene, const QString &text,
                                   const QColor &color, int pointSizeDelta = -2)
{
    auto *item = scene->addSimpleText(text);
    item->setBrush(color);

    QFont f = item->font();
    f.setPointSize(qMax(7, f.pointSize() + pointSizeDelta));
    item->setFont(f);

    return item;
}

/** Zahl mit Vorzeichen, wie man es fuer eine Lage um die Mitte erwartet. */
QString signedNumber(double value, int decimals)
{
    if (std::abs(value) < 0.5 * std::pow(10.0, -decimals))
        value = 0.0;
    QString text = QString::number(value, 'f', decimals);
    if (value > 0.0)
        text.prepend(QLatin1Char('+'));
    else if (value < 0.0)
        text.replace(0, 1, QChar(0x2212)); // echtes Minuszeichen
    return text;
}

} // namespace

PlotView::PlotView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setBackgroundBrush(kColors.background);
    setCacheMode(QGraphicsView::CacheNone);

    m_repaintClock.start();
    rebuild();
}

void PlotView::setWatermark(const QPixmap &pixmap)
{
    m_watermark = pixmap;
    m_watermarkScaled = QPixmap();
    rebuild();
}

void PlotView::setTrack(double minimum, double maximum)
{
    m_trackMin = qMin(minimum, maximum);
    m_trackMax = qMax(minimum, maximum);
    rebuild();
}

void PlotView::setSetpoint(double value)
{
    m_setpoint = value;
    rebuild();
}

void PlotView::setUnit(const QString &unit)
{
    m_unit = unit;
    rebuild();
}

void PlotView::setTimeWindow(double seconds)
{
    m_window = qMax(0.0, seconds);
    rebuild();
}

void PlotView::clearData()
{
    m_samples.clear();
    rebuild();
}

void PlotView::appendSample(double t, double value)
{
    m_samples.append(Sample{t, value});

    // Ringpuffer: bei Dauerbetrieb nicht unbegrenzt wachsen lassen.
    if (m_samples.size() > m_maxSamples)
        m_samples.remove(0, m_samples.size() - m_maxSamples);

    // Neuzeichnen auf ~60 Hz begrenzen. Bei 1 kHz Reglertakt wuerde sonst
    // jeder einzelne Messwert einen kompletten Szenenaufbau ausloesen.
    if (m_repaintClock.elapsed() < 16)
        return;

    m_repaintClock.restart();
    rebuild();
}

void PlotView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    rebuild();
}

QPointF PlotView::toScene(double t, double value) const
{
    const double tSpan = qMax(1e-9, m_tMax - m_tMin);
    const double ySpan = qMax(1e-9, m_yMax - m_yMin);

    // Ausreisser (Sensorrauschen am Bahnende) bleiben im Bild.
    value = qBound(m_yMin, value, m_yMax);

    const double x = m_plot.left() + (t - m_tMin) / tSpan * m_plot.width();
    const double y = m_plot.bottom() - (value - m_yMin) / ySpan * m_plot.height();

    return QPointF(x, y);
}

void PlotView::computeRanges()
{
    if (m_samples.isEmpty()) {
        m_tMin = 0.0;
        m_tMax = (m_window > 0.0) ? m_window : 10.0;
    } else {
        m_tMax = m_samples.constLast().t;
        m_tMin = (m_window > 0.0) ? qMax(m_samples.constFirst().t, m_tMax - m_window)
                                  : m_samples.constFirst().t;

        if (m_tMax - m_tMin < 1e-6)
            m_tMax = m_tMin + (m_window > 0.0 ? m_window : 1.0);
    }

    // Fester Wertebereich: die Bahn plus ein schmaler Streifen jenseits der
    // Enden, damit man sieht, wo die Bahn aufhoert.
    const double span = qMax(1e-6, m_trackMax - m_trackMin);
    const double margin = span * 0.08;
    m_yMin = m_trackMin - margin;
    m_yMax = m_trackMax + margin;
}

void PlotView::drawWatermark()
{
    if (m_watermark.isNull() || m_watermark.height() <= 0)
        return;

    // Die Bildmarke hat keinen Schriftzug, der aufbrechen koennte, und darf
    // deshalb gross werden: drei Viertel der Plothoehe, hoechstens 80 % der
    // Breite.
    const double aspect = double(m_watermark.width()) / m_watermark.height();
    const int target = qMin(qRound(m_plot.height() * 0.75 * aspect), qRound(m_plot.width() * 0.8));
    if (target < 40)
        return;

    // Skalieren ist teuer, deshalb nur bei geaenderter Groesse.
    if (m_watermarkScaled.width() != target)
        m_watermarkScaled = m_watermark.scaledToWidth(target, Qt::SmoothTransformation);

    // Auf dem dunklen Grund wirkt Weiss schnell zu laut, daher sehr blass.
    auto *item = m_scene->addPixmap(m_watermarkScaled);
    item->setOpacity(0.06);
    item->setPos(m_plot.center()
                 - QPointF(m_watermarkScaled.width() / 2.0,
                           m_watermarkScaled.height() / 2.0));
}

void PlotView::drawGrid()
{
    const QPen gridPen(kColors.grid, 1.0, Qt::SolidLine);

    // Waagerechte Linien: die Bahn in sechs gleiche Abschnitte, bei 30 cm
    // also alle 5 cm. So liegen beide Bahnenden und die Mitte auf Linien.
    const double yStep = (m_trackMax - m_trackMin) / 6.0;
    for (int i = 0; i <= 6; ++i) {
        const double v = m_trackMin + i * yStep;
        const QPointF p = toScene(m_tMin, v);
        m_scene->addLine(m_plot.left(), p.y(), m_plot.right(), p.y(), gridPen);

        auto *label = makeLabel(m_scene, signedNumber(v, 1), kColors.axisText);
        label->setPos(m_plot.left() - label->boundingRect().width() - 8,
                      p.y() - label->boundingRect().height() / 2.0);
    }

    // Einheit ueber der Y-Achse
    auto *unit = makeLabel(m_scene, m_unit, kColors.axisText);
    unit->setPos(m_plot.left() - unit->boundingRect().width() - 8,
                 m_plot.top() - unit->boundingRect().height() - 4);

    // Senkrechte Linien + Zeitachse
    const double tStep = niceStep(m_tMax - m_tMin, 6);
    const double tFirst = std::ceil(m_tMin / tStep) * tStep;

    for (double t = tFirst; t <= m_tMax + 1e-9; t += tStep) {
        const QPointF p = toScene(t, m_yMin);
        m_scene->addLine(p.x(), m_plot.top(), p.x(), m_plot.bottom(), gridPen);

        auto *label = makeLabel(m_scene, QString::number(t, 'f', tStep < 1.0 ? 1 : 0) + " s",
                                kColors.axisText);
        label->setPos(p.x() - label->boundingRect().width() / 2.0, m_plot.bottom() + 6);
    }

    // Rahmen der Plotflaeche
    m_scene->addRect(m_plot, QPen(kColors.grid, 1.0), Qt::NoBrush);
}

void PlotView::drawTrack()
{
    // Jenseits der Bahnenden schraffiert, wie eine Sperrflaeche in einer
    // technischen Zeichnung.
    QColor hatch = kColors.trackEnd;
    hatch.setAlpha(110);
    const QBrush hatchBrush(hatch, Qt::BDiagPattern);

    const QPointF top = toScene(m_tMin, m_trackMax);
    const QPointF bottom = toScene(m_tMin, m_trackMin);

    m_scene->addRect(QRectF(m_plot.left(), m_plot.top(), m_plot.width(), top.y() - m_plot.top()),
                     Qt::NoPen, hatchBrush);
    m_scene->addRect(QRectF(m_plot.left(), bottom.y(), m_plot.width(), m_plot.bottom() - bottom.y()),
                     Qt::NoPen, hatchBrush);

    const QPen endPen(kColors.trackEnd, 1.4);
    m_scene->addLine(m_plot.left(), top.y(), m_plot.right(), top.y(), endPen);
    m_scene->addLine(m_plot.left(), bottom.y(), m_plot.right(), bottom.y(), endPen);

    auto *label = makeLabel(m_scene, QStringLiteral("Bahnende"), kColors.axisText);
    label->setPos(m_plot.right() - label->boundingRect().width() - 6,
                  top.y() + 3);
}

void PlotView::drawSetpoint()
{
    const QPointF p = toScene(m_tMin, m_setpoint);

    QPen pen(kColors.setpoint, 1.4, Qt::DashLine);
    pen.setDashPattern({6, 5});
    m_scene->addLine(m_plot.left(), p.y(), m_plot.right(), p.y(), pen);

    auto *label = makeLabel(m_scene,
                            QStringLiteral("Soll %1 %2").arg(signedNumber(m_setpoint, 1), m_unit),
                            kColors.setpoint);
    label->setPos(m_plot.right() - label->boundingRect().width() - 6,
                  p.y() - label->boundingRect().height() - 2);
}

void PlotView::drawSeries()
{
    if (m_samples.isEmpty()) {
        // Leerer Plot soll nicht kaputt aussehen: kurz sagen, worauf er wartet.
        // Mittig zwischen Sollinie und unterem Bahnende, damit nichts kollidiert.
        auto *hint = makeLabel(m_scene, QStringLiteral("Noch keine Messwerte"), kColors.axisText, 0);
        const QRectF box = hint->boundingRect();
        const QPointF anchor = toScene(m_tMin, (m_setpoint + m_trackMin) / 2.0);
        hint->setPos(m_plot.center().x() - box.width() / 2.0, anchor.y() - box.height() / 2.0);
        return;
    }

    QPainterPath path;
    bool started = false;

    for (int i = 0; i < m_samples.size(); ++i) {
        const Sample &s = m_samples.at(i);

        // Einen Punkt vor dem Fenster mitnehmen, damit die Linie am linken
        // Rand nicht abreisst.
        if (s.t < m_tMin && !(i + 1 < m_samples.size() && m_samples.at(i + 1).t >= m_tMin))
            continue;

        QPointF p = toScene(s.t, s.value);
        p.setX(qMax(p.x(), m_plot.left()));

        if (!started) {
            path.moveTo(p);
            started = true;
        } else {
            path.lineTo(p);
        }
    }

    if (!started)
        return;

    QPen pen(kColors.ball, 2.0);
    pen.setCosmetic(true);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    m_scene->addPath(path, pen);

    // Der Ball selbst: ein Punkt am Ende der Kurve, dort wo er gerade liegt.
    const QPointF head = toScene(m_samples.constLast().t, m_samples.constLast().value);
    constexpr qreal r = 5.5;
    m_scene->addEllipse(QRectF(head.x() - r, head.y() - r, 2 * r, 2 * r),
                        QPen(kColors.background, 2.0), QBrush(kColors.ball));
}

void PlotView::rebuild()
{
    m_scene->clear();

    const QRectF viewRect(QPointF(0, 0), viewport()->size());
    m_scene->setSceneRect(viewRect);
    setSceneRect(viewRect);

    if (viewRect.width() < 120 || viewRect.height() < 80)
        return;

    // Raender: links Y-Beschriftung, oben die Einheit, unten die Zeitachse,
    // rechts Platz fuer den Ballpunkt.
    m_plot = viewRect.adjusted(58, 26, -18, -28);
    if (m_plot.width() < 40 || m_plot.height() < 40)
        return;

    computeRanges();
    drawWatermark();
    drawGrid();
    drawTrack();
    drawSetpoint();
    drawSeries();
}
