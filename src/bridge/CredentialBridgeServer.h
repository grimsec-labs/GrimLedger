#pragma once

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <functional>

class QLocalServer;
class QLocalSocket;
class CredentialRepository;

class CredentialBridgeServer : public QObject {
    Q_OBJECT

public:
    using ConfirmFillFn = std::function<bool(const QString& label, const QString& origin)>;
    using IsUnlockedFn = std::function<bool()>;
    using SessionKeyFn = std::function<QByteArray()>;

    explicit CredentialBridgeServer(QObject* parent = nullptr);
    ~CredentialBridgeServer() override;

    void setRepository(CredentialRepository* repository);
    void setConfirmFillHandler(ConfirmFillFn handler);
    void setUnlockedChecker(IsUnlockedFn checker);
    void setSessionKeyProvider(SessionKeyFn provider);

    bool start();
    void stop();

    static QString serverName();

signals:
    void clientConnected();

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    QByteArray readLineBuffer(QLocalSocket* socket) const;
    void handleRequest(QLocalSocket* socket, const QByteArray& line);

    QLocalServer* m_server = nullptr;
    CredentialRepository* m_repository = nullptr;
    ConfirmFillFn m_confirmFill;
    IsUnlockedFn m_isUnlocked;
    SessionKeyFn m_sessionKey;
    QHash<QLocalSocket*, QByteArray> m_lineBuffers;
};
