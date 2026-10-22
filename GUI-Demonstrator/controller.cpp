#include "controller.h"

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

namespace {

// Encoder-Aufloesung: hier deine echten Werte eintragen.
constexpr double kMotorTicksPerDegree = 4096.0 / 360.0;
constexpr double kGearTicksPerDegree = 1024.0 / 360.0;

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

Controller::Controller(QWidget *parent)
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

    // Ohne Apply-Knopf gilt jede Aenderung sofort: Enter oder Feld verlassen
    // bei den PID-Werten, jede Bewegung beim Zifferblatt.
    for (QLineEdit *edit : {ui->lineEdit_3, ui->lineEdit_2, ui->lineEdit})
        connect(edit, &QLineEdit::editingFinished, this, &Controller::applyPid);

    ui->motorDial->setRange(-kMotorAngleLimit, kMotorAngleLimit);
    connect(ui->motorDial, &MotorDial::valueChanged, this, &Controller::applyMotorAngle);

    setupKnobs();
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

// Wird bei jeder Aenderung von P, I oder D aufgerufen (Drehgeber oder Enter).
void Controller::applyPid()
{
    auto read = [](const QLineEdit *edit) {
        QString text = edit->text();
        text.replace(QLatin1Char(','), QLatin1Char('.'));
        return text.toDouble();
    };

    const double kp = read(ui->lineEdit_3);
    const double ki = read(ui->lineEdit_2);
    const double kd = read(ui->lineEdit);

    // ... hier kp, ki, kd an die echte Regelung uebergeben.
    //     Der Sollwert ist fest kBallSetpoint (Mitte der Bahn) ...
    Q_UNUSED(kp);
    Q_UNUSED(ki);
    Q_UNUSED(kd);
}

// Wird bei jeder Bewegung des Zifferblatts aufgerufen. Der Winkel geht direkt
// an den zweiten Motor, ohne Regelung.
void Controller::applyMotorAngle(double degrees)
{
    // ... hier degrees (-90 ... +90) an den Motor uebergeben ...
    Q_UNUSED(degrees);
}

// Genau dieser Slot wird mit den echten Hardwaredaten gefuettert.
void Controller::showSample(double t, double motorDeg, double gearDeg, double ballCm,
                            double velocity, double acceleration)
{
    ui->graphicsView->appendSample(t, ballCm);

    ui->valueCurrentPosition->setText(QString::number(ballCm, 'f', 2) + QStringLiteral(" cm"));
    ui->valueMotorEncoder->setText(QString::number(qRound(motorDeg * kMotorTicksPerDegree)));
    ui->valueGearEncoder->setText(QString::number(qRound(gearDeg * kGearTicksPerDegree)));
    ui->valueVelocity->setText(QString::number(velocity, 'f', 2) + QStringLiteral(" cm/s"));
    ui->valueAcceleration->setText(QString::number(acceleration, 'f', 2) + QStringLiteral(" cm/s\u00B2"));
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
        {21, 16, ui->lineEdit_3, ui->label_3, nullptr,        1.0,    0.0, 1000.0}, // Knopf 1: P
        {26, 13, ui->lineEdit_2, ui->label,   nullptr,        1.0,    0.0, 1000.0}, // Knopf 2: I
        { 6,  0, ui->lineEdit,   ui->label_2, nullptr,        0.5,    0.0, 1000.0}, // Knopf 3: D
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
