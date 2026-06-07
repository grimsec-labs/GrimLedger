#include "bridge/BridgeClipCoordinator.h"

#include <QLocalSocket>

BridgeClipCoordinator::BridgeClipCoordinator(QObject* parent)
    : QObject(parent) {}

void BridgeClipCoordinator::setLockGeneration(quint64 generation) {
    m_lockGeneration = generation;
}

bool BridgeClipCoordinator::hasPendingClip() const {
    return !m_pending.isEmpty();
}

bool BridgeClipCoordinator::beginClip(const PendingClip& pending) {
    if (!pending.socket) {
        return false;
    }
    if (m_pending.contains(pending.socket.data())) {
        return false;
    }
    m_pending.insert(pending.socket.data(), pending);
    emit confirmationRequested(pending.title, pending.origin, pending.textPreview, pending.socket.data());
    return true;
}

void BridgeClipCoordinator::cancelAll() {
    m_pending.clear();
}

std::optional<BridgeClipCoordinator::PendingClip> BridgeClipCoordinator::takePending(
    QLocalSocket* socket) {
    if (!socket || !m_pending.contains(socket)) {
        return std::nullopt;
    }
    const PendingClip pending = m_pending.take(socket);
    return pending;
}
