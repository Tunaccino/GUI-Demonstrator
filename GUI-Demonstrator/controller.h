#pragma once

#include <QDialog>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui {
class dialog;
}
QT_END_NAMESPACE

class DemoRun;
class KnobInput;
class QLabel;
class QLineEdit;

class Controller : public QDialog
{
    Q_OBJECT

public:
    explicit Controller(QWidget *parent = nullptr);
    ~Controller() override;

private:
    void applyCorporateFonts();
    void loadLogo();
    void applyGoal();
    void resetAll();
    void toggleDemo();
    void showSample(double t, double motorDeg, double gearDeg, double positionDeg,
                    double velocity, double acceleration);

    // Bedienung per Drehgeber: jeder Knopf gehoert fest zu einem Feld.
    void setupKnobs();
    void turnKnob(int index, int steps);
    void pressKnob(int index);

    Ui::dialog *ui;
    DemoRun *m_demo;

    struct Knob {
        int clkGpio;
        int swGpio;
        QLineEdit *edit;
        QLabel *label;     // Beschriftung, zeigt "x10" im Grobmodus; nullptr = keine
        double step;       // Aenderung pro Rastschritt
        double min;
        double max;
        bool coarse = false;
        QString labelText; // urspruengliche Beschriftung
    };
    QList<Knob> m_knobs;
};
