#pragma once

#include "models/Note.h"

#include <QVector>
#include <QString>
#include <QHash>

struct SearchMatch {
    Note note;
    QVector<int> titleMatchPositions;
    QVector<int> bodyMatchPositions;
    int score = 0;
};

class SearchEngine {
public:
    static QVector<SearchMatch> search(
        const QVector<Note>& notes,
        const QString& query,
        bool searchBodies,
        const QHash<qint64, QString>& bodiesById = {});

    static QString highlightHtml(const QString& text, const QVector<int>& positions, const QString& query);
};
