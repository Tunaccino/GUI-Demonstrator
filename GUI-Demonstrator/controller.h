#pragma once

#include <QDialog>
#include <QElapsedTimer>
#include <QList>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class dialog;
}
QT_END_NAMESPACE

class BoardConnection;
class KnobInput;
class MotorDial;
class QLabel;
class QLineEdit;

class Controller : public QDialog
{
    Q_OBJECT

public:
    /** lowerPort/upperPort: serielle Ports von Schrittmotor- und BLDC-Board. */
    Controller(const QString &lowerPort, const QString &upperPort, QWidget *parent = nullptr);
    ~Controller() override;

public slots:
    /**
     * Ein Messpunkt des Balls. Hier werden die Balldaten eingespeist, sobald
     * die Firmware sie liefert.
     *   t             Sekunden seit Start
     *   ballCm        Ballposition auf der Bahn, 0 = Mitte, -15 ... +15 cm
     *   velocity      Ballgeschwindigkeit, cm/s
     *   acceleration  Ballbeschleunigung, cm/s^2
     */
    void showSample(double t, double ballCm, double velocity, double acceleration);

    /**
     * Die beiden Encoder des Schrittmotor-Gelenks, je -180 ... +180 Grad.
     *   motorDeg      Motorwelle (Motor-Encoder, "indirect")
     *   gearDeg       Gelenk hinter dem Riemen (Getriebe-Encoder, "direct")
     */
    void showEncoders(double motorDeg, double gearDeg);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void applyCorporateFonts();
    void loadBranding();
    void applyPid(bool force = false);
    void applyMotorAngle(double degrees);

    // Verbindung zu den beiden Boards, Status als zusaetzliche Zeilen in der Status-Karte.
    void setupBoards(const QString &lowerPort, const QString &upperPort);
    QLabel *addStatusRow(const QString &caption);

    // Bedienung per Drehgeber: jeder Knopf gehoert fest zu einem Feld.
    void setupKnobs();
    void turnKnob(int index, int steps);
    void pressKnob(int index);

    Ui::dialog *ui;

    struct Knob {
        int clkGpio;
        int swGpio;
        QLineEdit *edit;   // PID-Feld; nullptr beim Zifferblatt
        QLabel *label;     // Beschriftung, zeigt "x10" im Grobmodus; nullptr = keine
        MotorDial *dial;   // Zifferblatt fuer den Motorwinkel; nullptr bei PID
        double step;       // Aenderung pro Rastschritt
        double min;
        double max;
        bool coarse = false;
        QString labelText = {}; // urspruengliche Beschriftung
    };
    QList<Knob> m_knobs;

    QTimer m_pidTimer;          // prueft regelmaessig, ob in einem PID-Feld getippt wurde
    QElapsedTimer m_pidEdited;  // seit dem letzten Tastendruck in einem PID-Feld
    QList<double> m_sentPid;    // zuletzt gesendet: P, I, D

    BoardConnection *m_lower = nullptr; // Schrittmotor: Gelenk am Zifferblatt
    BoardConnection *m_upper = nullptr; // BLDC: kippt das Rohr mit dem Ball
    QLabel *m_lowerStatus = nullptr;
    QLabel *m_upperStatus = nullptr;

    QString m_coordinateFamily; // Oswald Light fuer die Koordinaten am Rand
};
