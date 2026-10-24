#include "controller.h"

#include "boardconnection.h"
#include "interface.h"
#include "knobinput.h"
#include "motordial.h"
#include "plotview.h"
#include "svglogo.h"
#include "ui_Controller.h"

#include <QApplication>
#include <QFontDatabase>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QSvgRenderer>

#include <cmath>
#include <optional>

namespace {

// Die Firmware rechnet in Festkomma (interface.h): jeder Wert mal
// 2^FRACTION_BITS, Positionen in Steps des jeweiligen Winkelsensors.
constexpr double kFixedPointScale = double(1 << FRACTION_BITS);

// Winkel des Gelenk-Encoders (direct), bei dem das Zifferblatt 0 zeigt. Am
// Demonstrator ablesen: Gelenk senkrecht stellen, Getriebe-Encoder ablesen.
constexpr double kDialZeroDeg = 0.0;

// Encoder 20-mal pro Sekunde abfragen; eine Abfrage braucht bei 9600 Baud
// etwa 25 ms. Das obere Board liefert noch nichts fuer die Anzeige, dort
// merkt die seltene Abfrage nur, wenn die Verbindung abreisst.
constexpr int kEncoderPollMs = 50;
constexpr int kHeartbeatMs = 1000;

// Obergrenze fuer P, I und D, fuer Drehgeber und Tastatur. Negative Werte
// machen den Regler instabil, deshalb ab 0. Die Firmware speichert die Werte
// als int32 mal 2^FRACTION_BITS, darstellbar waere also knapp 65536. Enger ist
// aber die Stellgroesse P * Fehler + D * Geschwindigkeit + I * Integral: Ab
// 65536 Steps/cs^2 laeuft sie in der Firmware ueber und wechselt das
// Vorzeichen. Bei einem Fehler von 180 Grad ist das schon bei
// P = 65536 / (INTERFACE_STEPS_IN_ROTATION / 2) erreicht, mit Riemen 4:1 bei
// P = 82. Schutz davor muss die Firmware bieten (Stellgroesse begrenzen).
constexpr double kGainMax = 1000.0;

// Getippte PID-Werte gelten ohne Enter, sobald eine Weile nichts mehr getippt
// wurde. So geht beim Tippen von "140" nicht erst P = 1 und P = 14 raus.
constexpr int kPidCheckMs = 100;
constexpr int kTypingPauseMs = 500;

const QString kNoValue = QStringLiteral("–");

/** Zahl in einem PID-Feld; leer, wenn dort keine steht. Komma geht auch. */
std::optional<double> readGain(const QLineEdit *edit)
{
    QString text = edit->text();
    text.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    const double value = text.toDouble(&ok);
    if (!ok || !std::isfinite(value))
        return std::nullopt;
    return value;
}

// Statuspunkt der Boards: gruen verbunden, rot kein Board, gelb Fehler.
// Gelb ist Kapton 300 aus der DARE-Palette.
constexpr int kStatusDotSize = 12;
const QString kStatusGreen = QStringLiteral("#3FB950");
const QString kStatusRed = QStringLiteral("#E5534B");
const QString kStatusYellow = QStringLiteral("#E3B64D");

void showBoardStatus(QLabel *dot, BoardConnection::Health health, const QString &text)
{
    const QString &color = health == BoardConnection::Health::Connected ? kStatusGreen
                         : health == BoardConnection::Health::Error     ? kStatusYellow
                                                                        : kStatusRed;
    dot->setStyleSheet(QStringLiteral("background: %1; border-radius: %2px;").arg(color).arg(kStatusDotSize / 2));
    dot->setToolTip(text);
}

qint32 toFixedPoint(double value)
{
    return static_cast<qint32>(std::lround(value * kFixedPointScale));
}

/** Rohwert eines Winkelsensors in Grad, -180 ... +180. */
double toDegrees(qint32 raw, int stepsInRotation)
{
    return std::remainder(raw / kFixedPointScale / stepsInRotation * 360.0, 360.0);
}

// Die Bahn geht von -15 bis +15 cm, 0 ist die Mitte. Geregelt wird immer auf
// die Mitte.
constexpr double kTrackHalfLength = 15.0; // cm
constexpr double kBallSetpoint = 0.0;    // cm

// Motorwinkel, der direkt (ohne Regelung) an den zweiten Motor geht.
constexpr double kMotorAngleLimit = 90.0; // Grad, also -90 ... +90

// DARE-Farbpalette, Eloxal-Stufen fuer den dunklen Grund.
const QColor kEloxal900(0x22, 0x26, 0x2D);
const QColor kEloxal700(0x53, 0x5A, 0x66);
const QColor kEloxal500(0x68, 0x6F, 0x7A);

// Blaupausen-Raster wie auf der Titelfolie: grosse Felder mit Buchstaben und
// Zahlen am Rand, jedes Feld in 5 x 5 feine Kaestchen geteilt. Die Masse sind
// von der Folie (960 x 540) uebernommen, das passt 1:1 zur Fenstergroesse.
constexpr int kGridMajor = 158;
constexpr int kGridMinorDivisions = 5;
constexpr int kFrameOuter = 18; // Abstand Doppelrahmen zum Fensterrand
constexpr int kFrameInner = 21;

const QString kWordmark = QStringLiteral(":/logo/ATHENE_wordmark_weiss.svg");
const QString kLogoDark = QStringLiteral(":/logo/DARE-ROBOTICS_logo-horizontal_dunkel.svg");
const QString kIconWhite = QStringLiteral(":/logo/DARE-ROBOTICS_icon_weiss.svg");

/** Laedt eine eingebaute Schrift und liefert ihren Familiennamen (leer bei Fehler). */
QString loadFont(const QString &path)
{
    const int id = QFontDatabase::addApplicationFont(path);
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    return families.isEmpty() ? QString() : families.constFirst();
}

/** Schriftfamilie mit Fallbacks; leere Eintraege werden uebersprungen. */
QStringList familyStack(const QStringList &families)
{
    QStringList stack;
    for (const QString &family : families) {
        if (!family.isEmpty() && !stack.contains(family))
            stack.append(family);
    }
    return stack;
}

/** Dynamische Eigenschaft im Stylesheet neu auswerten lassen. */
void repolish(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

} // namespace

Controller::Controller(const QString &lowerPort, const QString &upperPort, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::dialog)
{
    ui->setupUi(this);

    applyCorporateFonts();
    loadBranding();

    ui->lineEdit_3->setText(QStringLiteral("140.0")); // P
    ui->lineEdit_2->setText(QStringLiteral("40.0"));  // I
    ui->lineEdit->setText(QStringLiteral("22.0"));    // D

    ui->graphicsView->setTrack(-kTrackHalfLength, kTrackHalfLength);
    ui->graphicsView->setSetpoint(kBallSetpoint);
    ui->graphicsView->setUnit(QStringLiteral("cm"));
    ui->graphicsView->setTimeWindow(12.0); // rollendes Fenster; 0 = alles zeigen

    // Ohne Apply-Knopf gilt jede Aenderung sofort: Drehgeber und Zifferblatt
    // bei jeder Bewegung, getippte PID-Werte nach einer kurzen Tipp-Pause
    // (oder gleich mit Enter). Dafuer schaut m_pidTimer regelmaessig nach.
    for (QLineEdit *edit : {ui->lineEdit_3, ui->lineEdit_2, ui->lineEdit}) {
        connect(edit, &QLineEdit::editingFinished, this, [this] { applyPid(); });
        connect(edit, &QLineEdit::textEdited, this, [this] { m_pidEdited.start(); });
    }
    connect(&m_pidTimer, &QTimer::timeout, this, [this] {
        if (m_pidEdited.isValid() && m_pidEdited.elapsed() >= kTypingPauseMs) {
            m_pidEdited.invalidate();
            applyPid();
        }
    });
    m_pidTimer.start(kPidCheckMs);

    ui->motorDial->setRange(-kMotorAngleLimit, kMotorAngleLimit);
    connect(ui->motorDial, &MotorDial::valueChanged, this, &Controller::applyMotorAngle);

    setupKnobs();
    setupBoards(lowerPort, upperPort);
}

Controller::~Controller()
{
    delete ui;
}

// Corporate Design wie auf den Folien: Oswald Bold fuer Titel, Ueberschriften
// und grosse Zahlen, Oswald Light fuer die Koordinaten am Rand, Arial fuer
// alles andere. Oswald ist in die Binary eingebaut (fonts/, SIL OFL), damit
// der Pi nichts nachinstallieren muss. ATHENE steht als Wortmarke aus der
// Datei im Kopf, nicht als Text.
//
// Die Familien werden hier und nicht im Stylesheet gesetzt: Qt-Stylesheets
// koennen keine font-family-Fallbackliste, die Regel faellt dann komplett aus.
// QFont::setFamilies kann es.
void Controller::applyCorporateFonts()
{
    const QString oswaldBold = loadFont(QStringLiteral(":/fonts/Oswald-Bold.ttf"));
    const QString oswaldLight = loadFont(QStringLiteral(":/fonts/Oswald-Light.ttf"));

    const QStringList bodyFamilies = {QStringLiteral("Arial"), QStringLiteral("Liberation Sans"),
                                      QStringLiteral("Helvetica")};
    const QStringList headingFamilies = familyStack({
        oswaldBold, QStringLiteral("Oswald"), QStringLiteral("Arial Narrow"),
        QStringLiteral("Liberation Sans Narrow"), QStringLiteral("Arial"),
        QStringLiteral("Liberation Sans")});
    m_coordinateFamily = oswaldLight.isEmpty() ? headingFamilies.constFirst() : oswaldLight;

    // Grundschrift. Die Anwendung bekommt sie auch, damit die Achsen und die
    // Legende im Plot dieselbe Schrift tragen.
    QFont appFont;
    appFont.setFamilies(bodyFamilies);
    appFont.setPointSize(10);
    QApplication::setFont(appFont);

    QFont body;
    body.setFamilies(bodyFamilies);
    body.setPixelSize(14);
    setFont(body);

    auto heading = [&](int pixelSize, qreal letterSpacing = 0.0, bool upper = false) {
        QFont f;
        f.setFamilies(headingFamilies);
        f.setPixelSize(pixelSize);
        f.setBold(true);
        if (letterSpacing != 0.0)
            f.setLetterSpacing(QFont::AbsoluteSpacing, letterSpacing);
        if (upper)
            f.setCapitalization(QFont::AllUppercase);
        return f;
    };

    // Unterzeile unter ATHENE, wie auf der Titelfolie etwas enger gesetzt.
    ui->tagline->setFont(heading(22, -0.3));

    // Kartentitel wie die Abschnittsmarken der Folien: Versalien, gesperrt.
    for (QLabel *title : {ui->statusTitle, ui->pidTitle, ui->motorTitle})
        title->setFont(heading(14, 2.4, true));

    // Ballposition als grosse Zahl, wie die 92 % auf der KI-Folie.
    ui->valueCurrentPosition->setFont(heading(36));

    for (QLabel *badge : {ui->knobBadge1, ui->knobBadge2, ui->knobBadge3, ui->knobBadge4})
        badge->setFont(heading(15));

    // PID gross, damit man die Werte am Stand auch aus Abstand lesen kann.
    for (QLabel *label : {ui->label_3, ui->label, ui->label_2})
        label->setFont(heading(24, 0.5));

    for (QLineEdit *edit : {ui->lineEdit_3, ui->lineEdit_2, ui->lineEdit})
        edit->setFont(heading(32, 0.5));

    // Das Zifferblatt zeichnet seinen Wert in dieser Schrift.
    ui->motorDial->setFont(heading(30));

    // Arial setzt Ziffern auf gleiche Breite, deshalb springen die kleinen
    // Messwerte beim Aktualisieren nicht.
    QFont value = body;
    value.setPixelSize(15);
    for (QLabel *label : {ui->valueMotorEncoder, ui->valueGearEncoder,
                          ui->valueVelocity, ui->valueAcceleration})
        label->setFont(value);

    QFont caption = body;
    caption.setPixelSize(13);
    for (QLabel *label : {ui->label_8, ui->label_6, ui->label_7, ui->label_9, ui->label_10})
        label->setFont(caption);
}

// Logo und Wortmarke werden nie nachgezeichnet, sondern immer als Datei
// platziert. Beide stammen als Vektor aus dem Jury-Pitch (logo/), sind in die
// Binary eingebaut und werden von SvgLogo in jeder Groesse scharf gerendert.
// Gibt es offizielle Exporte aus dem Logo-Paket, einfach die Dateien in logo/
// unter gleichem Namen ersetzen und neu bauen.
void Controller::loadBranding()
{
    ui->wordmark->setSource(kWordmark);
    ui->wordmark->setLogoHeight(32);
    ui->wordmark->setDescription(QStringLiteral("ATHENE"));

    ui->dareLogo->setSource(kLogoDark);
    ui->dareLogo->setLogoHeight(52);
    ui->dareLogo->setDescription(QStringLiteral("DARE ROBOTICS"));

    // Wasserzeichen im Plot: nur die Bildmarke ohne Schriftzug, weiss fuer den
    // dunklen Grund. PlotView skaliert selbst; hier reicht eine grosse Vorlage.
    QSvgRenderer renderer(kIconWhite);
    if (!renderer.isValid())
        return;

    const QSizeF native = renderer.viewBoxF().size();
    const int width = 1400;
    const int height = qRound(width * native.height() / native.width());

    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&painter, QRectF(0, 0, width, height));
    painter.end();

    ui->graphicsView->setWatermark(pixmap);
}

// Hintergrund wie die Titelfolie: Eloxal-Grund, Raster, Doppelrahmen und
// Koordinaten A-F / 1-3 am Rand. Das Raster haengt oben links fest, damit es
// beim Vergroessern des Fensters nicht wandert.
void Controller::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), kEloxal900);

    const int w = width();
    const int h = height();
    const int minor = kGridMajor / kGridMinorDivisions;

    QColor minorColor = kEloxal700;
    minorColor.setAlpha(60);
    QColor majorColor = kEloxal700;
    majorColor.setAlpha(140);

    // Feines Raster, ohne die Linien, die das grobe Raster ohnehin zeichnet.
    p.setPen(QPen(minorColor, 1));
    for (int i = 1; i * minor < w; ++i) {
        if (i % kGridMinorDivisions)
            p.drawLine(i * minor, 0, i * minor, h);
    }
    for (int i = 1; i * minor < h; ++i) {
        if (i % kGridMinorDivisions)
            p.drawLine(0, i * minor, w, i * minor);
    }

    p.setPen(QPen(majorColor, 1));
    for (int x = kGridMajor; x < w; x += kGridMajor)
        p.drawLine(x, 0, x, h);
    for (int y = kGridMajor; y < h; y += kGridMajor)
        p.drawLine(0, y, w, y);

    // Doppelrahmen: aussen kraeftig, innen leise.
    p.setPen(QPen(kEloxal700, 1));
    p.drawRect(QRect(kFrameOuter, kFrameOuter, w - 2 * kFrameOuter - 1, h - 2 * kFrameOuter - 1));
    p.setPen(QPen(majorColor, 1));
    p.drawRect(QRect(kFrameInner, kFrameInner, w - 2 * kFrameInner - 1, h - 2 * kFrameInner - 1));

    // Koordinaten: Buchstaben oben ueber jedem vollen Feld, Zahlen links und
    // rechts. Angeschnittene Felder am Rand bleiben unbeschriftet.
    QFont coordinate;
    coordinate.setFamilies({m_coordinateFamily, QStringLiteral("Arial")});
    coordinate.setPixelSize(10);
    coordinate.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    p.setFont(coordinate);
    p.setPen(kEloxal500);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    for (int i = 0; i < 26; ++i) {
        const int cx = i * kGridMajor + kGridMajor / 2;
        if (cx > w - kFrameInner - minor)
            break;
        p.drawText(QRect(cx - 10, 0, 20, kFrameOuter), Qt::AlignCenter,
                   QString(QChar('A' + i)));
    }

    for (int j = 0;; ++j) {
        const int cy = j * kGridMajor + kGridMajor / 2;
        if (cy > h - kFrameInner - minor)
            break;
        const QString number = QString::number(j + 1);
        p.drawText(QRect(0, cy - 10, kFrameOuter, 20), Qt::AlignCenter, number);
        p.drawText(QRect(w - kFrameOuter, cy - 10, kFrameOuter, 20), Qt::AlignCenter, number);
    }
}

// Liest P, I und D aus den Feldern und schickt sie, wenn sie sich geaendert
// haben (force: immer, z. B. nach dem Neuverbinden). Aufgerufen von den
// Drehgebern, von Enter und nach einer Tipp-Pause.
void Controller::applyPid(bool force)
{
    const QList<QLineEdit *> edits = {ui->lineEdit_3, ui->lineEdit_2, ui->lineEdit};
    QList<double> gains;
    for (qsizetype i = 0; i < edits.size(); ++i) {
        const std::optional<double> typed = readGain(edits[i]);
        // Keine Zahl im Feld (z. B. leer getippt): letzten gesendeten Wert zurueckholen.
        std::optional<double> value = typed;
        if (!value && m_sentPid.size() == edits.size())
            value = m_sentPid[i];
        if (!value)
            return;

        const double bounded = qBound(0.0, *value, kGainMax);
        if (!typed || bounded != *typed)
            edits[i]->setText(QString::number(bounded, 'f', 1));
        gains << bounded;
    }
    if (!force && gains == m_sentPid)
        return;
    m_sentPid = gains;

    // Die Werte gehen unveraendert als Festkommazahlen an die Firmware, die
    // Felder zeigen also die Einheiten der Firmware (1/cs^2, 1/cs^3, 1/cs).
    // Mehrere Aenderungen kurz hintereinander fasst BoardConnection zusammen.
    m_upper->set({{IM_PARAMETER_P, toFixedPoint(gains[0])},
                  {IM_PARAMETER_I, toFixedPoint(gains[1])},
                  {IM_PARAMETER_D, toFixedPoint(gains[2])}});
}

// Wird bei jeder Bewegung des Zifferblatts aufgerufen. Der Winkel geht als
// Zielposition an das Schrittmotor-Gelenk; ohne Ballregelung, die Firmware
// faehrt ihn mit ihrem eigenen Gelenkregler an.
void Controller::applyMotorAngle(double degrees)
{
    const double steps = (degrees + kDialZeroDeg) / 360.0 * INTERFACE_STEPS_IN_ROTATION;
    m_lower->set({{IM_TARGET_POSITION, toFixedPoint(steps)}});
}

void Controller::showSample(double t, double ballCm, double velocity, double acceleration)
{
    ui->graphicsView->appendSample(t, ballCm);

    ui->valueCurrentPosition->setText(QString::number(ballCm, 'f', 2) + QStringLiteral(" cm"));
    ui->valueVelocity->setText(QString::number(velocity, 'f', 2) + QStringLiteral(" cm/s"));
    ui->valueAcceleration->setText(QString::number(acceleration, 'f', 2) + QStringLiteral(" cm/s\u00B2"));
}

void Controller::showEncoders(double motorDeg, double gearDeg)
{
    ui->valueMotorEncoder->setText(QString::number(motorDeg, 'f', 1) + QStringLiteral(" \u00B0"));
    ui->valueGearEncoder->setText(QString::number(gearDeg, 'f', 1) + QStringLiteral(" \u00B0"));
}

// --- Boards ----------------------------------------------------------------
//
// Annahme bis zur Pruefung am Demonstrator: Der Schrittmotor (unteres Board)
// ist das Gelenk am Zifferblatt, der BLDC (oberes Board) kippt das Rohr und
// balanciert den Ball. Stimmt das nicht, hier m_lower und m_upper tauschen.
// Beide Encoder gehoeren zum Schrittmotor-Gelenk: direct sitzt hinter dem
// Riemen am Gelenk, indirect an der Motorwelle.

void Controller::setupBoards(const QString &lowerPort, const QString &upperPort)
{
    m_lowerStatus = addStatusRow(QStringLiteral("Lower Board"));
    m_upperStatus = addStatusRow(QStringLiteral("Upper Board"));

    m_lower = new BoardConnection(QStringLiteral("Lower board"), lowerPort, this);
    m_upper = new BoardConnection(QStringLiteral("Upper board"), upperPort, this);

    connect(m_lower, &BoardConnection::statusChanged, this, [this](BoardConnection::Health health, const QString &text) {
        showBoardStatus(m_lowerStatus, health, text);
        if (!m_lower->isConnected()) {
            ui->valueMotorEncoder->setText(kNoValue);
            ui->valueGearEncoder->setText(kNoValue);
        }
    });
    connect(m_upper, &BoardConnection::statusChanged, this, [this](BoardConnection::Health health, const QString &text) {
        showBoardStatus(m_upperStatus, health, text);
    });

    // Ein frisch gestartetes Board steht auf null, deshalb nach jedem
    // (Neu-)Verbinden den aktuellen Stand der GUI uebertragen.
    connect(m_lower, &BoardConnection::connected, this, [this] { applyMotorAngle(ui->motorDial->value()); });
    connect(m_upper, &BoardConnection::connected, this, [this] { applyPid(true); });

    m_lower->setPolling(IM_MEASURED_POSITION_DIRECT | IM_MEASURED_POSITION_INDIRECT, kEncoderPollMs);
    connect(m_lower, &BoardConnection::valuesReceived, this, [this](const BoardConnection::Values &values) {
        showEncoders(toDegrees(values.value(IM_MEASURED_POSITION_INDIRECT), INTERFACE_STEPS_IN_MOTOR_ROTATION),
                     toDegrees(values.value(IM_MEASURED_POSITION_DIRECT), INTERFACE_STEPS_IN_ROTATION));
    });
    m_upper->setPolling(IM_PARAMETER_P, kHeartbeatMs);
}

// Neue Zeile unten in der Status-Karte: Beschriftung wie bei den Encodern,
// rechts ein Statuspunkt. Den Klartext zeigt der Tooltip.
QLabel *Controller::addStatusRow(const QString &caption)
{
    auto *captionLabel = new QLabel(caption, this);
    captionLabel->setProperty("role", QStringLiteral("caption"));
    captionLabel->setFont(ui->label_6->font());

    auto *dot = new QLabel(this);
    dot->setFixedSize(kStatusDotSize, kStatusDotSize);
    showBoardStatus(dot, BoardConnection::Health::Disconnected, QStringLiteral("not connected"));

    const int row = ui->statusGrid->rowCount();
    ui->statusGrid->addWidget(captionLabel, row, 0);
    ui->statusGrid->addWidget(dot, row, 1, Qt::AlignRight | Qt::AlignVCenter);
    return dot;
}

// --- Drehgeber -------------------------------------------------------------
//
// Vier KY-040, jeder fest einem Feld zugeordnet. Die goldene Nummer am
// Feld zeigt, welcher Knopf es ist:
//   Knopf 1-3: P, I, D       Knopf 4: Motorwinkel (Zifferblatt)
//   Drehen   -> Wert aendern, gilt sofort
//   Druecken -> zwischen fein und grob (x10) umschalten
//
// Die GPIO-Nummern muessen zu den dtoverlay-Zeilen in /boot/firmware/config.txt
// passen (CLK = pin_a beim rotary-encoder, SW = gpio beim gpio-key).

void Controller::setupKnobs()
{
    //  CLK  SW  Feld           Beschriftung Zifferblatt     Schritt  Min    Max
    m_knobs = {
        {21, 16, ui->lineEdit_3, ui->label_3, nullptr,        1.0,    0.0, kGainMax}, // Knopf 1: P
        {26, 13, ui->lineEdit_2, ui->label,   nullptr,        1.0,    0.0, kGainMax}, // Knopf 2: I
        { 6,  0, ui->lineEdit,   ui->label_2, nullptr,        0.5,    0.0, kGainMax}, // Knopf 3: D
        {11, 10, nullptr,        nullptr,     ui->motorDial,  1.0,
         -kMotorAngleLimit, kMotorAngleLimit},                                      // Knopf 4: Motorwinkel
    };

    for (int i = 0; i < m_knobs.size(); ++i) {
        Knob &knob = m_knobs[i];
        if (knob.label) {
            knob.labelText = knob.label->text();

            // Platz fuer "x10" gleich mit reservieren, sonst rutscht das Feld
            // beim Umschalten zur Seite.
            const QString widest = knob.labelText + QStringLiteral(" \u00D710");
            knob.label->setMinimumWidth(knob.label->fontMetrics().horizontalAdvance(widest) + 2);
        }

        auto *input = new KnobInput(knob.clkGpio, knob.swGpio, this);
        connect(input, &KnobInput::rotated, this, [this, i](int steps) { turnKnob(i, steps); });
        connect(input, &KnobInput::pressed, this, [this, i] { pressKnob(i); });
    }
}

void Controller::turnKnob(int index, int steps)
{
    const Knob &knob = m_knobs[index];
    const double step = knob.coarse ? knob.step * 10.0 : knob.step;

    // Das Zifferblatt rechnet selbst und meldet den neuen Winkel.
    if (knob.dial) {
        knob.dial->setFocus();
        knob.dial->stepBy(steps * step);
        return;
    }

    // Fokus auf das Feld, damit man sieht, was sich gerade aendert.
    knob.edit->setFocus();

    // Auch "1,5" mit Komma soll funktionieren, falls jemand von Hand tippt.
    QString text = knob.edit->text();
    text.replace(QLatin1Char(','), QLatin1Char('.'));

    const double value = qBound(knob.min, text.toDouble() + steps * step, knob.max);
    knob.edit->setText(QString::number(value, 'f', 1));

    // Ohne Apply-Knopf gilt der neue Wert sofort.
    applyPid();
}

void Controller::pressKnob(int index)
{
    Knob &knob = m_knobs[index];

    knob.coarse = !knob.coarse;

    if (knob.dial) {
        knob.dial->setFocus();
        knob.dial->setCoarse(knob.coarse);
        return;
    }

    knob.edit->setFocus();
    if (knob.label) {
        knob.label->setText(knob.coarse ? knob.labelText + QStringLiteral(" \u00D710")
                                        : knob.labelText);
        // Im Grobmodus wird die Beschriftung golden (Stylesheet: [coarse="true"]).
        knob.label->setProperty("coarse", knob.coarse);
        repolish(knob.label);
    }
}
