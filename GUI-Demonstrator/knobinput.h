#pragma once

#include <QList>
#include <QObject>

class QSocketNotifier;

/**
 * Drehgeber (KY-040) als Eingabe fuer die GUI.
 *
 * Die eigentliche Auswertung macht der Linux-Kernel ueber die Overlays
 * rotary-encoder und gpio-key (Eintraege in /boot/firmware/config.txt).
 * Die Klasse liest nur die fertigen Ereignisse aus /dev/input und meldet sie
 * als Qt-Signale. Sie laeuft in der normalen Ereignisschleife, ein eigener
 * Thread ist nicht noetig.
 *
 * Gesucht wird automatisch nach /dev/input/by-path/platform-rotary@*-event
 * und platform-button@*-event, es ist also egal, welche GPIOs belegt sind.
 * Ohne Drehgeber (oder unter Windows/macOS) tut die Klasse einfach nichts.
 */
class KnobInput : public QObject
{
    Q_OBJECT

public:
    explicit KnobInput(QObject *parent = nullptr);
    ~KnobInput() override;

    /** true, wenn mindestens ein Geraet (Drehgeber oder Taster) geoeffnet ist. */
    bool isAvailable() const { return !m_fds.isEmpty(); }

signals:
    /** +1 bzw. -1 pro Rastschritt, je nach Drehrichtung. */
    void rotated(int steps);

    /** Taster gedrueckt (wird nur beim Herunterdruecken gemeldet). */
    void pressed();

private:
    void openDevice(const QString &path);
    void readEvents(int fd);

    QList<int> m_fds;
    QList<QSocketNotifier *> m_notifiers;
};
