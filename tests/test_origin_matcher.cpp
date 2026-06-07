#include "bridge/OriginMatcher.h"

#include <QtCore/QString>

namespace {

void check(bool condition, const char* message) {
    if (!condition) {
        throw message;
    }
}

} // namespace

int main() {
    try {
        check(OriginMatcher::pageOriginMatchesCredentialUrl(
                  QStringLiteral("https://example.com"),
                  QStringLiteral("https://example.com")),
            "exact origin match");
        check(!OriginMatcher::pageOriginMatchesCredentialUrl(
                  QStringLiteral("http://example.com"),
                  QStringLiteral("https://example.com")),
            "http downgrade rejected");
        check(!OriginMatcher::pageOriginMatchesCredentialUrl(
                  QStringLiteral("https://app.example.com"),
                  QStringLiteral("https://example.com"),
                  false),
            "subdomain rejected by default");
        check(OriginMatcher::pageOriginMatchesCredentialUrl(
                  QStringLiteral("https://app.example.com"),
                  QStringLiteral("https://example.com"),
                  true),
            "subdomain allowed when enabled");
        check(!OriginMatcher::pageOriginMatchesCredentialUrl(
                  QStringLiteral("https://example.com:8443"),
                  QStringLiteral("https://example.com")),
            "port mismatch rejected");
    } catch (const char* message) {
        return message ? 1 : 1;
    }
    return 0;
}
