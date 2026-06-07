#include "models/ChamberId.h"

QString chamberLabel(ChamberId chamber) {
    switch (chamber) {
    case ChamberId::General: return QStringLiteral("General");
    case ChamberId::Credentials: return QStringLiteral("Credentials");
    case ChamberId::Work: return QStringLiteral("Work");
    case ChamberId::Personal: return QStringLiteral("Personal");
    case ChamberId::Casebook: return QStringLiteral("Casebook");
    }
    return QStringLiteral("General");
}

int chamberIdValue(ChamberId chamber) {
    return static_cast<int>(chamber);
}
