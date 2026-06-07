#include "bridge/OriginMatcher.h"

#include <QUrl>

namespace OriginMatcher {

QString normalizeHost(const QString& host) {
    return host.trimmed().toLower();
}

bool pageOriginMatchesCredentialUrl(const QString& pageOrigin, const QString& credentialUrl) {
    if (credentialUrl.trimmed().isEmpty()) {
        return false;
    }

    const QUrl page(pageOrigin);
    QUrl cred(credentialUrl.trimmed());
    if (!cred.isValid() || cred.scheme().isEmpty()) {
        cred = QUrl(QStringLiteral("https://") + credentialUrl.trimmed());
    }
    if (!page.isValid() || !cred.isValid()) {
        return false;
    }

    const QString pageHost = normalizeHost(page.host());
    const QString credHost = normalizeHost(cred.host());
    if (pageHost.isEmpty() || credHost.isEmpty()) {
        return false;
    }

    if (page.scheme() != QStringLiteral("https")
        && page.scheme() != QStringLiteral("http")) {
        return false;
    }

    return pageHost == credHost || pageHost.endsWith('.' + credHost);
}

} // namespace OriginMatcher
