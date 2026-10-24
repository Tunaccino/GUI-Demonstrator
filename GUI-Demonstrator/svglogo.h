#pragma once

#include <QWidget>

class QSvgRenderer;

/**
 * Zeigt ein Logo oder eine Wortmarke aus einer SVG-Datei.
 *
 * Die Grafik wird nie nachgezeichnet, sondern immer aus der Datei gerendert,
 * und zwar als Vektor: Sie bleibt in jeder Groesse scharf. Die Breite folgt
 * aus der gewuenschten Hoehe und dem Seitenverhaeltnis der Datei.
 *
 * Fehlt die Datei, bleibt das Widget leer und nimmt keinen Platz ein.
 */
class SvgLogo : public QWidget
{
    Q_OBJECT

public:
    explicit SvgLogo(QWidget *parent = nullptr);

    /** Pfad zur SVG-Datei, auch aus den Ressourcen (":/logo/..."). */
    bool setSource(const QString &path);

    /** Hoehe in Pixeln; die Breite ergibt sich aus dem Seitenverhaeltnis. */
    void setLogoHeight(int height);

    /** Barrierefreie Beschreibung, z. B. "DARE ROBOTICS". */
    void setDescription(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateSize();

    QSvgRenderer *m_renderer;
    int m_height = 32;
};
