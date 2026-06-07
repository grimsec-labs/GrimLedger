#pragma once

#include <QString>

namespace OriginMatcher {

QString normalizeHost(const QString& host);
bool pageOriginMatchesCredentialUrl(const QString& pageOrigin, const QString& credentialUrl);

} // namespace OriginMatcher
