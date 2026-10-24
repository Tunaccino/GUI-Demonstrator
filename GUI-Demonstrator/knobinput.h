#pragma once

#include <QList>
#include <QObject>

class QSocketNotifier;

/**
 * Ein Drehgeber (KY-040) samt Taster als Eingabe fuer die GUI.
 *
 * Die eigentliche Auswertung macht der Linux-Kernel ueber die Overlays
 * rotary-encoder und gpio-key (Eintraege in /boot/firmware/config.txt).
 * Die Klasse liest nur die fertigen Ereignisse aus /dev/input und meldet sie
 * als Qt-Signale. Sie laeuft in der normalen Ereignisschleife, ein eigener
 * Thread ist nicht noetig.
 *
 * Jede Instanz gehoert zu genau einem Drehgeber. Welcher das ist, ergibt sich
 * aus den GPIO-Nummern von CLK und SW: Die Overlays legen die Geraete als
 * /dev/input/by-path/platform-rotary@<CLK hex>-event bzw.
 * platform-button@<SW hex>-event an.
 *
 * Fehlt ein Geraet (oder laeuft das Programm nicht unter Linux), tut die
 * Instanz einfach nichts.
 */
class KnobInput : public QObject
{
    Q_OBJECT

public:
    /** swGpio < 0 bedeutet: Drehgeber ohne Taster. */
    KnobInput(int clkGpio, int swGpio, QObject *parent = nullptr);
    ~KnobInput() override;

    bool hasRotary() const { return m_hasRotary; }
    bool hasButton() const { return m_hasButton; }

signals:
    /** +1 bzw. -1 pro Rastschritt, je nach Drehrichtung. */
    void rotated(int steps);

    /** Taster gedrueckt (wird nur beim Herunterdruecken gemeldet). */
    void pressed();

private:
    bool openDevice(const QString &path);
    void readEvents(int fd);

    QList<int> m_fds;
    QList<QSocketNotifier *> m_notifiers;
    bool m_hasRotary = false;
    bool m_hasButton = false;
};
