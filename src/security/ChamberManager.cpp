#include "security/ChamberManager.h"
#include "security/CryptoManager.h"

void ChamberManager::reset() {
    for (QByteArray& key : m_chamberKeys) {
        CryptoManager::secureZero(key);
    }
    CryptoManager::secureZero(m_masterKey);
    m_chamberKeys.clear();
    m_unlocked.clear();
}

void ChamberManager::unlockAll(const QByteArray& masterKey) {
    reset();
    m_masterKey = masterKey;
    m_unlocked.insert(chamberIdValue(ChamberId::General));
    m_unlocked.insert(chamberIdValue(ChamberId::Credentials));
    m_unlocked.insert(chamberIdValue(ChamberId::Work));
    m_unlocked.insert(chamberIdValue(ChamberId::Personal));
    m_unlocked.insert(chamberIdValue(ChamberId::Casebook));
    for (int id : m_unlocked) {
        m_chamberKeys[id] = CryptoManager::deriveChamberKey(masterKey, id);
    }
}

bool ChamberManager::unlockChamber(ChamberId chamber, const QByteArray& masterKey) {
    if (masterKey.size() != CryptoManager::kKeySize) {
        return false;
    }
    const int id = chamberIdValue(chamber);
    const QByteArray derived = CryptoManager::deriveChamberKey(masterKey, id);
    if (derived.isEmpty()) {
        return false;
    }
    m_chamberKeys[id] = derived;
    m_unlocked.insert(id);
    if (m_masterKey.isEmpty()) {
        m_masterKey = masterKey;
    }
    return true;
}

void ChamberManager::lockChamber(ChamberId chamber) {
    const int id = chamberIdValue(chamber);
    if (m_chamberKeys.contains(id)) {
        CryptoManager::secureZero(m_chamberKeys[id]);
        m_chamberKeys.remove(id);
    }
    m_unlocked.remove(id);
}

bool ChamberManager::isChamberUnlocked(ChamberId chamber) const {
    return m_unlocked.contains(chamberIdValue(chamber));
}

QByteArray ChamberManager::keyForChamber(ChamberId chamber) const {
    return m_chamberKeys.value(chamberIdValue(chamber));
}

QByteArray ChamberManager::keyForNote(int chamberId, const QByteArray& masterKey) const {
    if (chamberId <= 0 || chamberId == chamberIdValue(ChamberId::General)) {
        return masterKey;
    }
    if (!m_unlocked.contains(chamberId)) {
        return QByteArray();
    }
    return m_chamberKeys.value(chamberId);
}
