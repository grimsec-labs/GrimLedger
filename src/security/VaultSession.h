#pragma once

#include <QByteArray>
#include <QObject>
#include <QTimer>
#include <optional>

// Holds the derived vault key in memory while unlocked.
// Clears key on lock. Emits signals for auto-lock.
class VaultSession : public QObject {
    Q_OBJECT

public:
    explicit VaultSession(QObject* parent = nullptr);

    bool isUnlocked() const { return m_key.has_value(); }

    const QByteArray& key() const;
    void setKey(QByteArray key);

    void lock();

    void setAutoLockEnabled(bool enabled);
    void setAutoLockMinutes(int minutes);
    bool autoLockEnabled() const { return m_autoLockEnabled; }
    int autoLockMinutes() const { return m_autoLockMinutes; }

    void resetActivityTimer();

signals:
    void locked();
    void activityReset();

private slots:
    void onAutoLockTimeout();

private:
    std::optional<QByteArray> m_key;
    QTimer m_autoLockTimer;
    bool m_autoLockEnabled = true;
    int m_autoLockMinutes = 15;
};
