#pragma once

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QSerialPort>
#include <QTimer>

#include <optional>

/**
 * Verbindung zu einem Board (Arduino mit pd-demonstrator-Firmware) nach dem
 * Protokoll aus pd-demonstrator-interface (interface.h).
 *
 * Ablauf: Port oeffnen, warten (der Arduino startet beim Oeffnen neu), Magic
 * und Version pruefen, danach Auftraege abarbeiten. Es ist immer nur eine
 * Anfrage unterwegs. Ungueltige Frames verwirft die Firmware ohne Antwort,
 * deshalb wird nach einem Timeout wiederholt und nach mehreren Fehlschlaegen
 * neu verbunden. Faellt der Port weg (Kabel gezogen), wird alle paar Sekunden
 * neu versucht.
 *
 * Werte gehen als Map Maskenbit -> Rohwert (Festkomma laut interface.h) hin
 * und her. QMap sortiert nach Schluessel, damit stimmt automatisch die
 * Reihenfolge auf der Leitung (aufsteigende Maskenbits).
 */
class BoardConnection : public QObject
{
    Q_OBJECT

public:
    using Values = QMap<quint16, qint32>;

    /**
     * Ampel fuer die Statusanzeige.
     *   Disconnected  kein Board da (Port fehlt, Kabel gezogen) oder es startet gerade
     *   Error         Port da, aber das Board antwortet nicht, hat die falsche
     *                 Firmware/Version oder lehnt eine Anfrage ab
     *   Connected     alles in Ordnung
     */
    enum class Health { Disconnected, Error, Connected };
    Q_ENUM(Health)

    BoardConnection(const QString &name, const QString &portName, QObject *parent = nullptr);

    bool isConnected() const { return m_state == State::Connected; }

    /**
     * Werte setzen. Ein noch nicht gesendetes SET mit denselben Feldern wird
     * ersetzt, so stauen sich beim schnellen Drehen keine Frames. Ohne
     * Verbindung passiert nichts; nach dem Verbinden kommt connected().
     */
    void set(const Values &values);

    /** Diese Felder regelmaessig abfragen; 0 schaltet das Abfragen ab. */
    void setPolling(quint16 mask, int intervalMs);

signals:
    /** Ampel und Klartext (englisch, fuer Tooltip und Log). */
    void statusChanged(BoardConnection::Health health, const QString &text);

    /** Magic und Version passen, ab jetzt werden Auftraege gesendet. */
    void connected();

    /** Antwort auf eine Abfrage. */
    void valuesReceived(const QMap<quint16, qint32> &values);

private:
    enum class State { Closed, Starting, Handshake, Connected };

    struct Request
    {
        QByteArray frame;
        quint8 function = 0;
        quint16 mask = 0;
        int responseSize = 0;
    };

    void open();
    void close(Health health, const QString &reason, int retryMs);
    void startHandshake();
    void enqueue(const Request &request);
    void sendNext();
    void send();
    void receive();
    void handleResponse(const QByteArray &response);
    void timeout();
    void setStatus(Health health, const QString &text);
    Health connectingHealth() const;

    QString m_name;
    QSerialPort m_port;
    QTimer m_stateTimer;   // Neuversuch bzw. Warten auf den Neustart des Arduino
    QTimer m_timeoutTimer;
    QTimer m_pollTimer;
    quint16 m_pollMask = 0;
    State m_state = State::Closed;
    QList<Request> m_queue;
    std::optional<Request> m_pending;
    QByteArray m_received;
    int m_attempts = 0;
    Health m_health = Health::Disconnected;
    QString m_status;
};
