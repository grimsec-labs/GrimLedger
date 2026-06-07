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
    QLocalSocket socket;
    socket.connectToServer(QStringLiteral("grimledger-bridge"));
    if (!socket.waitForConnected(2000)) {
        QJsonObject err;
        err.insert(QStringLiteral("ok"), false);
        err.insert(QStringLiteral("error"), QStringLiteral("GrimLedger is not running or bridge is offline."));
        return QJsonDocument(err).toJson(QJsonDocument::Compact);
    }

    socket.write(requestLine.trimmed() + '\n');
    socket.flush();
    if (!socket.waitForReadyRead(10000)) {
        QJsonObject err;
        err.insert(QStringLiteral("ok"), false);
        err.insert(QStringLiteral("error"), QStringLiteral("GrimLedger bridge timed out."));
        return QJsonDocument(err).toJson(QJsonDocument::Compact);
    }

    while (socket.canReadLine() || socket.waitForReadyRead(200)) {
        const QByteArray line = socket.readLine().trimmed();
        if (!line.isEmpty()) {
            socket.disconnectFromServer();
            return line;
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
        writeNativeMessage(response);
    }

    return 0;
}
