#include "bridge/BridgeFillCoordinator.h"

#include <QLocalSocket>

BridgeFillCoordinator::BridgeFillCoordinator(QObject* parent)
    : QObject(parent) {}

void BridgeFillCoordinator::setLockGeneration(quint64 generation) {
    m_lockGeneration = generation;
}

bool BridgeFillCoordinator::hasPendingFill() const {
    return !m_pending.isEmpty();
}

bool BridgeFillCoordinator::beginFill(const PendingFill& pending) {
    if (!pending.socket || pending.credId <= 0 || pending.origin.isEmpty()
        || pending.nonce.isEmpty() || !m_pending.isEmpty()) {
        return false;
    }
    m_pending.insert(pending.socket, pending);
    emit confirmationRequested(pending.label, pending.origin, pending.socket);
    return true;
}

void BridgeFillCoordinator::cancelAll() {
    m_pending.clear();
}

std::optional<BridgeFillCoordinator::PendingFill> BridgeFillCoordinator::takePending(
    QLocalSocket* socket) {
    if (!socket || !m_pending.contains(socket)) {
        return std::nullopt;
    }
    const PendingFill pending = m_pending.take(socket);
    return pending;
}

std::optional<BridgeFillCoordinator::PendingFill> BridgeFillCoordinator::pendingFor(
    QLocalSocket* socket) const {
    if (!socket || !m_pending.contains(socket)) {
        return std::nullopt;
    }
    return m_pending.value(socket);
}
