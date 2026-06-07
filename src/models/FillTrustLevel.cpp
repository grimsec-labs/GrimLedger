#include "models/FillTrustLevel.h"

QString fillTrustLevelLabel(FillTrustLevel level) {
    switch (level) {
    case FillTrustLevel::ManualOnly:
        return QStringLiteral("Manual only (no browser fill)");
    case FillTrustLevel::UsernameOnly:
        return QStringLiteral("Username only via bridge");
    case FillTrustLevel::ExactOrigin:
        return QStringLiteral("Exact origin (recommended)");
    case FillTrustLevel::AllowSubdomains:
        return QStringLiteral("Allow subdomains");
    }
    return QStringLiteral("Exact origin (recommended)");
}

FillTrustLevel fillTrustLevelFromInt(int value) {
    if (value < 0 || value > static_cast<int>(FillTrustLevel::AllowSubdomains)) {
        return FillTrustLevel::ExactOrigin;
    }
    return static_cast<FillTrustLevel>(value);
}

bool fillTrustAllowsBridgeListing(FillTrustLevel level) {
    return level != FillTrustLevel::ManualOnly;
}

bool fillTrustSendsPassword(FillTrustLevel level) {
    return level != FillTrustLevel::ManualOnly && level != FillTrustLevel::UsernameOnly;
}

bool fillTrustAllowsSubdomains(FillTrustLevel level) {
    return level == FillTrustLevel::AllowSubdomains;
}
