#pragma once

#include <QDialog>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui {
class dialog;
}
QT_END_NAMESPACE

class KnobInput;
class MotorDial;
class QLabel;
class QLineEdit;

class Controller : public QDialog
{
    Q_OBJECT

public:
    explicit Controller(QWidget *parent = nullptr);
    ~Controller() override;

public slots:
    /**
     * Ein Messpunkt. Genau hier werden die echten Hardwaredaten eingespeist.
     *   t             Sekunden seit Start
     *   motorDeg      Winkel des geregelten Motors (Motor-Encoder), Grad
     *   gearDeg       Winkel am Getriebeausgang (Getriebe-Encoder), Grad
     *   ballCm        Ballposition auf der Bahn, 0 = Mitte, -15 ... +15 cm
     *   velocity      Ballgeschwindigkeit, cm/s
     *   acceleration  Ballbeschleunigung, cm/s^2
     */
    void showSample(double t, double motorDeg, double gearDeg, double ballCm,
                    double velocity, double acceleration);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void applyCorporateFonts();
    void loadBranding();
    void applyPid();
    void applyMotorAngle(double degrees);

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

    QString m_coordinateFamily; // Oswald Light fuer die Koordinaten am Rand
};
