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
    , m_knob(new KnobInput(this))
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

    setupKnob();

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

void Controller::setupKnob()
{
    // Reihenfolge beim Durchschalten per Druck. Schrittweite und Grenzen
    // hier nach Bedarf anpassen.
    m_knobFields = {
        {ui->lineEdit_3, 1.0, 0.0, 1000.0},    // P
        {ui->lineEdit_2, 1.0, 0.0, 1000.0},    // I
        {ui->lineEdit,   0.5, 0.0, 1000.0},    // D
        {ui->lineEdit_4, 1.0, -360.0, 360.0},  // Goal in Grad
    };

    connect(m_knob, &KnobInput::pressed, this, &Controller::selectNextKnobField);
    connect(m_knob, &KnobInput::rotated, this, &Controller::adjustKnobField);

    // Mit Drehgeber ist von Anfang an das erste Feld aktiv, damit man sieht,
    // was das Drehen gerade veraendert.
    if (m_knob->isAvailable())
        m_knobFields[m_knobIndex].edit->setFocus();
}

// Wurde zwischendurch mit Maus oder Tastatur ein anderes Feld angeklickt,
// macht der Drehgeber dort weiter.
void Controller::syncKnobIndexToFocus()
{
    for (int i = 0; i < m_knobFields.size(); ++i) {
        if (m_knobFields[i].edit->hasFocus()) {
            m_knobIndex = i;
            return;
        }
    }
}

void Controller::selectNextKnobField()
{
    syncKnobIndexToFocus();
    m_knobIndex = (m_knobIndex + 1) % m_knobFields.size();
    m_knobFields[m_knobIndex].edit->setFocus();
}

void Controller::adjustKnobField(int steps)
{
    syncKnobIndexToFocus();
    const KnobField &field = m_knobFields[m_knobIndex];
    field.edit->setFocus();

    // Auch "1,5" mit Komma soll funktionieren, falls jemand von Hand tippt.
    QString text = field.edit->text();
    text.replace(QLatin1Char(','), QLatin1Char('.'));

    double value = text.toDouble();
    value = qBound(field.min, value + steps * field.step, field.max);
    field.edit->setText(QString::number(value, 'f', 1));

    // Die Zielposition wirkt sofort, wie nach einem Klick auf "Uebernehmen".
    if (field.edit == ui->lineEdit_4)
        applyGoal();
}
