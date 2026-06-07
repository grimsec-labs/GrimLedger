#pragma once

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <functional>

class QLocalServer;
class QLocalSocket;
class CredentialRepository;
class NoteRepository;
class BridgeFillCoordinator;
class BridgeClipCoordinator;

class CredentialBridgeServer : public QObject {
    Q_OBJECT

public:
    using IsUnlockedFn = std::function<bool()>;
    using SessionKeyFn = std::function<QByteArray()>;
    using BridgeEnabledFn = std::function<bool()>;
    using ClipEnabledFn = std::function<bool()>;

    explicit CredentialBridgeServer(QObject* parent = nullptr);
    ~CredentialBridgeServer() override;

    void setRepository(CredentialRepository* repository);
    void setNoteRepository(NoteRepository* notes);
    void setFillCoordinator(BridgeFillCoordinator* coordinator);
    void setClipCoordinator(BridgeClipCoordinator* coordinator);
    void setUnlockedChecker(IsUnlockedFn checker);
    void setSessionKeyProvider(SessionKeyFn provider);
    void setBridgeEnabledChecker(BridgeEnabledFn checker);
    void setClipEnabledChecker(ClipEnabledFn checker);

    bool start();
    void stop();
    bool isListening() const;
    void cancelPendingRequests();
    void completeFillDecision(QLocalSocket* socket, bool approved);
    void completeClipDecision(QLocalSocket* socket, bool approved, qint64 folderId = 0);

    static QString serverName();
    static QByteArray currentSessionToken();

signals:
    void clientConnected();
    void listenFailed(const QString& reason);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    bool validateToken(const QJsonObject& req) const;
    void writeResponse(QLocalSocket* socket, const QJsonObject& response);
    void handleRequest(QLocalSocket* socket, const QByteArray& line);

    QLocalServer* m_server = nullptr;
    CredentialRepository* m_repository = nullptr;
    NoteRepository* m_notes = nullptr;
    BridgeFillCoordinator* m_fillCoordinator = nullptr;
    BridgeClipCoordinator* m_clipCoordinator = nullptr;
    IsUnlockedFn m_isUnlocked;
    SessionKeyFn m_sessionKey;
    BridgeEnabledFn m_bridgeEnabled;
    ClipEnabledFn m_clipEnabled;
    QByteArray m_sessionToken;
    QString m_endpoint;
    quint64 m_lockGeneration = 0;
    QHash<QLocalSocket*, QByteArray> m_lineBuffers;
    int m_activeClients = 0;
};
