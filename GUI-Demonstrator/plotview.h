#pragma once

#include <QElapsedTimer>
#include <QGraphicsView>
#include <QColor>
#include <QPixmap>
#include <QVector>

class QGraphicsScene;

/** Farbsatz eines Schemas, Werte aus der DARE-Farbpalette. */
struct PlotColors
{
    QColor background;
    QColor grid;
    QColor axisText;
    QColor zeroLine;
    QColor goalLine;
    QColor motor;
    QColor gear;
    QColor position;
};

/**
 * Rollender Linienplot fuer die Reglervisualisierung.
 *
 * Zeichnet direkt in die QGraphicsScene - keine Fremdbibliothek noetig.
 * Angezeigt werden: Startlinie bei 0.0, Ziellinie, sowie die Verlaeufe von
 * Motor-Encoder, Getriebe-Encoder und aktueller Position.
 *
 * Alle Werte werden in Grad erwartet. Encoder-Rohwerte (Ticks) bitte vorher
 * umrechnen, sonst liegen die Kurven um Groessenordnungen auseinander.
 */
class PlotView : public QGraphicsView
{
    Q_OBJECT

public:
    /** Farbschema. Eloxal = dunkler Foliengrund, Kapton = goldener Grund. */
    enum class Theme { Eloxal, Kapton };

    explicit PlotView(QWidget *parent = nullptr);

    void setTheme(Theme theme);

    /** Zielposition in Grad; wird als gestrichelte Linie gezeichnet. */
    void setGoal(double degrees);

    /** Sichtbares Zeitfenster in Sekunden. 0 = gesamten Verlauf zeigen. */
    void setTimeWindow(double seconds);

    /** Neuen Messpunkt anhaengen. t in Sekunden seit Start. */
    void appendSample(double t, double motorDeg, double gearDeg, double positionDeg);

    /** Alle Messpunkte verwerfen. */
    void clearData();

    /** Logo, das blass hinter dem Plot liegt. Leeres Pixmap schaltet es ab. */
    void setWatermark(const QPixmap &pixmap);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Sample
    {
        double t = 0.0;
        double motor = 0.0;
        double gear = 0.0;
        double pos = 0.0;
    };

    const PlotColors &colors() const;
    void rebuild();
    void computeRanges();
    void drawGrid();
    void drawWatermark();
    void drawReferenceLines();
    void drawSeries();
    void drawLegend();

    QPointF toScene(double t, double value) const;
    void addSeriesPath(double Sample::*member, const QColor &color, qreal width);

    Theme m_theme = Theme::Eloxal;

    QPixmap m_watermark;
    QPixmap m_watermarkScaled;

    QGraphicsScene *m_scene = nullptr;
    QVector<Sample> m_samples;
    QElapsedTimer m_repaintClock;

    double m_goal = 0.0;
    double m_window = 12.0;
    int m_maxSamples = 6000;

    double m_tMin = 0.0;
    double m_tMax = 1.0;
    double m_yMin = -1.0;
    double m_yMax = 1.0;

    QRectF m_plot;
};
