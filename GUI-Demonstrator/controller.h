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

    // Bedienung per Drehgeber: Druecken waehlt das naechste Feld,
    // Drehen aendert dessen Wert.
    void setupKnob();
    void selectNextKnobField();
    void adjustKnobField(int steps);
    void syncKnobIndexToFocus();

    Ui::dialog *ui;
    DemoRun *m_demo;

    struct KnobField {
        QLineEdit *edit;
        double step; // Aenderung pro Rastschritt
        double min;
        double max;
    };
    QList<KnobField> m_knobFields;
    int m_knobIndex = 0;
    KnobInput *m_knob;
};
