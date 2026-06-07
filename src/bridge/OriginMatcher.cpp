#include "bridge/OriginMatcher.h"

#include <QUrl>

namespace OriginMatcher {

namespace {

int effectivePort(const QUrl& url) {
    if (url.port() != -1) {
        return url.port();
    }
    if (url.scheme() == QStringLiteral("https")) {
        return 443;
    }
    if (url.scheme() == QStringLiteral("http")) {
        return 80;
    }
    return -1;
}

QUrl normalizeCredentialUrl(const QString& credentialUrl) {
    QUrl cred(credentialUrl.trimmed());
    if (!cred.isValid() || cred.scheme().isEmpty()) {
        cred = QUrl(QStringLiteral("https://") + credentialUrl.trimmed());
    }
    return cred;
}

} // namespace

QString normalizeHost(const QString& host) {
    return host.trimmed().toLower();
}

bool pageOriginMatchesCredentialUrl(
    const QString& pageOrigin,
    const QString& credentialUrl,
    bool allowSubdomains) {
    if (credentialUrl.trimmed().isEmpty()) {
        return false;
    }

    const QUrl page(pageOrigin);
    const QUrl cred = normalizeCredentialUrl(credentialUrl);
    if (!page.isValid() || !cred.isValid()) {
        return false;
    }

    if (page.scheme() != QStringLiteral("https")
        && page.scheme() != QStringLiteral("http")) {
        return false;
    }

    const QString pageHost = normalizeHost(page.host());
    const QString credHost = normalizeHost(cred.host());
    if (pageHost.isEmpty() || credHost.isEmpty()) {
        return false;
    }

    if (cred.scheme() == QStringLiteral("https") && page.scheme() != QStringLiteral("https")) {
        return false;
    }

    if (page.scheme() != cred.scheme()) {
        return false;
    }

    if (effectivePort(page) != effectivePort(cred)) {
        return false;
    }

    if (pageHost == credHost) {
        return true;
    }

    if (allowSubdomains && pageHost.endsWith('.' + credHost)) {
        return true;
    }

    return false;
}

} // namespace OriginMatcher
