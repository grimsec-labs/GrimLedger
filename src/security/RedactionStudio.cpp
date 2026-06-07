#include "security/RedactionStudio.h"

#include <QRegularExpression>
#include <algorithm>

namespace RedactionStudio {

QVector<RedactionHit> detectHits(const QString& text) {
    QVector<RedactionHit> hits;
    for (const SecretFinding& finding : SecretScanner::scanText(text)) {
        RedactionHit hit;
        hit.start = finding.start;
        hit.length = finding.length;
        hit.label = finding.reason;
        hits.append(hit);
    }

    static const QRegularExpression emailRe(
        QStringLiteral(R"(\b[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}\b)"));
    auto it = emailRe.globalMatch(text);
    while (it.hasNext()) {
        const auto match = it.next();
        RedactionHit hit;
        hit.start = match.capturedStart();
        hit.length = match.capturedLength();
        hit.label = QStringLiteral("Email address");
        hits.append(hit);
    }

    static const QRegularExpression ipRe(
        QStringLiteral(R"(\b(?:\d{1,3}\.){3}\d{1,3}\b)"));
    it = ipRe.globalMatch(text);
    while (it.hasNext()) {
        const auto match = it.next();
        RedactionHit hit;
        hit.start = match.capturedStart();
        hit.length = match.capturedLength();
        hit.label = QStringLiteral("IP address");
        hits.append(hit);
    }

    static const QRegularExpression sealedRe(
        QStringLiteral(R"(\[\[sealed:[^\]]+\]\])"));
    it = sealedRe.globalMatch(text);
    while (it.hasNext()) {
        const auto match = it.next();
        RedactionHit hit;
        hit.start = match.capturedStart();
        hit.length = match.capturedLength();
        hit.label = QStringLiteral("Sealed block");
        hits.append(hit);
    }

    return hits;
}

QString applyRedactions(const QString& text, const QVector<RedactionHit>& hits) {
    QString output = text;
    QVector<RedactionHit> sorted = hits;
    std::sort(sorted.begin(), sorted.end(), [](const RedactionHit& a, const RedactionHit& b) {
        return a.start > b.start;
    });
    for (const RedactionHit& hit : sorted) {
        if (!hit.approved || hit.length <= 0) {
            continue;
        }
        output.replace(hit.start, hit.length, QString(hit.length, QLatin1Char('#')));
    }
    return output;
}

} // namespace RedactionStudio
