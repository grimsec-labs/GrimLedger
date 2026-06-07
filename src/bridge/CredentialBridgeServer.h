#pragma once

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <functional>

class QLocalServer;
class QLocalSocket;
class CredentialRepository;
class BridgeFillCoordinator;

class CredentialBridgeServer : public QObject {
    Q_OBJECT

public:
    using IsUnlockedFn = std::function<bool()>;
    using SessionKeyFn = std::function<QByteArray()>;
    using BridgeEnabledFn = std::function<bool()>;

    explicit CredentialBridgeServer(QObject* parent = nullptr);
    ~CredentialBridgeServer() override;

    void setRepository(CredentialRepository* repository);
    void setFillCoordinator(BridgeFillCoordinator* coordinator);
    void setUnlockedChecker(IsUnlockedFn checker);
    void setSessionKeyProvider(SessionKeyFn provider);
    void setBridgeEnabledChecker(BridgeEnabledFn checker);

    bool start();
    void stop();
    void cancelPendingRequests();
    void completeFillDecision(QLocalSocket* socket, bool approved);

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
    BridgeFillCoordinator* m_fillCoordinator = nullptr;
    IsUnlockedFn m_isUnlocked;
    SessionKeyFn m_sessionKey;
    BridgeEnabledFn m_bridgeEnabled;
    QByteArray m_sessionToken;
    QString m_endpoint;
    quint64 m_lockGeneration = 0;
    QHash<QLocalSocket*, QByteArray> m_lineBuffers;
    int m_activeClients = 0;
};
