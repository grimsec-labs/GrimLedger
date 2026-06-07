#pragma once

#include <QString>

enum class FillTrustLevel : quint8 {
    ManualOnly = 0,
    UsernameOnly = 1,
    ExactOrigin = 2,
    AllowSubdomains = 3,
};

QString fillTrustLevelLabel(FillTrustLevel level);
FillTrustLevel fillTrustLevelFromInt(int value);
bool fillTrustAllowsBridgeListing(FillTrustLevel level);
bool fillTrustSendsPassword(FillTrustLevel level);
bool fillTrustAllowsSubdomains(FillTrustLevel level);
