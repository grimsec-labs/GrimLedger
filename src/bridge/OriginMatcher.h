#pragma once

#include "models/FillTrustLevel.h"

#include <QString>

namespace OriginMatcher {

QString normalizeHost(const QString& host);
bool pageOriginMatchesCredentialUrl(
    const QString& pageOrigin,
    const QString& credentialUrl,
    FillTrustLevel trustLevel = FillTrustLevel::ExactOrigin);

} // namespace OriginMatcher
