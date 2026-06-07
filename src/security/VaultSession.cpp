#include "security/VaultSession.h"
#include "security/CryptoManager.h"

VaultSession::VaultSession(QObject* parent)
    : QObject(parent) {
    m_autoLockTimer.setSingleShot(true);
    connect(&m_autoLockTimer, &QTimer::timeout, this, &VaultSession::onAutoLockTimeout);
}

const QByteArray& VaultSession::key() const {
    static const QByteArray empty;
    return m_key.has_value() ? *m_key : empty;
}

void VaultSession::setKey(QByteArray key) {
    if (m_key.has_value()) {
        CryptoManager::secureZero(*m_key);
    }
    m_key = std::move(key);
    resetActivityTimer();
}

void VaultSession::lock() {
    if (m_key.has_value()) {
        CryptoManager::secureZero(*m_key);
        m_key.reset();
    }
    m_autoLockTimer.stop();
    emit locked();
}

void VaultSession::setAutoLockEnabled(bool enabled) {
    m_autoLockEnabled = enabled;
    if (!enabled) {
        m_autoLockTimer.stop();
    } else if (isUnlocked()) {
        resetActivityTimer();
    }
}

void VaultSession::setAutoLockMinutes(int minutes) {
    m_autoLockMinutes = qMax(1, minutes);
    if (isUnlocked() && m_autoLockEnabled) {
        resetActivityTimer();
    }
}

void VaultSession::resetActivityTimer() {
    if (!isUnlocked() || !m_autoLockEnabled) {
        return;
    }
    m_autoLockTimer.start(m_autoLockMinutes * 60 * 1000);
    emit activityReset();
}

void VaultSession::onAutoLockTimeout() {
    lock();
}
