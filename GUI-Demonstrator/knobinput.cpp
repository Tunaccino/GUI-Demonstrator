#include "knobinput.h"

#include <QDebug>
#include <QSocketNotifier>

#ifdef Q_OS_LINUX
#include <QDir>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

KnobInput::KnobInput(QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_LINUX
    // Die Overlays legen Geraete an, deren Name die GPIO-Nummer in Hex enthaelt,
    // z. B. platform-rotary@11-event (GPIO17) und platform-button@1b-event (GPIO27).
    const QDir dir(QStringLiteral("/dev/input/by-path"));
    const QStringList names = dir.entryList(
        {QStringLiteral("platform-rotary@*-event"), QStringLiteral("platform-button@*-event")},
        QDir::Files | QDir::System);

    for (const QString &name : names)
        openDevice(dir.filePath(name));

    if (m_fds.isEmpty())
        qInfo("Kein Drehgeber gefunden - Bedienung nur per Maus/Tastatur.");
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

void KnobInput::openDevice(const QString &path)
{
#ifdef Q_OS_LINUX
    const int fd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        qWarning().noquote() << "Kann" << path << "nicht oeffnen:" << std::strerror(errno)
                             << "(Benutzer in der Gruppe 'input'?)";
        return;
    }

    // Exklusiv belegen: Solange die GUI laeuft, sieht der Desktop die
    // Ereignisse nicht. Sonst koennte der Taster z. B. als Tastendruck
    // im gerade aktiven Fenster landen.
    ::ioctl(fd, EVIOCGRAB, 1);

    m_fds.append(fd);

    auto *notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this, fd] { readEvents(fd); });
    m_notifiers.append(notifier);

    qInfo().noquote() << "Drehgeber-Geraet geoeffnet:" << path;
#else
    Q_UNUSED(path);
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
