#include "bridge/CredentialBridgeServer.h"
#include "bridge/OriginMatcher.h"
#include "models/Credential.h"
#include "storage/CredentialRepository.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>

namespace {

QJsonObject errorResponse(const QString& message) {
    QJsonObject obj;
    obj.insert(QStringLiteral("ok"), false);
    obj.insert(QStringLiteral("error"), message);
    return obj;
}

void writeResponse(QLocalSocket* socket, const QJsonObject& response) {
    const QByteArray payload = QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n';
    socket->write(payload);
    socket->flush();
}

} // namespace

CredentialBridgeServer::CredentialBridgeServer(QObject* parent)
    : QObject(parent)
    , m_server(new QLocalServer(this)) {
    connect(m_server, &QLocalServer::newConnection, this, &CredentialBridgeServer::onNewConnection);
}

CredentialBridgeServer::~CredentialBridgeServer() {
    stop();
}

QString CredentialBridgeServer::serverName() {
    return QStringLiteral("grimledger-bridge");
}

void CredentialBridgeServer::setRepository(CredentialRepository* repository) {
    m_repository = repository;
}

void CredentialBridgeServer::setConfirmFillHandler(ConfirmFillFn handler) {
    m_confirmFill = std::move(handler);
}

void CredentialBridgeServer::setUnlockedChecker(IsUnlockedFn checker) {
    m_isUnlocked = std::move(checker);
}

void CredentialBridgeServer::setSessionKeyProvider(SessionKeyFn provider) {
    m_sessionKey = std::move(provider);
}

bool CredentialBridgeServer::start() {
    stop();
    QLocalServer::removeServer(serverName());
    if (!m_server->listen(serverName())) {
        return false;
    }
    return true;
}

void CredentialBridgeServer::stop() {
    if (m_server->isListening()) {
        m_server->close();
    }
    QLocalServer::removeServer(serverName());
    m_lineBuffers.clear();
}

void CredentialBridgeServer::onNewConnection() {
    while (QLocalSocket* client = m_server->nextPendingConnection()) {
        m_lineBuffers.insert(client, QByteArray());
        connect(client, &QLocalSocket::readyRead, this, &CredentialBridgeServer::onClientReadyRead);
        connect(client, &QLocalSocket::disconnected, this, &CredentialBridgeServer::onClientDisconnected);
        emit clientConnected();
    }
}

void CredentialBridgeServer::onClientDisconnected() {
    if (auto* client = qobject_cast<QLocalSocket*>(sender())) {
        m_lineBuffers.remove(client);
        client->deleteLater();
    }
}

void CredentialBridgeServer::onClientReadyRead() {
    auto* client = qobject_cast<QLocalSocket*>(sender());
    if (!client) {
        return;
    }

    m_lineBuffers[client] += client->readAll();
    QByteArray& buffer = m_lineBuffers[client];

    while (true) {
        const int newline = buffer.indexOf('\n');
        if (newline < 0) {
            break;
        }
        const QByteArray line = buffer.left(newline).trimmed();
        buffer.remove(0, newline + 1);
        if (!line.isEmpty()) {
            handleRequest(client, line);
        }
    }
}

void CredentialBridgeServer::handleRequest(QLocalSocket* socket, const QByteArray& line) {
    const QJsonDocument doc = QJsonDocument::fromJson(line);
    if (!doc.isObject()) {
        writeResponse(socket, errorResponse(QStringLiteral("Invalid JSON request.")));
        return;
    }

    const QJsonObject req = doc.object();
    const QString action = req.value(QStringLiteral("action")).toString();

    if (action == QStringLiteral("ping") || action == QStringLiteral("lock_status")) {
        const bool unlocked = m_isUnlocked && m_isUnlocked();
        QJsonObject resp;
        resp.insert(QStringLiteral("ok"), true);
        resp.insert(QStringLiteral("locked"), !unlocked);
        resp.insert(QStringLiteral("version"), 1);
        writeResponse(socket, resp);
        return;
    }

    if (!m_isUnlocked || !m_isUnlocked()) {
        writeResponse(socket, errorResponse(QStringLiteral("Vault is locked.")));
        return;
    }
    if (!m_repository || !m_sessionKey) {
        writeResponse(socket, errorResponse(QStringLiteral("Bridge is not ready.")));
        return;
    }

    const QByteArray key = m_sessionKey();
    if (key.isEmpty()) {
        writeResponse(socket, errorResponse(QStringLiteral("Vault is locked.")));
        return;
    }

    if (action == QStringLiteral("list_matches")) {
        const QString origin = req.value(QStringLiteral("origin")).toString();
        if (origin.isEmpty()) {
            writeResponse(socket, errorResponse(QStringLiteral("Missing origin.")));
            return;
        }

        QJsonArray matches;
        for (const Credential& cred : m_repository->listCredentials(key)) {
            if (!OriginMatcher::pageOriginMatchesCredentialUrl(origin, cred.url)) {
                continue;
            }
            QJsonObject item;
            item.insert(QStringLiteral("id"), static_cast<double>(cred.id));
            item.insert(QStringLiteral("label"), cred.label);
            item.insert(QStringLiteral("username"), cred.username);
            matches.append(item);
        }

        QJsonObject resp;
        resp.insert(QStringLiteral("ok"), true);
        resp.insert(QStringLiteral("matches"), matches);
        writeResponse(socket, resp);
        return;
    }

    if (action == QStringLiteral("fill")) {
        const qint64 id = static_cast<qint64>(req.value(QStringLiteral("id")).toDouble());
        const QString origin = req.value(QStringLiteral("origin")).toString();
        if (id <= 0 || origin.isEmpty()) {
            writeResponse(socket, errorResponse(QStringLiteral("Missing id or origin.")));
            return;
        }

        const auto cred = m_repository->getCredential(id, key);
        if (!cred) {
            writeResponse(socket, errorResponse(QStringLiteral("Credential not found.")));
            return;
        }
        if (!OriginMatcher::pageOriginMatchesCredentialUrl(origin, cred->url)) {
            writeResponse(socket, errorResponse(QStringLiteral("Origin does not match credential URL.")));
            return;
        }

        if (m_confirmFill && !m_confirmFill(cred->label, origin)) {
            writeResponse(socket, errorResponse(QStringLiteral("Fill denied by user.")));
            return;
        }

        QJsonObject resp;
        resp.insert(QStringLiteral("ok"), true);
        resp.insert(QStringLiteral("username"), cred->username);
        resp.insert(QStringLiteral("password"), cred->password);
        writeResponse(socket, resp);
        return;
    }

    writeResponse(socket, errorResponse(QStringLiteral("Unknown action.")));
}
