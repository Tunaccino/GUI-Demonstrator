#include "knobinput.h"

#include <QDebug>
#include <QSocketNotifier>

#ifdef Q_OS_LINUX
#include <QFileInfo>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace {

QString devicePath(const char *kind, int gpio)
{
    // Die Overlays benennen die Geraete nach der GPIO-Nummer in Hex,
    // z. B. GPIO21 -> platform-rotary@15-event.
    return QStringLiteral("/dev/input/by-path/platform-%1@%2-event")
        .arg(QLatin1String(kind), QString::number(gpio, 16));
}

} // namespace

KnobInput::KnobInput(int clkGpio, int swGpio, QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_LINUX
    m_hasRotary = openDevice(devicePath("rotary", clkGpio));
    if (swGpio >= 0)
        m_hasButton = openDevice(devicePath("button", swGpio));
#else
    Q_UNUSED(clkGpio);
    Q_UNUSED(swGpio);
#endif
}

KnobInput::~KnobInput()
{
    // Erst die Notifier abmelden, dann die Dateien schliessen.
    qDeleteAll(m_notifiers);
#ifdef Q_OS_LINUX
    for (int fd : std::as_const(m_fds))
        ::close(fd);
#endif
}

bool KnobInput::openDevice(const QString &path)
{
#ifdef Q_OS_LINUX
    if (!QFileInfo::exists(path)) {
        qWarning().noquote() << "Nicht gefunden:" << path
                             << "- Eintrag in config.txt pruefen und neu starten.";
        return false;
    }

    const int fd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        qWarning().noquote() << "Kann" << path << "nicht oeffnen:" << std::strerror(errno)
                             << "(Benutzer in der Gruppe 'input'?)";
        return false;
    }

    // Exklusiv belegen: Solange die GUI laeuft, sieht der Desktop die
    // Ereignisse nicht. Sonst koennte ein Taster z. B. als Tastendruck
    // im gerade aktiven Fenster landen.
    ::ioctl(fd, EVIOCGRAB, 1);

    m_fds.append(fd);

    auto *notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this, fd] { readEvents(fd); });
    m_notifiers.append(notifier);

    qInfo().noquote() << "Geoeffnet:" << path;
    return true;
#else
    Q_UNUSED(path);
    return false;
#endif
}

void KnobInput::readEvents(int fd)
{
#ifdef Q_OS_LINUX
    input_event ev;
    while (::read(fd, &ev, sizeof ev) == static_cast<ssize_t>(sizeof ev)) {
        if (ev.type == EV_REL && ev.value != 0)
            emit rotated(ev.value);
        else if (ev.type == EV_KEY && ev.value == 1) // 1 = gedrueckt, 0 = losgelassen, 2 = Wiederholung
            emit pressed();
    }
#else
    Q_UNUSED(fd);
#endif
}
