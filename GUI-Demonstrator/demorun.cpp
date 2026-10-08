#include "demorun.h"

#include <QRandomGenerator>

#include <cmath>

namespace {

double clamp(double v, double lo, double hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/** Gleichverteiltes Rauschen im Bereich +/- amplitude. */
double noise(double amplitude)
{
    return (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0) * amplitude;
}

/** Bildet einen Winkel auf die Aufloesung eines Encoders ab. */
double quantize(double value, double resolution)
{
    return std::round(value / resolution) * resolution;
}

} // namespace

DemoRun::DemoRun(QObject *parent)
    : QObject(parent)
{
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(static_cast<int>(m_dt * 1000.0));
    connect(&m_timer, &QTimer::timeout, this, &DemoRun::step);
}

void DemoRun::start(double goalDeg, double kp, double ki, double kd)
{
    m_goal = goalDeg;
    m_kp = kp;
    m_ki = ki;
    m_kd = kd;

    m_t = 0.0;
    m_pos = 0.0;
    m_vel = 0.0;
    m_accel = 0.0;
    m_integral = 0.0;
    m_prevError = goalDeg;
    m_gearPos = 0.0;
    m_settledFor = 0.0;

    emit started();
    emit sample(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    m_timer.start();
}

void DemoRun::stop()
{
    if (m_timer.isActive()) {
        m_timer.stop();
        emit finished();
    }
}

void DemoRun::step()
{
    const double error = m_goal - m_pos;
    const double derivative = (error - m_prevError) / m_dt;
    m_prevError = error;

    // Anti-Windup per bedingter Integration: solange die Stellgroesse in der
    // Begrenzung haengt, wird nicht weiter aufintegriert. Ohne das laeuft der
    // I-Anteil waehrend der Beschleunigungsphase voll und erzeugt ein
    // Ueberschwingen, das erst nach kp/ki Sekunden wieder abgebaut ist.
    const double candidate = m_integral + error * m_dt;
    const double unsaturated = m_kp * error + m_ki * candidate + m_kd * derivative;
    const double command = clamp(unsaturated, -m_maxAccel, m_maxAccel);

    if (std::abs(unsaturated - command) < 1e-9)
        m_integral = candidate;

    m_accel = command - m_friction * m_vel;
    m_vel += m_accel * m_dt;
    m_pos += m_vel * m_dt;
    m_t += m_dt;

    // Motorseitiger Encoder: hohe Aufloesung, etwas Rauschen.
    const double motorDeg = quantize(m_pos, 0.01) + noise(0.02);

    // Abtriebsseitiger Encoder: folgt dem Motor erst nach dem Totgang und
    // hat eine deutlich groebere Aufloesung.
    if (m_pos - m_gearPos > m_backlash)
        m_gearPos = m_pos - m_backlash;
    else if (m_gearPos - m_pos > m_backlash)
        m_gearPos = m_pos + m_backlash;

    const double gearDeg = quantize(m_gearPos, 0.25);

    // Fusion beider Quellen - das ist die angezeigte "Current Position".
    const double positionDeg = 0.7 * motorDeg + 0.3 * gearDeg;

    emit sample(m_t, motorDeg, gearDeg, positionDeg, m_vel, m_accel);

    // Eingeschwungen? Dann noch kurz weiterlaufen und beenden.
    if (std::abs(error) < 0.3 && std::abs(m_vel) < 0.5)
        m_settledFor += m_dt;
    else
        m_settledFor = 0.0;

    if (m_settledFor > 1.5 || m_t > 30.0)
        stop();
}
