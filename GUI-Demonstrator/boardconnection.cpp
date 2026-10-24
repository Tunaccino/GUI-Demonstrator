#include "boardconnection.h"

#include "interface.h"
extern "C" {
#include "checksum16.h"
}

#include <QDebug>

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace {

// Die Firmware nimmt Adresse 0 oder 1 an und antwortet immer mit 1.
constexpr quint8 kAddress = 1;

// Der Arduino startet beim Oeffnen des Ports neu (DTR) und ist nach etwa
// 2 s durch den Bootloader.
constexpr int kStartupMs = 2000;

// Die laengste Antwort (alle 16 Werte) braucht bei 9600 Baud gut 70 ms, dazu
// kommen 3,6 ms Pause, bis die Firmware das Frame-Ende erkennt.
constexpr int kTimeoutMs = 200;
constexpr int kMaxAttempts = 3;
constexpr int kReopenMs = 2000;
constexpr int kIncompatibleRetryMs = 10000;

QByteArray withChecksum(QByteArray frame)
{
    uint16_t checksum = checksum16_initialize();
    for (const char byte : std::as_const(frame))
        checksum = checksum16_update(checksum, static_cast<uint8_t>(byte));
    checksum = checksum16_finalize(checksum);
    frame.append(reinterpret_cast<const char *>(&checksum), sizeof checksum);
    return frame;
}

bool checksumValid(const QByteArray &frame)
{
    uint16_t checksum = checksum16_initialize();
    for (qsizetype i = 0; i < frame.size() - 2; ++i)
        checksum = checksum16_update(checksum, static_cast<uint8_t>(frame[i]));
    checksum = checksum16_finalize(checksum);
    uint16_t received;
    std::memcpy(&received, frame.constData() + frame.size() - 2, sizeof received);
    return checksum == received;
}

/** Anfangsstueck einer Anfrage, also alles vor den Werten und der Pruefsumme. */
template <typename T>
QByteArray head(const T &request, std::size_t size = sizeof(T))
{
    return QByteArray(reinterpret_cast<const char *>(&request), static_cast<qsizetype>(size));
}

int getResponseSize(quint16 mask)
{
    return int(sizeof(InterfaceGetResponseBegin) + qPopulationCount(mask) * sizeof(int32_t)
               + sizeof(InterfaceGetResponseEnd));
}

qint32 valueAt(const QByteArray &response, std::size_t offset)
{
    qint32 value;
    std::memcpy(&value, response.constData() + offset, sizeof value);
    return value;
}

} // namespace

BoardConnection::BoardConnection(const QString &name, const QString &portName, QObject *parent)
    : QObject(parent)
    , m_name(name)
{
    m_port.setPortName(portName);
    m_port.setBaudRate(static_cast<qint32>(INTERFACE_BAUD));
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    m_stateTimer.setSingleShot(true);
    m_timeoutTimer.setSingleShot(true);
    m_timeoutTimer.setInterval(kTimeoutMs);

    connect(&m_port, &QSerialPort::readyRead, this, &BoardConnection::receive);
    connect(&m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        // ResourceError: Geraet verschwunden, z. B. USB-Kabel gezogen
        if (error == QSerialPort::ResourceError && m_port.isOpen())
            close(Health::Disconnected, QStringLiteral("connection lost"), kReopenMs);
    });
    connect(&m_stateTimer, &QTimer::timeout, this, [this] {
        if (m_state == State::Closed)
            open();
        else if (m_state == State::Starting)
            startHandshake();
    });
    connect(&m_timeoutTimer, &QTimer::timeout, this, &BoardConnection::timeout);
    connect(&m_pollTimer, &QTimer::timeout, this, [this] {
        // Hoechstens eine Abfrage in der Schlange, sonst staut es sich, wenn
        // die Leitung langsamer ist als der Takt.
        const auto isPoll = [](const Request &request) { return request.function == IF_GET; };
        if ((m_pending && isPoll(*m_pending)) || std::any_of(m_queue.cbegin(), m_queue.cend(), isPoll))
            return;
        const InterfaceGetRequest request = {kAddress, IF_GET, m_pollMask, 0};
        enqueue({withChecksum(head(request, sizeof request - sizeof request.checksum)),
                 IF_GET, m_pollMask, getResponseSize(m_pollMask)});
    });

    // Erst aus der Ereignisschleife oeffnen, damit statusChanged schon verbunden ist.
    QTimer::singleShot(0, this, &BoardConnection::open);
}

void BoardConnection::set(const Values &values)
{
    if (m_state != State::Connected || values.isEmpty())
        return;

    InterfaceSetRequestBegin begin = {kAddress, IF_SET, 0};
    for (auto it = values.cbegin(); it != values.cend(); ++it)
        begin.mask |= it.key();
    QByteArray frame = head(begin);
    for (const qint32 value : values)
        frame.append(reinterpret_cast<const char *>(&value), sizeof value);
    const Request request = {withChecksum(frame), IF_SET, begin.mask, int(sizeof(InterfaceSetResponse))};

    // Noch nicht gesendet? Dann gilt einfach der neueste Wert.
    for (Request &queued : m_queue) {
        if (queued.function == IF_SET && queued.mask == request.mask) {
            queued = request;
            return;
        }
    }

    // Einstellungen vor die wartende Abfrage, die Anzeige kann warten.
    const auto poll = std::find_if(m_queue.begin(), m_queue.end(),
                                   [](const Request &queued) { return queued.function == IF_GET; });
    m_queue.insert(poll, request);
    sendNext();
}

void BoardConnection::setPolling(quint16 mask, int intervalMs)
{
    m_pollMask = mask;
    m_pollTimer.setInterval(intervalMs);
    if (mask == 0)
        m_pollTimer.stop();
    else if (m_state == State::Connected)
        m_pollTimer.start();
}

void BoardConnection::open()
{
    m_port.clearError();
    if (!m_port.open(QIODevice::ReadWrite)) {
        m_state = State::Closed;
        setStatus(Health::Disconnected, QStringLiteral("not found (%1)").arg(m_port.portName()));
        m_stateTimer.start(kReopenMs);
        return;
    }
    m_state = State::Starting;
    setStatus(connectingHealth(), QStringLiteral("starting ..."));
    m_stateTimer.start(kStartupMs);
}

void BoardConnection::close(Health health, const QString &reason, int retryMs)
{
    m_port.close();
    m_timeoutTimer.stop();
    m_pollTimer.stop();
    m_queue.clear();
    m_pending.reset();
    m_received.clear();
    m_state = State::Closed;
    setStatus(health, reason);
    m_stateTimer.start(retryMs);
}

void BoardConnection::startHandshake()
{
    // Was der Bootloader beim Start ausgibt, gehoert nicht zum Protokoll.
    m_port.clear();
    m_state = State::Handshake;
    setStatus(connectingHealth(), QStringLiteral("checking firmware ..."));

    const InterfaceGetMagicRequest magic = {kAddress, IF_GET_MAGIC, 0};
    enqueue({withChecksum(head(magic, sizeof magic - sizeof magic.checksum)),
             IF_GET_MAGIC, 0, int(sizeof(InterfaceGetMagicResponse))});
    const InterfaceGetVersionRequest version = {kAddress, IF_GET_VERSION, 0};
    enqueue({withChecksum(head(version, sizeof version - sizeof version.checksum)),
             IF_GET_VERSION, 0, int(sizeof(InterfaceGetVersionResponse))});
}

void BoardConnection::enqueue(const Request &request)
{
    m_queue.append(request);
    sendNext();
}

void BoardConnection::sendNext()
{
    if (m_pending || m_queue.isEmpty() || !m_port.isOpen())
        return;
    m_pending = m_queue.takeFirst();
    m_attempts = 0;
    send();
}

void BoardConnection::send()
{
    m_received.clear();
    // Der ganze Frame mit einem write(): Eine Pause von mehr als 3,5 Zeichen
    // wertet die Firmware als Frame-Ende und verwirft beide Haelften.
    m_port.write(m_pending->frame);
    m_timeoutTimer.start();
}

void BoardConnection::receive()
{
    const QByteArray data = m_port.readAll();
    if (!m_pending)
        return; // nichts erwartet, z. B. verspaetete Antwort nach einem Neuverbinden
    m_received += data;

    // Eine Fehlerantwort (Funktion | IF_EXCEPTION) ist kuerzer als erwartet.
    int expected = m_pending->responseSize;
    if (m_received.size() >= 2 && (static_cast<quint8>(m_received[1]) & IF_EXCEPTION))
        expected = int(sizeof(InterfaceExceptionResponse));
    if (m_received.size() < expected)
        return;

    m_timeoutTimer.stop();
    const QByteArray response = m_received.left(expected);
    m_received.clear();
    handleResponse(response);
}

void BoardConnection::handleResponse(const QByteArray &response)
{
    const Request request = *m_pending;
    const quint8 function = static_cast<quint8>(response[1]);
    if (!checksumValid(response) || static_cast<quint8>(response[0]) != kAddress
        || (function != request.function && function != (request.function | IF_EXCEPTION))) {
        timeout(); // wie keine Antwort behandeln: nochmal senden
        return;
    }
    m_pending.reset();

    if (function & IF_EXCEPTION) {
        // Die Verbindung steht, aber die Firmware kann mit der Anfrage nichts anfangen.
        setStatus(Health::Error, QStringLiteral("firmware rejected function %1 (code %2)")
                                     .arg(request.function).arg(static_cast<quint8>(response[2])));
        sendNext();
        return;
    }

    // Nach einer abgelehnten Anfrage ist wieder alles gut, sobald eine andere durchgeht.
    if (m_state == State::Connected && m_health == Health::Error)
        setStatus(Health::Connected, QStringLiteral("connected"));

    if (function == IF_GET_MAGIC) {
        if (valueAt(response, offsetof(InterfaceGetMagicResponse, magic)) != INTERFACE_MAGIC) {
            close(Health::Error, QStringLiteral("wrong firmware"), kIncompatibleRetryMs);
            return;
        }
    } else if (function == IF_GET_VERSION) {
        const qint32 version = valueAt(response, offsetof(InterfaceGetVersionResponse, version));
        if (version != INTERFACE_VERSION) {
            close(Health::Error, QStringLiteral("version %1, GUI expects %2").arg(version).arg(INTERFACE_VERSION),
                  kIncompatibleRetryMs);
            return;
        }
        m_state = State::Connected;
        setStatus(Health::Connected, QStringLiteral("connected"));
        if (m_pollMask != 0)
            m_pollTimer.start();
        emit connected();
    } else if (function == IF_GET) {
        Values values;
        std::size_t offset = sizeof(InterfaceGetResponseBegin);
        for (int bit = 0; bit < 16; ++bit) {
            const quint16 field = quint16(1u << bit);
            if (!(request.mask & field))
                continue;
            values.insert(field, valueAt(response, offset));
            offset += sizeof(qint32);
        }
        emit valuesReceived(values);
    }

    sendNext();
}

void BoardConnection::timeout()
{
    if (!m_pending)
        return;
    if (++m_attempts < kMaxAttempts) {
        send();
        return;
    }
    close(Health::Error,
          m_state == State::Handshake ? QStringLiteral("no answer (firmware flashed?)") : QStringLiteral("no answer"),
          kReopenMs);
}

void BoardConnection::setStatus(Health health, const QString &text)
{
    if (health == m_health && text == m_status)
        return;
    m_health = health;
    m_status = text;
    qInfo().noquote() << m_name + ':' << text;
    emit statusChanged(health, text);
}

// Waehrend des Verbindens bleibt ein Fehler stehen, bis er behoben ist. Sonst
// wuerde die Ampel bei jedem Neuversuch kurz von Gelb auf Rot springen.
BoardConnection::Health BoardConnection::connectingHealth() const
{
    return m_health == Health::Error ? Health::Error : Health::Disconnected;
}
