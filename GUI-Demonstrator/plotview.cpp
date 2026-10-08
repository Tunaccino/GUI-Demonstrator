#include "plotview.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QResizeEvent>

#include <cmath>

namespace {

// Farben aus der DARE-Farbpalette, jeweils fuer den Zweck, den die Palette
// ihnen zuweist. Welcher Satz gilt, haengt vom Grund ab: auf Eloxal 900 tragen
// nur die hellen Stufen, auf Kapton 050 nur die dunklen.
const PlotColors kEloxalColors{
    QColor("#22262D"), // Eloxal 900, Foliengrund
    QColor("#535A66"), // Eloxal 700
    QColor("#9DA4AE"), // Eloxal 300
    QColor("#686F7A"), // Eloxal 500
    QColor("#CC9300"), // Kapton 500, die Marke
    QColor("#E3B64D"), // Kapton 300, zweite Datenreihe
    QColor("#9DA4AE"), // Eloxal 300
    QColor("#FFFFFF"), // Weiss, Hauptkurve
};

const PlotColors kKaptonColors{
    QColor("#F6E4BB"), // Kapton 100, hinterlegte Flaeche
    QColor("#E3B64D"), // Kapton 300, Gitter - bewusst leise
    QColor("#686F7A"), // Eloxal 500, Beschriftungen
    QColor("#9DA4AE"), // Eloxal 300, Linien und Inaktives
    QColor("#8A6300"), // Kapton 700, goldener Text auf hellem Grund
    QColor("#A87A00"), // Kapton 600, grosse Zahlen und Diagramme
    QColor("#535A66"), // Eloxal 700
    QColor("#22262D"), // Eloxal 900, Hauptkurve
};

/** Liefert eine "runde" Schrittweite (1, 2, 5 x 10^n) fuer die Achsbeschriftung. */
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

} // namespace

const PlotColors &PlotView::colors() const
{
    return m_theme == Theme::Kapton ? kKaptonColors : kEloxalColors;
}

void PlotView::setTheme(Theme theme)
{
    if (m_theme == theme)
        return;

    m_theme = theme;
    setBackgroundBrush(colors().background);
    rebuild();
}

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
    setBackgroundBrush(colors().background);
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

void PlotView::setGoal(double degrees)
{
    if (qFuzzyCompare(m_goal, degrees))
        return;

    m_goal = degrees;
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

void PlotView::appendSample(double t, double motorDeg, double gearDeg, double positionDeg)
{
    m_samples.append(Sample{t, motorDeg, gearDeg, positionDeg});

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

    // Startposition (0.0) und Ziel sind immer im Bild.
    double lo = qMin(0.0, m_goal);
    double hi = qMax(0.0, m_goal);

    for (const Sample &s : m_samples) {
        if (s.t < m_tMin)
            continue;

        lo = qMin(lo, qMin(s.pos, qMin(s.motor, s.gear)));
        hi = qMax(hi, qMax(s.pos, qMax(s.motor, s.gear)));
    }

    double span = hi - lo;
    if (span < 1e-6)
        span = 1.0;

    const double padding = span * 0.12;
    m_yMin = lo - padding;
    m_yMax = hi + padding;
}

void PlotView::drawWatermark()
{
    if (m_watermark.isNull())
        return;

    // Mindestgroesse aus dem Logo Guide: unter 240 px Breite bricht die
    // ROBOTICS-Subline auf. Passt das nicht in die Plotflaeche, bleibt das
    // Wasserzeichen lieber ganz weg.
    constexpr int kMinWidth = 240;

    const int target = qMax(kMinWidth, qRound(m_plot.width() * 0.5));
    if (target > m_plot.width() * 0.8)
        return;

    // Skalieren ist teuer, deshalb nur bei geaenderter Groesse.
    if (m_watermarkScaled.width() != target)
        m_watermarkScaled = m_watermark.scaledToWidth(target, Qt::SmoothTransformation);

    auto *item = m_scene->addPixmap(m_watermarkScaled);
    item->setOpacity(0.10);
    item->setPos(m_plot.center()
                 - QPointF(m_watermarkScaled.width() / 2.0,
                           m_watermarkScaled.height() / 2.0));
}

void PlotView::drawGrid()
{
    const QPen gridPen(colors().grid, 1.0, Qt::SolidLine);

    // Waagerechte Linien + Y-Beschriftung
    const double yStep = niceStep(m_yMax - m_yMin, 5);
    const double yFirst = std::ceil(m_yMin / yStep) * yStep;

    for (double v = yFirst; v <= m_yMax + 1e-9; v += yStep) {
        const QPointF p = toScene(m_tMin, v);
        m_scene->addLine(m_plot.left(), p.y(), m_plot.right(), p.y(), gridPen);

        auto *label = makeLabel(m_scene, QString::number(v, 'f', yStep < 1.0 ? 2 : 1), colors().axisText);
        label->setPos(m_plot.left() - label->boundingRect().width() - 8,
                      p.y() - label->boundingRect().height() / 2.0);
    }

    // Senkrechte Linien + Zeitachse
    const double tStep = niceStep(m_tMax - m_tMin, 6);
    const double tFirst = std::ceil(m_tMin / tStep) * tStep;

    for (double t = tFirst; t <= m_tMax + 1e-9; t += tStep) {
        const QPointF p = toScene(t, m_yMin);
        m_scene->addLine(p.x(), m_plot.top(), p.x(), m_plot.bottom(), gridPen);

        auto *label = makeLabel(m_scene, QString::number(t, 'f', tStep < 1.0 ? 1 : 0) + " s", colors().axisText);
        label->setPos(p.x() - label->boundingRect().width() / 2.0, m_plot.bottom() + 6);
    }

    // Rahmen der Plotflaeche
    m_scene->addRect(m_plot, QPen(colors().grid, 1.0), Qt::NoBrush);
}

void PlotView::drawReferenceLines()
{
    // Startposition
    const QPointF zero = toScene(m_tMin, 0.0);
    if (zero.y() >= m_plot.top() && zero.y() <= m_plot.bottom()) {
        m_scene->addLine(m_plot.left(), zero.y(), m_plot.right(), zero.y(),
                         QPen(colors().zeroLine, 1.4, Qt::SolidLine));

        auto *label = makeLabel(m_scene, QStringLiteral("Start 0.0\u00B0"), colors().zeroLine);
        label->setPos(m_plot.left() + 6, zero.y() - label->boundingRect().height() - 2);
    }

    // Zielposition
    const QPointF goal = toScene(m_tMin, m_goal);
    if (goal.y() >= m_plot.top() && goal.y() <= m_plot.bottom()) {
        QPen goalPen(colors().goalLine, 1.4, Qt::DashLine);
        goalPen.setDashPattern({6, 5});
        m_scene->addLine(m_plot.left(), goal.y(), m_plot.right(), goal.y(), goalPen);

        auto *label = makeLabel(m_scene,
                                QStringLiteral("Goal %1\u00B0").arg(m_goal, 0, 'f', 1),
                                colors().goalLine);
        label->setPos(m_plot.right() - label->boundingRect().width() - 6,
                      goal.y() - label->boundingRect().height() - 2);
    }
}

void PlotView::addSeriesPath(double Sample::*member, const QColor &color, qreal width)
{
    QPainterPath path;
    bool started = false;

    for (int i = 0; i < m_samples.size(); ++i) {
        const Sample &s = m_samples.at(i);

        // Einen Punkt vor dem Fenster mitnehmen, damit die Linie am linken
        // Rand nicht abreisst.
        if (s.t < m_tMin && !(i + 1 < m_samples.size() && m_samples.at(i + 1).t >= m_tMin))
            continue;

        const QPointF p = toScene(s.t, s.*member);

        if (!started) {
            path.moveTo(p);
            started = true;
        } else {
            path.lineTo(p);
        }
    }

    if (!started)
        return;

    QPen pen(color, width);
    pen.setCosmetic(true);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);

    auto *item = m_scene->addPath(path, pen);
    item->setFlag(QGraphicsItem::ItemClipsToShape, false);
}

void PlotView::drawSeries()
{
    addSeriesPath(&Sample::motor, colors().motor, 1.4);
    addSeriesPath(&Sample::gear, colors().gear, 1.4);
    addSeriesPath(&Sample::pos, colors().position, 2.0);
}

void PlotView::drawLegend()
{
    struct Entry
    {
        QString text;
        QColor color;
    };

    const QVector<Entry> entries = {
        {QStringLiteral("Current Position"), colors().position},
        {QStringLiteral("Motor Encoder"), colors().motor},
        {QStringLiteral("Gear Encoder"), colors().gear},
    };

    qreal x = m_plot.left();
    const qreal y = m_plot.bottom() + 24;

    for (const Entry &e : entries) {
        m_scene->addLine(x, y + 6, x + 16, y + 6, QPen(e.color, 2.0));

        auto *label = makeLabel(m_scene, e.text, colors().axisText);
        label->setPos(x + 22, y);

        x += 22 + label->boundingRect().width() + 20;
    }
}

void PlotView::rebuild()
{
    m_scene->clear();

    const QRectF viewRect(QPointF(0, 0), viewport()->size());
    m_scene->setSceneRect(viewRect);
    setSceneRect(viewRect);

    if (viewRect.width() < 120 || viewRect.height() < 80)
        return;

    // Raender: links Y-Beschriftung, unten Zeitachse + Legende.
    m_plot = viewRect.adjusted(56, 16, -16, -48);
    if (m_plot.width() < 40 || m_plot.height() < 40)
        return;

    computeRanges();
    drawWatermark();
    drawGrid();
    drawReferenceLines();
    drawSeries();
    drawLegend();
}
