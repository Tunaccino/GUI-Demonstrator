#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class dialog;
}
QT_END_NAMESPACE

class DemoRun;

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

    Ui::dialog *ui;
    DemoRun *m_demo;
};
