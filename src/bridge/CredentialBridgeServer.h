#pragma once

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QPointer>
#include <functional>

class QLocalServer;
class QLocalSocket;
class CredentialRepository;

class CredentialBridgeServer : public QObject {
    Q_OBJECT

public:
    using ConfirmFillFn = std::function<void(
        const QString& label,
        const QString& origin,
        std::function<void(bool approved)> callback)>;
    using IsUnlockedFn = std::function<bool()>;
    using SessionKeyFn = std::function<QByteArray()>;
    using BridgeEnabledFn = std::function<bool()>;

    explicit CredentialBridgeServer(QObject* parent = nullptr);
    ~CredentialBridgeServer() override;

    void setRepository(CredentialRepository* repository);
    void setConfirmFillHandler(ConfirmFillFn handler);
    void setUnlockedChecker(IsUnlockedFn checker);
    void setSessionKeyProvider(SessionKeyFn provider);
    void setBridgeEnabledChecker(BridgeEnabledFn checker);

    bool start();
    void stop();
    void cancelPendingRequests();

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
    struct PendingFill {
        QPointer<QLocalSocket> socket;
        qint64 credId = 0;
        QString origin;
        QString label;
        quint64 lockGeneration = 0;
    };

    bool validateToken(const QJsonObject& req) const;
    void writeResponse(QLocalSocket* socket, const QJsonObject& response);
    void handleRequest(QLocalSocket* socket, const QByteArray& line);
    void completePendingFill(QLocalSocket* socket, bool approved);

    QLocalServer* m_server = nullptr;
    CredentialRepository* m_repository = nullptr;
    ConfirmFillFn m_confirmFill;
    IsUnlockedFn m_isUnlocked;
    SessionKeyFn m_sessionKey;
    BridgeEnabledFn m_bridgeEnabled;
    QByteArray m_sessionToken;
    quint64 m_lockGeneration = 0;
    QHash<QLocalSocket*, QByteArray> m_lineBuffers;
    QHash<QLocalSocket*, PendingFill> m_pendingFills;
    int m_activeClients = 0;
};
