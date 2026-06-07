#include "bridge/CredentialBridgeServer.h"
#include "bridge/BridgeAuth.h"
#include "bridge/OriginMatcher.h"
#include "models/CredentialSummary.h"
#include "storage/CredentialRepository.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>

namespace {

constexpr int kMaxLineBytes = 64 * 1024;
constexpr int kMaxClients = 4;
constexpr int kMaxMatches = 64;

QJsonObject errorResponse(const QString& message) {
    QJsonObject obj;
    obj.insert(QStringLiteral("ok"), false);
    obj.insert(QStringLiteral("error"), message);
    return obj;
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
    return BridgeAuth::endpointName();
}

QByteArray CredentialBridgeServer::currentSessionToken() {
    QByteArray token;
    if (BridgeAuth::readSessionToken(token)) {
        return token;
    }
    return QByteArray();
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

void CredentialBridgeServer::setBridgeEnabledChecker(BridgeEnabledFn checker) {
    m_bridgeEnabled = std::move(checker);
}

bool CredentialBridgeServer::start() {
    stop();

    m_sessionToken = BridgeAuth::generateToken();
    if (!BridgeAuth::writeSessionToken(m_sessionToken)) {
        emit listenFailed(QStringLiteral("Could not write bridge session token."));
        return false;
    }

    QLocalServer::removeServer(serverName());
    if (!m_server->listen(serverName())) {
        BridgeAuth::clearSessionToken();
        m_sessionToken.clear();
        emit listenFailed(QStringLiteral("Could not listen on bridge endpoint."));
        return false;
    }
    return true;
}

void CredentialBridgeServer::stop() {
    cancelPendingRequests();
    ++m_lockGeneration;

    if (m_server->isListening()) {
        m_server->close();
    }
    QLocalServer::removeServer(serverName());
    m_lineBuffers.clear();
    m_activeClients = 0;
    m_sessionToken.clear();
    BridgeAuth::clearSessionToken();
}

void CredentialBridgeServer::cancelPendingRequests() {
    for (auto it = m_pendingFills.begin(); it != m_pendingFills.end(); ++it) {
        if (QLocalSocket* socket = it.key()) {
            writeResponse(socket, errorResponse(QStringLiteral("Request cancelled.")));
        }
    }
    m_pendingFills.clear();
}

bool CredentialBridgeServer::validateToken(const QJsonObject& req) const {
    const QString token = req.value(QStringLiteral("token")).toString();
    if (token.isEmpty() || m_sessionToken.isEmpty()) {
        return false;
    }
    return BridgeAuth::constantTimeEquals(
        m_sessionToken,
        token.toUtf8());
}

void CredentialBridgeServer::writeResponse(QLocalSocket* socket, const QJsonObject& response) {
    if (!socket) {
        return;
    }
    const QByteArray payload = QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n';
    if (socket->write(payload) != payload.size()) {
        return;
    }
    socket->flush();
}

void CredentialBridgeServer::onNewConnection() {
    while (QLocalSocket* client = m_server->nextPendingConnection()) {
        if (m_activeClients >= kMaxClients) {
            writeResponse(client, errorResponse(QStringLiteral("Too many bridge clients.")));
            client->disconnectFromServer();
            client->deleteLater();
            continue;
        }
        ++m_activeClients;
        m_lineBuffers.insert(client, QByteArray());
        connect(client, &QLocalSocket::readyRead, this, &CredentialBridgeServer::onClientReadyRead);
        connect(client, &QLocalSocket::disconnected, this, &CredentialBridgeServer::onClientDisconnected);
        emit clientConnected();
    }
}

void CredentialBridgeServer::onClientDisconnected() {
    if (auto* client = qobject_cast<QLocalSocket*>(sender())) {
        m_pendingFills.remove(client);
        m_lineBuffers.remove(client);
        if (m_activeClients > 0) {
            --m_activeClients;
        }
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

    if (buffer.size() > kMaxLineBytes) {
        writeResponse(client, errorResponse(QStringLiteral("Request too large.")));
        client->disconnectFromServer();
        return;
    }

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

    if (!validateToken(req)) {
        writeResponse(socket, errorResponse(QStringLiteral("Unauthorized bridge request.")));
        return;
    }

    if (action == QStringLiteral("ping") || action == QStringLiteral("lock_status")) {
        const bool unlocked = m_isUnlocked && m_isUnlocked();
        QJsonObject resp;
        resp.insert(QStringLiteral("ok"), true);
        resp.insert(QStringLiteral("locked"), !unlocked);
        resp.insert(QStringLiteral("version"), 2);
        writeResponse(socket, resp);
        return;
    }

    if (!m_isUnlocked || !m_isUnlocked()) {
        writeResponse(socket, errorResponse(QStringLiteral("Vault is locked.")));
        return;
    }
    if (!m_bridgeEnabled || !m_bridgeEnabled()) {
        writeResponse(socket, errorResponse(QStringLiteral("Browser bridge is disabled.")));
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
        for (const CredentialSummary& cred : m_repository->listCredentialSummaries(key)) {
            if (!OriginMatcher::pageOriginMatchesCredentialUrl(
                    origin, cred.url, cred.allowSubdomains)) {
                continue;
            }
            QJsonObject item;
            item.insert(QStringLiteral("id"), static_cast<double>(cred.id));
            item.insert(QStringLiteral("label"), cred.label);
            item.insert(QStringLiteral("username"), cred.username);
            matches.append(item);
            if (matches.size() >= kMaxMatches) {
                break;
            }
        }

        QJsonObject resp;
        resp.insert(QStringLiteral("ok"), true);
        resp.insert(QStringLiteral("matches"), matches);
        writeResponse(socket, resp);
        return;
    }

    if (action == QStringLiteral("fill")) {
        if (!m_confirmFill) {
            writeResponse(socket, errorResponse(QStringLiteral("Fill confirmation is not configured.")));
            return;
        }
        if (!m_pendingFills.isEmpty()) {
            writeResponse(socket, errorResponse(QStringLiteral("Another fill is already pending.")));
            return;
        }

        const qint64 id = static_cast<qint64>(req.value(QStringLiteral("id")).toDouble());
        const QString origin = req.value(QStringLiteral("origin")).toString();
        const QString nonce = req.value(QStringLiteral("nonce")).toString();
        if (id <= 0 || origin.isEmpty() || nonce.isEmpty()) {
            writeResponse(socket, errorResponse(QStringLiteral("Missing id, origin, or nonce.")));
            return;
        }

        const auto cred = m_repository->getCredential(id, key);
        if (!cred) {
            writeResponse(socket, errorResponse(QStringLiteral("Credential not found.")));
            return;
        }
        if (cred->integrityError) {
            writeResponse(socket, errorResponse(QStringLiteral("Credential integrity error.")));
            return;
        }
        if (!OriginMatcher::pageOriginMatchesCredentialUrl(
                origin, cred->url, cred->allowSubdomains)) {
            writeResponse(socket, errorResponse(QStringLiteral("Origin does not match credential URL.")));
            return;
        }

        PendingFill pending;
        pending.socket = socket;
        pending.credId = id;
        pending.origin = origin;
        pending.label = cred->label;
        pending.lockGeneration = m_lockGeneration;
        m_pendingFills.insert(socket, pending);

        m_confirmFill(cred->label, origin, [this, socket](bool approved) {
            completePendingFill(socket, approved);
        });
        return;
    }

    writeResponse(socket, errorResponse(QStringLiteral("Unknown action.")));
}

void CredentialBridgeServer::completePendingFill(QLocalSocket* socket, bool approved) {
    const auto it = m_pendingFills.find(socket);
    if (it == m_pendingFills.end()) {
        return;
    }

    const PendingFill pending = it.value();
    m_pendingFills.erase(it);

    if (!socket || socket->state() != QLocalSocket::ConnectedState) {
        return;
    }
    if (pending.lockGeneration != m_lockGeneration) {
        writeResponse(socket, errorResponse(QStringLiteral("Vault locked during confirmation.")));
        return;
    }
    if (!m_isUnlocked || !m_isUnlocked()) {
        writeResponse(socket, errorResponse(QStringLiteral("Vault is locked.")));
        return;
    }
    if (!approved) {
        writeResponse(socket, errorResponse(QStringLiteral("Fill denied by user.")));
        return;
    }

    const QByteArray key = m_sessionKey ? m_sessionKey() : QByteArray();
    if (key.isEmpty()) {
        writeResponse(socket, errorResponse(QStringLiteral("Vault is locked.")));
        return;
    }

    const auto cred = m_repository->getCredential(pending.credId, key);
    if (!cred || cred->integrityError) {
        writeResponse(socket, errorResponse(QStringLiteral("Credential not available.")));
        return;
    }
    if (!OriginMatcher::pageOriginMatchesCredentialUrl(
            pending.origin, cred->url, cred->allowSubdomains)) {
        writeResponse(socket, errorResponse(QStringLiteral("Origin no longer matches.")));
        return;
    }

    QJsonObject resp;
    resp.insert(QStringLiteral("ok"), true);
    resp.insert(QStringLiteral("username"), cred->username);
    resp.insert(QStringLiteral("password"), cred->password);
    writeResponse(socket, resp);
}
