#pragma once

#include "models/ChamberId.h"

#include <QByteArray>
#include <QHash>
#include <QSet>

class ChamberManager {
public:
    void reset();
    void unlockAll(const QByteArray& masterKey);
    bool unlockChamber(ChamberId chamber, const QByteArray& masterKey);
    void lockChamber(ChamberId chamber);
    bool isChamberUnlocked(ChamberId chamber) const;
    QByteArray keyForChamber(ChamberId chamber) const;
    QByteArray keyForNote(int chamberId, const QByteArray& masterKey) const;

private:
    QByteArray m_masterKey;
    QSet<int> m_unlocked;
    QHash<int, QByteArray> m_chamberKeys;
};
