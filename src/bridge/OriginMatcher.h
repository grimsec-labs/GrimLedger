#pragma once

#include <QString>

namespace OriginMatcher {

QString normalizeHost(const QString& host);
bool pageOriginMatchesCredentialUrl(
    const QString& pageOrigin,
    const QString& credentialUrl,
    bool allowSubdomains = false);

} // namespace OriginMatcher
