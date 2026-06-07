#include "search/SearchEngine.h"

#include <QRegularExpression>
#include <algorithm>

QVector<SearchMatch> SearchEngine::search(
    const QVector<Note>& notes,
    const QString& query,
    bool searchBodies,
    const QHash<qint64, QString>& bodiesById) {
    QVector<SearchMatch> results;
    if (query.trimmed().isEmpty()) {
        for (const Note& n : notes) {
            SearchMatch m;
            m.note = n;
            results.append(m);
        }
        return results;
    }

    const QString q = query.trimmed();
    const Qt::CaseSensitivity cs = Qt::CaseInsensitive;

    for (const Note& n : notes) {
        SearchMatch m;
        m.note = n;
        int score = 0;

        int pos = 0;
        while ((pos = n.title.indexOf(q, pos, cs)) != -1) {
            m.titleMatchPositions.append(pos);
            score += 10;
            pos += q.size();
        }

        for (const QString& tag : n.tags) {
            if (tag.contains(q, cs)) {
                score += 5;
            }
        }

        if (searchBodies) {
            const QString body = bodiesById.contains(n.id)
                ? bodiesById[n.id]
                : n.body;
            pos = 0;
            while ((pos = body.indexOf(q, pos, cs)) != -1) {
                m.bodyMatchPositions.append(pos);
                score += 1;
                pos += q.size();
            }
        }

        if (score > 0 || m.titleMatchPositions.isEmpty() == false) {
            m.score = score;
            results.append(m);
        } else if (!searchBodies && n.title.contains(q, cs)) {
            m.score = 1;
            results.append(m);
        }
    }

    std::sort(results.begin(), results.end(), [](const SearchMatch& a, const SearchMatch& b) {
        return a.score > b.score;
    });

    return results;
}

QString SearchEngine::highlightHtml(
    const QString& text,
    const QVector<int>& positions,
    const QString& query) {
    if (positions.isEmpty() || query.isEmpty()) {
        return text.toHtmlEscaped();
    }

    QString html;
    int last = 0;
    const int len = query.size();

    for (int pos : positions) {
        html += text.mid(last, pos - last).toHtmlEscaped();
        html += QStringLiteral("<span class='search-hit'>")
            + text.mid(pos, len).toHtmlEscaped()
            + QStringLiteral("</span>");
        last = pos + len;
    }
    html += text.mid(last).toHtmlEscaped();
    return html;
}
