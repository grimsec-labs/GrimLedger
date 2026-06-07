#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QHash>
#include <optional>

class QLocalSocket;

class BridgeClipCoordinator : public QObject {
    Q_OBJECT

public:
    struct PendingClip {
        QPointer<QLocalSocket> socket;
        QString title;
        QString url;
        QString text;
        QString textPreview;
        QString origin;
        quint64 lockGeneration = 0;
    };

    explicit BridgeClipCoordinator(QObject* parent = nullptr);

    void setLockGeneration(quint64 generation);
    bool hasPendingClip() const;
    bool beginClip(const PendingClip& pending);
    void cancelAll();
    std::optional<PendingClip> takePending(QLocalSocket* socket);

signals:
    void confirmationRequested(
        const QString& title,
        const QString& origin,
        const QString& preview,
        QLocalSocket* socket);

private:
    quint64 m_lockGeneration = 0;
    QHash<QLocalSocket*, PendingClip> m_pending;
};
