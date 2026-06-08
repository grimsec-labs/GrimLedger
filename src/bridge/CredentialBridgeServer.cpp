#include "bridge/CredentialBridgeServer.h"
#include "bridge/BridgeAuth.h"
#include "bridge/BridgeFillCoordinator.h"
#include "bridge/BridgeClipCoordinator.h"
#include "storage/NoteRepository.h"
#include "models/Note.h"
#include "bridge/OriginMatcher.h"
#include "models/CredentialSummary.h"
#include "models/FillTrustLevel.h"
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
    QString endpoint;
    if (BridgeAuth::readSessionEndpoint(endpoint)) {
        return endpoint;
    }
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

void CredentialBridgeServer::setNoteRepository(NoteRepository* notes) {
    m_notes = notes;
}

void CredentialBridgeServer::setFillCoordinator(BridgeFillCoordinator* coordinator) {
    m_fillCoordinator = coordinator;
}

void CredentialBridgeServer::setClipCoordinator(BridgeClipCoordinator* coordinator) {
    m_clipCoordinator = coordinator;
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

void CredentialBridgeServer::setClipEnabledChecker(ClipEnabledFn checker) {
    m_clipEnabled = std::move(checker);
}

bool CredentialBridgeServer::start() {
    stop();

    m_sessionToken = BridgeAuth::generateToken();
    m_endpoint = BridgeAuth::generateSessionEndpoint();
    if (!BridgeAuth::writeSessionToken(m_sessionToken)
        || !BridgeAuth::writeSessionEndpoint(m_endpoint)) {
        emit listenFailed(QStringLiteral("Could not write bridge session token."));
        return false;
    }

    // GL-SEC-004: restrict the local socket to the current user so other local
    // accounts cannot connect to the bridge (defense in depth alongside the token).
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(m_endpoint);
    if (!m_server->listen(m_endpoint)) {
        BridgeAuth::clearSessionToken();
        m_sessionToken.clear();
        m_endpoint.clear();
        emit listenFailed(QStringLiteral("Could not listen on bridge endpoint."));
        return false;
    }
    return true;
}

bool CredentialBridgeServer::isListening() const {
    return m_server && m_server->isListening();
}

void CredentialBridgeServer::stop() {
    cancelPendingRequests();
    ++m_lockGeneration;
    if (m_fillCoordinator) {
        m_fillCoordinator->setLockGeneration(m_lockGeneration);
    }

    if (m_server->isListening()) {
        m_server->close();
    }
    if (!m_endpoint.isEmpty()) {
        QLocalServer::removeServer(m_endpoint);
    }
    m_lineBuffers.clear();
    m_activeClients = 0;
    m_sessionToken.clear();
    m_endpoint.clear();
    BridgeAuth::clearSessionToken();
}

void CredentialBridgeServer::cancelPendingRequests() {
    if (m_fillCoordinator) {
        m_fillCoordinator->cancelAll();
    }
    if (m_clipCoordinator) {
        m_clipCoordinator->cancelAll();
    }
    for (auto it = m_lineBuffers.begin(); it != m_lineBuffers.end(); ++it) {
        if (QLocalSocket* socket = it.key()) {
            writeResponse(socket, errorResponse(QStringLiteral("Request cancelled.")));
        }
    }
}

bool CredentialBridgeServer::validateToken(const QJsonObject& req) const {
    const QString token = req.value(QStringLiteral("token")).toString();
    if (token.isEmpty() || m_sessionToken.isEmpty()) {
        return false;
    }
    const auto decoded = BridgeAuth::tokenFromTransportString(token);
    if (!decoded) {
        return false;
    }
    return BridgeAuth::constantTimeEquals(m_sessionToken, *decoded);
}

void CredentialBridgeServer::writeResponse(QLocalSocket* socket, const QJsonObject& response) {
    if (!socket) {
        return;
    }
    const QByteArray payload = QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n';
    qint64 written = 0;
    while (written < payload.size()) {
        const qint64 chunk = socket->write(payload.constData() + written, payload.size() - written);
        if (chunk <= 0) {
            return;
        }
        written += chunk;
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
        if (m_fillCoordinator) {
            m_fillCoordinator->takePending(client);
        }
        if (m_clipCoordinator) {
            m_clipCoordinator->takePending(client);
        }
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
            if (!fillTrustAllowsBridgeListing(cred.fillTrustLevel)) {
                continue;
            }
            if (!OriginMatcher::pageOriginMatchesCredentialUrl(
                    origin, cred.url, cred.fillTrustLevel)) {
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
        if (!m_fillCoordinator) {
            writeResponse(socket, errorResponse(QStringLiteral("Fill confirmation is not configured.")));
            return;
        }
        if (m_fillCoordinator->hasPendingFill()) {
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
        if (!fillTrustAllowsBridgeListing(cred->fillTrustLevel)) {
            writeResponse(socket, errorResponse(QStringLiteral("Credential is manual-only.")));
            return;
        }
        if (!OriginMatcher::pageOriginMatchesCredentialUrl(
                origin, cred->url, cred->fillTrustLevel)) {
            writeResponse(socket, errorResponse(QStringLiteral("Origin does not match credential URL.")));
            return;
        }

        BridgeFillCoordinator::PendingFill pending;
        pending.socket = socket;
        pending.credId = id;
        pending.origin = origin;
        pending.nonce = nonce;
        pending.label = cred->label;
        pending.lockGeneration = m_lockGeneration;
        if (!m_fillCoordinator->beginFill(pending)) {
            writeResponse(socket, errorResponse(QStringLiteral("Could not start fill confirmation.")));
        }
        return;
    }

    if (action == QStringLiteral("clip")) {
        if (!m_clipCoordinator || !m_notes) {
            writeResponse(socket, errorResponse(QStringLiteral("Clip capture is not configured.")));
            return;
        }
        if (!m_clipEnabled || !m_clipEnabled()) {
            writeResponse(socket, errorResponse(QStringLiteral("Web clipper is disabled.")));
            return;
        }
        if (m_clipCoordinator->hasPendingClip()) {
            writeResponse(socket, errorResponse(QStringLiteral("Another clip is already pending.")));
            return;
        }

        const QString title = req.value(QStringLiteral("title")).toString().left(200);
        const QString url = req.value(QStringLiteral("url")).toString().left(2000);
        const QString text = req.value(QStringLiteral("text")).toString().left(16 * 1024);
        const QString origin = req.value(QStringLiteral("origin")).toString();
        if (title.isEmpty() || text.isEmpty() || origin.isEmpty()) {
            writeResponse(socket, errorResponse(QStringLiteral("Missing clip title, text, or origin.")));
            return;
        }

        BridgeClipCoordinator::PendingClip pending;
        pending.socket = socket;
        pending.title = title;
        pending.url = url;
        pending.text = text;
        pending.textPreview = text.left(240);
        pending.origin = origin;
        pending.lockGeneration = m_lockGeneration;
        if (!m_clipCoordinator->beginClip(pending)) {
            writeResponse(socket, errorResponse(QStringLiteral("Could not start clip confirmation.")));
        }
        return;
    }

    writeResponse(socket, errorResponse(QStringLiteral("Unknown action.")));
}

void CredentialBridgeServer::completeClipDecision(QLocalSocket* socket, bool approved, qint64 folderId) {
    if (!m_clipCoordinator || !m_notes || !m_sessionKey) {
        return;
    }
    const auto pendingOpt = m_clipCoordinator->takePending(socket);
    if (!pendingOpt) {
        return;
    }
    const BridgeClipCoordinator::PendingClip pending = *pendingOpt;
    if (!socket || socket->state() != QLocalSocket::ConnectedState) {
        return;
    }
    if (pending.lockGeneration != m_lockGeneration || !approved) {
        writeResponse(socket, errorResponse(QStringLiteral("Clip denied or vault locked.")));
        return;
    }

    const QByteArray key = m_sessionKey();
    if (key.isEmpty()) {
        writeResponse(socket, errorResponse(QStringLiteral("Vault is locked.")));
        return;
    }

    Note note;
    note.title = pending.title;
    note.body = QStringLiteral("# %1\n\nSource: %2\n\n%3")
        .arg(pending.title, pending.url, pending.text);
    note.folderId = folderId > 0 ? folderId : m_notes->defaultFolderId();
    const qint64 noteId = m_notes->createNote(note, key);
    if (noteId <= 0) {
        writeResponse(socket, errorResponse(QStringLiteral("Could not save clipped note.")));
        return;
    }

    QJsonObject resp;
    resp.insert(QStringLiteral("ok"), true);
    resp.insert(QStringLiteral("noteId"), static_cast<double>(noteId));
    writeResponse(socket, resp);
}

void CredentialBridgeServer::completeFillDecision(QLocalSocket* socket, bool approved) {
    if (!m_fillCoordinator) {
        return;
    }
    const auto pendingOpt = m_fillCoordinator->takePending(socket);
    if (!pendingOpt) {
        return;
    }
    const BridgeFillCoordinator::PendingFill pending = *pendingOpt;

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
            pending.origin, cred->url, cred->fillTrustLevel)) {
        writeResponse(socket, errorResponse(QStringLiteral("Origin no longer matches.")));
        return;
    }

    QJsonObject resp;
    resp.insert(QStringLiteral("ok"), true);
    resp.insert(QStringLiteral("nonce"), pending.nonce);
    resp.insert(QStringLiteral("username"), cred->username);
    if (fillTrustSendsPassword(cred->fillTrustLevel)) {
        resp.insert(QStringLiteral("password"), cred->password);
    } else {
        resp.insert(QStringLiteral("password"), QString());
    }
    writeResponse(socket, resp);
}
