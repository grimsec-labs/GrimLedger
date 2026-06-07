#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QHash>
#include <optional>

class QLocalSocket;

class BridgeFillCoordinator : public QObject {
    Q_OBJECT

public:
    struct PendingFill {
        QPointer<QLocalSocket> socket;
        qint64 credId = 0;
        QString origin;
        QString nonce;
        QString label;
        quint64 lockGeneration = 0;
    };

    explicit BridgeFillCoordinator(QObject* parent = nullptr);

    void setLockGeneration(quint64 generation);
    bool hasPendingFill() const;

    bool beginFill(const PendingFill& pending);
    void cancelAll();
    std::optional<PendingFill> takePending(QLocalSocket* socket);
    std::optional<PendingFill> pendingFor(QLocalSocket* socket) const;

signals:
    void confirmationRequested(
        const QString& label,
        const QString& origin,
        QLocalSocket* socket);

private:
    quint64 m_lockGeneration = 0;
    QHash<QLocalSocket*, PendingFill> m_pending;
};
