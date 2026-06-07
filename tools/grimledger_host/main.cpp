#include "bridge/BridgeAuth.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

namespace {

void setBinaryStdio() {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
}

bool readExact(int fd, char* buffer, int size) {
    int total = 0;
    while (total < size) {
#ifdef _WIN32
        const int chunk = _read(fd, buffer + total, size - total);
#else
        const int chunk = static_cast<int>(read(fd, buffer + total, static_cast<size_t>(size - total)));
#endif
        if (chunk <= 0) {
            return false;
        }
        total += chunk;
    }
    return true;
}

bool writeExact(int fd, const char* buffer, int size) {
    int total = 0;
    while (total < size) {
#ifdef _WIN32
        const int chunk = _write(fd, buffer + total, size - total);
#else
        const int chunk = static_cast<int>(write(fd, buffer + total, static_cast<size_t>(size - total)));
#endif
        if (chunk <= 0) {
            return false;
        }
        total += chunk;
    }
    return true;
}

bool readNativeMessage(QByteArray& out) {
    char header[4] = {};
    if (!readExact(0, header, 4)) {
        return false;
    }

    const quint32 length = static_cast<quint8>(header[0])
        | (static_cast<quint32>(static_cast<quint8>(header[1])) << 8)
        | (static_cast<quint32>(static_cast<quint8>(header[2])) << 16)
        | (static_cast<quint32>(static_cast<quint8>(header[3])) << 24);

    if (length == 0 || length > 1024 * 1024) {
        return false;
    }

    out.resize(static_cast<int>(length));
    return readExact(0, out.data(), static_cast<int>(length));
}

bool writeNativeMessage(const QByteArray& payload) {
    const quint32 length = static_cast<quint32>(payload.size());
    const char header[4] = {
        static_cast<char>(length & 0xFF),
        static_cast<char>((length >> 8) & 0xFF),
        static_cast<char>((length >> 16) & 0xFF),
        static_cast<char>((length >> 24) & 0xFF),
    };

    return writeExact(1, header, 4) && writeExact(1, payload.constData(), payload.size());
}

QByteArray forwardToDesktop(const QByteArray& requestLine) {
    QByteArray sessionToken;
    if (!BridgeAuth::readSessionToken(sessionToken)) {
        QJsonObject err;
        err.insert(QStringLiteral("ok"), false);
        err.insert(QStringLiteral("error"), QStringLiteral("GrimLedger bridge session is offline."));
        return QJsonDocument(err).toJson(QJsonDocument::Compact);
    }

    QJsonDocument inDoc = QJsonDocument::fromJson(requestLine);
    if (!inDoc.isObject()) {
        QJsonObject err;
        err.insert(QStringLiteral("ok"), false);
        err.insert(QStringLiteral("error"), QStringLiteral("Invalid JSON."));
        return QJsonDocument(err).toJson(QJsonDocument::Compact);
    }

    QJsonObject req = inDoc.object();
    req.insert(QStringLiteral("token"), BridgeAuth::tokenToTransportString(sessionToken));

    QLocalSocket socket;
    QString endpoint;
    if (!BridgeAuth::readSessionEndpoint(endpoint)) {
        endpoint = BridgeAuth::endpointName();
    }
    socket.connectToServer(endpoint);
    if (!socket.waitForConnected(2000)) {
        QJsonObject err;
        err.insert(QStringLiteral("ok"), false);
        err.insert(QStringLiteral("error"), QStringLiteral("GrimLedger is not running or bridge is offline."));
        return QJsonDocument(err).toJson(QJsonDocument::Compact);
    }

    const QByteArray line = QJsonDocument(req).toJson(QJsonDocument::Compact) + '\n';
    qint64 written = 0;
    while (written < line.size()) {
        const qint64 chunk = socket.write(line.constData() + written, line.size() - written);
        if (chunk <= 0) {
            QJsonObject err;
            err.insert(QStringLiteral("ok"), false);
            err.insert(QStringLiteral("error"), QStringLiteral("Failed to write bridge request."));
            return QJsonDocument(err).toJson(QJsonDocument::Compact);
        }
        written += chunk;
    }
    socket.flush();
    if (!socket.waitForReadyRead(10000)) {
        QJsonObject err;
        err.insert(QStringLiteral("ok"), false);
        err.insert(QStringLiteral("error"), QStringLiteral("GrimLedger bridge timed out."));
        return QJsonDocument(err).toJson(QJsonDocument::Compact);
    }

    while (socket.canReadLine() || socket.waitForReadyRead(200)) {
        const QByteArray responseLine = socket.readLine().trimmed();
        if (!responseLine.isEmpty()) {
            socket.disconnectFromServer();
            return responseLine;
        }
    }

    QJsonObject err;
    err.insert(QStringLiteral("ok"), false);
    err.insert(QStringLiteral("error"), QStringLiteral("Empty bridge response."));
    return QJsonDocument(err).toJson(QJsonDocument::Compact);
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    setBinaryStdio();

    while (true) {
        QByteArray nativePayload;
        if (!readNativeMessage(nativePayload)) {
            break;
        }

        const QJsonDocument inDoc = QJsonDocument::fromJson(nativePayload);
        if (!inDoc.isObject()) {
            const QJsonObject err{
                {QStringLiteral("ok"), false},
                {QStringLiteral("error"), QStringLiteral("Invalid JSON.")}};
            writeNativeMessage(QJsonDocument(err).toJson(QJsonDocument::Compact));
            continue;
        }

        const QByteArray response = forwardToDesktop(
            QJsonDocument(inDoc.object()).toJson(QJsonDocument::Compact));
        if (!writeNativeMessage(response)) {
            break;
        }
    }

    return 0;
}
