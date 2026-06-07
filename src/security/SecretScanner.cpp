#include "security/SecretScanner.h"

#include <QRegularExpression>

namespace SecretScanner {

namespace {

void addMatch(QVector<SecretFinding>& out, const QString& text, const QRegularExpressionMatch& match, const QString& reason) {
    SecretFinding finding;
    finding.reason = reason;
    finding.start = match.capturedStart();
    finding.length = match.capturedLength();
    finding.snippet = text.mid(finding.start, qMin(finding.length, 48));
    out.append(finding);
}

} // namespace

QVector<SecretFinding> scanText(const QString& text) {
    QVector<SecretFinding> findings;
    if (text.isEmpty()) {
        return findings;
    }

    const QRegularExpression patterns[] = {
        QRegularExpression(QStringLiteral(R"(-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----)"),
            QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral(R"(AKIA[0-9A-Z]{16})")),
        QRegularExpression(QStringLiteral(R"((?i)(?:api[_-]?key|secret|token|password)\s*[:=]\s*['\"]?[A-Za-z0-9_\-]{12,})")),
        QRegularExpression(QStringLiteral(R"((?:mongodb|postgres|mysql)://[^\s:]+:[^\s@]+@)"),
            QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral(R"(\b[0-9a-f]{32,64}\b)")),
    };
    const QString reasons[] = {
        QStringLiteral("Private key header"),
        QStringLiteral("AWS access key id"),
        QStringLiteral("Credential-like assignment"),
        QStringLiteral("Connection string with password"),
        QStringLiteral("High-entropy hex string"),
    };

    for (int i = 0; i < 5; ++i) {
        auto it = patterns[i].globalMatch(text);
        while (it.hasNext()) {
            addMatch(findings, text, it.next(), reasons[i]);
        }
    }

    return findings;
}

QString redactSnippet(const QString& snippet) {
    return QString(snippet.size(), QLatin1Char('#'));
}

} // namespace SecretScanner
