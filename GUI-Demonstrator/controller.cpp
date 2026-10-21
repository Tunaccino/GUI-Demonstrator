#include "controller.h"

#include "demorun.h"
#include "knobinput.h"
#include "plotview.h"
#include "ui_Controller.h"

#include <QCoreApplication>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>

namespace {

// Encoder-Aufloesung: hier deine echten Werte eintragen.
constexpr double kMotorTicksPerDegree = 4096.0 / 360.0;
constexpr double kGearTicksPerDegree = 1024.0 / 360.0;

} // namespace

Controller::Controller(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::dialog)
    , m_demo(new DemoRun(this))
{
    ui->setupUi(this);

    applyCorporateFonts();
    loadLogo();

    ui->lineEdit_3->setText(QStringLiteral("140.0")); // P
    ui->lineEdit_2->setText(QStringLiteral("40.0"));  // I
    ui->lineEdit->setText(QStringLiteral("22.0"));    // D
    ui->lineEdit_4->setText(QStringLiteral("90.0"));  // Goal

    // Muss zum Stylesheet in Controller.ui passen:
    // Theme::Kapton zu Controller.ui, Theme::Eloxal zu Controller_eloxal.ui.
    ui->graphicsView->setTheme(PlotView::Theme::Kapton);

    ui->graphicsView->setGoal(90.0);
    ui->graphicsView->setTimeWindow(12.0); // rollendes Fenster; 0 = alles zeigen

    connect(ui->pushButton, &QPushButton::clicked, this, &Controller::applyGoal);
    connect(ui->resetButton, &QPushButton::clicked, this, &Controller::resetAll);
    connect(ui->demoButton, &QPushButton::clicked, this, &Controller::toggleDemo);

    connect(m_demo, &DemoRun::started, this,
            [this] { ui->demoButton->setText(tr("Stopp")); });
    connect(m_demo, &DemoRun::finished, this,
            [this] { ui->demoButton->setText(tr("Probelauf")); });

    connect(m_demo, &DemoRun::sample, this, &Controller::showSample);

    setupKnobs();

    resetAll();
}

Controller::~Controller()
{
    delete ui;
}

// Corporate Design: Oswald Bold fuer Titel und Ueberschriften, Arial fuer
// alles andere. DARE Atomed bleibt aussen vor - die ist laut Vorgabe
// ausschliesslich fuer Projektnamen in Versalien.
//
// Die Familien werden hier und nicht im Stylesheet gesetzt: Qt-Stylesheets
// koennen keine font-family-Fallbackliste, die Regel faellt dann komplett aus.
// QFont::setFamilies kann es.
void Controller::applyCorporateFonts()
{
    QFont body;
    body.setFamilies({QStringLiteral("Arial"), QStringLiteral("Liberation Sans"),
                      QStringLiteral("Helvetica")});
    body.setPointSize(font().pointSize());
    setFont(body);

    QFont heading;
    heading.setFamilies({QStringLiteral("Oswald"), QStringLiteral("Arial Narrow"),
                         QStringLiteral("Liberation Sans Narrow"),
                         QStringLiteral("Arial"), QStringLiteral("Liberation Sans")});
    heading.setBold(true);

    QFont groupTitle = heading;
    groupTitle.setPointSize(body.pointSize() - 1);
    groupTitle.setCapitalization(QFont::AllUppercase);

    const QList<QGroupBox *> groups = {ui->statusGroup, ui->pidGroup, ui->targetGroup};
    for (QGroupBox *group : groups)
        group->setFont(groupTitle);

    // Arial setzt Ziffern auf gleiche Breite, deshalb braucht es fuer die
    // Messwerte keine vierte Schrift - die Zahlen springen beim Aktualisieren
    // trotzdem nicht.
    QFont value = body;
    const QList<QLabel *> valueLabels = {
        ui->valueCurrentPosition, ui->valueMotorEncoder, ui->valueGearEncoder,
        ui->valueVelocity, ui->valueAcceleration
    };
    for (QLabel *label : valueLabels)
        label->setFont(value);
}

// Das Logo wird nie nachgezeichnet, sondern immer als Datei platziert.
// Erwartet wird DARE-ROBOTICS_01_logo-horizontal.svg im Unterordner logo/,
// entweder neben der Binary oder im Arbeitsverzeichnis. Fuer den dunklen Grund
// gehoert dort die weisse Einfarbversion aus 5_einfarbig-weiss hinein.
// Fehlt die Datei, bleibt der Plot einfach ohne Wasserzeichen.
void Controller::loadLogo()
{
    const QStringList candidates = {
        QStringLiteral("logo/DARE-ROBOTICS_01_logo-horizontal.svg"),
        QStringLiteral("DARE-ROBOTICS_01_logo-horizontal.svg"),
        QStringLiteral("logo/DARE-ROBOTICS_01_logo-horizontal.png"),
        QStringLiteral("DARE-ROBOTICS_01_logo-horizontal.png"),
    };

    // Einmal relativ zum Arbeitsverzeichnis, einmal neben der Binary. Aus einer
    // IDE gestartet ist das Arbeitsverzeichnis oft der Build-Ordner, von der
    // Shell aus das Projektverzeichnis.
    const QString exeDir = QCoreApplication::applicationDirPath() + QLatin1Char('/');

    for (const QString &name : candidates) {
        QPixmap pixmap(name);
        if (pixmap.isNull())
            pixmap.load(exeDir + name);
        if (pixmap.isNull())
            continue;

        ui->graphicsView->setWatermark(pixmap);
        return;
    }
}

void Controller::applyGoal()
{
    const double goal = ui->lineEdit_4->text().toDouble();
    ui->graphicsView->setGoal(goal);

    // ... hier die Reglerparameter an die echte Regelung uebergeben ...
}

void Controller::resetAll()
{
    m_demo->stop();
    ui->graphicsView->clearData();

    ui->valueCurrentPosition->setText(QStringLiteral("0.00 \u00B0"));
    ui->valueMotorEncoder->setText(QStringLiteral("0"));
    ui->valueGearEncoder->setText(QStringLiteral("0"));
    ui->valueVelocity->setText(QStringLiteral("0.00 \u00B0/s"));
    ui->valueAcceleration->setText(QStringLiteral("0.00 \u00B0/s\u00B2"));
}

void Controller::toggleDemo()
{
    if (m_demo->isRunning()) {
        m_demo->stop();
        return;
    }

    const double goal = ui->lineEdit_4->text().toDouble();

    ui->graphicsView->setGoal(goal);
    ui->graphicsView->clearData();

    m_demo->start(goal,
                  ui->lineEdit_3->text().toDouble(),
                  ui->lineEdit_2->text().toDouble(),
                  ui->lineEdit->text().toDouble());
}

// Genau dieser Slot wird spaeter mit den echten Hardwaredaten gefuettert.
void Controller::showSample(double t, double motorDeg, double gearDeg, double positionDeg,
                            double velocity, double acceleration)
{
    ui->graphicsView->appendSample(t, motorDeg, gearDeg, positionDeg);

    ui->valueCurrentPosition->setText(QString::number(positionDeg, 'f', 2) + QStringLiteral(" \u00B0"));
    ui->valueMotorEncoder->setText(QString::number(qRound(motorDeg * kMotorTicksPerDegree)));
    ui->valueGearEncoder->setText(QString::number(qRound(gearDeg * kGearTicksPerDegree)));
    ui->valueVelocity->setText(QString::number(velocity, 'f', 2) + QStringLiteral(" \u00B0/s"));
    ui->valueAcceleration->setText(QString::number(acceleration, 'f', 2) + QStringLiteral(" \u00B0/s\u00B2"));
}

// --- Drehgeber -------------------------------------------------------------
//
// Vier KY-040, jeder fest einem Feld zugeordnet:
//   Drehen  -> Wert aendern (bei Goal sofort uebernehmen)
//   Druecken bei P, I, D -> zwischen fein und grob (x10) umschalten
//   Druecken bei Goal    -> Probelauf starten bzw. stoppen
//
// Die GPIO-Nummern muessen zu den dtoverlay-Zeilen in /boot/firmware/config.txt
// passen (CLK = pin_a beim rotary-encoder, SW = gpio beim gpio-key).

void Controller::setupKnobs()
{
    //          CLK  SW  Feld            Beschriftung  Schritt  Min      Max
    m_knobs = {
        {21, 16, ui->lineEdit_3, ui->label_3, 1.0,    0.0,   1000.0}, // Knopf 1: P
        {26, 13, ui->lineEdit_2, ui->label,   1.0,    0.0,   1000.0}, // Knopf 2: I
        { 6,  0, ui->lineEdit,   ui->label_2, 0.5,    0.0,   1000.0}, // Knopf 3: D
        {11, 10, ui->lineEdit_4, nullptr,     1.0, -360.0,    360.0}, // Knopf 4: Goal in Grad
    };

    for (int i = 0; i < m_knobs.size(); ++i) {
        Knob &knob = m_knobs[i];
        if (knob.label)
            knob.labelText = knob.label->text();

        auto *input = new KnobInput(knob.clkGpio, knob.swGpio, this);
        connect(input, &KnobInput::rotated, this, [this, i](int steps) { turnKnob(i, steps); });
        connect(input, &KnobInput::pressed, this, [this, i] { pressKnob(i); });
    }
}

void Controller::turnKnob(int index, int steps)
{
    const Knob &knob = m_knobs[index];

    // Fokus auf das Feld, damit man sieht, was sich gerade aendert.
    knob.edit->setFocus();

    // Auch "1,5" mit Komma soll funktionieren, falls jemand von Hand tippt.
    QString text = knob.edit->text();
    text.replace(QLatin1Char(','), QLatin1Char('.'));

    const double step = knob.coarse ? knob.step * 10.0 : knob.step;
    const double value = qBound(knob.min, text.toDouble() + steps * step, knob.max);
    knob.edit->setText(QString::number(value, 'f', 1));

    // Die Zielposition wirkt sofort, wie nach einem Klick auf "Uebernehmen".
    if (knob.edit == ui->lineEdit_4)
        applyGoal();
}

void Controller::pressKnob(int index)
{
    Knob &knob = m_knobs[index];

    if (knob.edit == ui->lineEdit_4) {
        toggleDemo();
        return;
    }

    knob.coarse = !knob.coarse;
    knob.edit->setFocus();
    if (knob.label)
        knob.label->setText(knob.coarse ? knob.labelText + QStringLiteral(" \u00D710")
                                        : knob.labelText);
}
