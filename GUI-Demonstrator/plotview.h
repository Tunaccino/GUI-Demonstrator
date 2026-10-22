#pragma once

#include <QElapsedTimer>
#include <QGraphicsView>
#include <QColor>
#include <QPixmap>
#include <QVector>

class QGraphicsScene;

/** Farbsatz fuer den dunklen Foliengrund, Werte aus der DARE-Farbpalette. */
struct PlotColors
{
    QColor background;
    QColor grid;
    QColor axisText;
    QColor trackEnd;
    QColor setpoint;
    QColor ball;
};

/**
 * Rollender Linienplot fuer die Ballposition auf der Bahn.
 *
 * Zeichnet direkt in die QGraphicsScene - keine Fremdbibliothek noetig.
 * Die Y-Achse ist fest auf die Bahn eingestellt (Standard -15 bis +15 cm),
 * damit der Ball immer an derselben Stelle im Bild erscheint. Angezeigt
 * werden: Bahnenden, Sollposition, Verlauf der Ballposition und ein Punkt
 * fuer die aktuelle Lage des Balls.
 */
class PlotView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PlotView(QWidget *parent = nullptr);

    /** Bahnenden; alles dazwischen ist der sichtbare Wertebereich. */
    void setTrack(double minimum, double maximum);

    /** Sollposition; wird als gestrichelte goldene Linie gezeichnet. */
    void setSetpoint(double value);

    /** Einheit fuer Achse und Beschriftung, z. B. "cm". */
    void setUnit(const QString &unit);

    /** Sichtbares Zeitfenster in Sekunden. 0 = gesamten Verlauf zeigen. */
    void setTimeWindow(double seconds);

    /** Neuen Messpunkt anhaengen. t in Sekunden seit Start. */
    void appendSample(double t, double value);

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
        double value = 0.0;
    };

    void rebuild();
    void computeRanges();
    void drawGrid();
    void drawWatermark();
    void drawTrack();
    void drawSetpoint();
    void drawSeries();

    QPointF toScene(double t, double value) const;

    QPixmap m_watermark;
    QPixmap m_watermarkScaled;

    QGraphicsScene *m_scene = nullptr;
    QVector<Sample> m_samples;
    QElapsedTimer m_repaintClock;

    double m_trackMin = -15.0;
    double m_trackMax = 15.0;
    double m_setpoint = 0.0;
    QString m_unit = QStringLiteral("cm");

    double m_window = 12.0;
    int m_maxSamples = 6000;

    double m_tMin = 0.0;
    double m_tMax = 1.0;
    double m_yMin = -1.0;
    double m_yMax = 1.0;

    QRectF m_plot;
};
