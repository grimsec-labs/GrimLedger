#include "search/SemanticSearch.h"

#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace SemanticSearch {

namespace {

QHash<QString, QStringList> synonymMap() {
    return {
        {QStringLiteral("ssh"), {QStringLiteral("secure shell"), QStringLiteral("remote"), QStringLiteral("login")}},
        {QStringLiteral("breach"), {QStringLiteral("leak"), QStringLiteral("compromise"), QStringLiteral("exposed")}},
        {QStringLiteral("malware"), {QStringLiteral("virus"), QStringLiteral("trojan"), QStringLiteral("payload")}},
        {QStringLiteral("firewall"), {QStringLiteral("iptables"), QStringLiteral("network"), QStringLiteral("acl")}},
    };
}

QVector<QString> tokenize(const QString& text) {
    static const QRegularExpression wordRe(QStringLiteral(R"([A-Za-z0-9_\-]{3,})"));
    QVector<QString> tokens;
    auto it = wordRe.globalMatch(text.toLower());
    while (it.hasNext()) {
        tokens.append(it.next().captured(0));
    }
    return tokens;
}

} // namespace

QVector<QString> expandQueryTerms(const QString& query) {
    QSet<QString> terms;
    for (const QString& token : tokenize(query)) {
        terms.insert(token);
        const QHash<QString, QStringList> synonyms = synonymMap();
        if (synonyms.contains(token)) {
            for (const QString& related : synonyms.value(token)) {
                for (const QString& part : tokenize(related)) {
                    terms.insert(part);
                }
            }
        }
    }
    return terms.values();
}

double scoreBody(const QString& body, const QVector<QString>& terms) {
    if (terms.isEmpty()) {
        return 0.0;
    }
    const QVector<QString> bodyTokens = tokenize(body);
    if (bodyTokens.isEmpty()) {
        return 0.0;
    }
    int hits = 0;
    for (const QString& term : terms) {
        for (const QString& token : bodyTokens) {
            if (token.contains(term)) {
                ++hits;
                break;
            }
        }
    }
    return static_cast<double>(hits) / static_cast<double>(terms.size());
}

QVector<SemanticMatch> search(
    const QVector<Note>& notes,
    const QHash<qint64, QString>& bodiesById,
    const QString& query) {
    const QVector<QString> terms = expandQueryTerms(query);
    QVector<SemanticMatch> matches;
    for (const Note& note : notes) {
        const QString body = bodiesById.value(note.id, note.body);
        const QString corpus = note.title + QLatin1Char(' ') + body + QLatin1Char(' ')
            + note.tags.join(QLatin1Char(' '));
        const double score = scoreBody(corpus, terms);
        if (score > 0.0) {
            SemanticMatch match;
            match.note = note;
            match.score = score;
            matches.append(match);
        }
    }
    std::sort(matches.begin(), matches.end(), [](const SemanticMatch& a, const SemanticMatch& b) {
        return a.score > b.score;
    });
    return matches;
}

bool buildIndexEntry(
    const QString& body,
    QByteArray& encryptedTokensOut,
    const QByteArray& key,
    qint64 noteId) {
    Q_UNUSED(key);
    Q_UNUSED(noteId);
    encryptedTokensOut = tokenize(body).join(QLatin1Char(' ')).toUtf8();
    return true;
}

} // namespace SemanticSearch
