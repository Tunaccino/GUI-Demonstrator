#pragma once

#include <QObject>
#include <QTimer>

/**
 * Simulierter Probelauf ohne angeschlossene Hardware.
 *
 * Modelliert eine traege Achse (Masse + Reibung) mit PID-Regler und liefert
 * im Takt von 50 Hz Messwerte, wie sie auch die echte Regelung liefern wuerde:
 * Motor-Encoder (feine Aufloesung, leichtes Rauschen), Getriebe-Encoder
 * (grobe Aufloesung, Totgang) und die daraus fusionierte Position.
 */
class DemoRun : public QObject
{
    Q_OBJECT

public:
    explicit DemoRun(QObject *parent = nullptr);

    void start(double goalDeg, double kp, double ki, double kd);
    void stop();
    bool isRunning() const { return m_timer.isActive(); }

signals:
    /** Ein Messpunkt. Winkel in Grad, Geschwindigkeit in Grad/s, Beschleunigung in Grad/s^2. */
    void sample(double t, double motorDeg, double gearDeg, double positionDeg,
                double velocity, double acceleration);

    void started();
    void finished();

private:
    void step();

    QTimer m_timer;

    const double m_dt = 0.02;        // 50 Hz
    const double m_maxAccel = 900.0; // Grad/s^2, Stellgroessenbegrenzung
    const double m_friction = 2.4;   // viskose Reibung
    const double m_backlash = 0.35;  // Getriebespiel in Grad

    double m_goal = 0.0;
    double m_kp = 0.0;
    double m_ki = 0.0;
    double m_kd = 0.0;

    double m_t = 0.0;
    double m_pos = 0.0;
    double m_vel = 0.0;
    double m_accel = 0.0;
    double m_integral = 0.0;
    double m_prevError = 0.0;
    double m_gearPos = 0.0;
    double m_settledFor = 0.0;
};
