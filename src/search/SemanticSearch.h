#pragma once

#include "models/Note.h"

#include <QHash>
#include <QVector>

struct SemanticMatch {
    Note note;
    double score = 0.0;
};

namespace SemanticSearch {

QVector<QString> expandQueryTerms(const QString& query);
QVector<SemanticMatch> search(
    const QVector<Note>& notes,
    const QHash<qint64, QString>& bodiesById,
    const QString& query);
bool buildIndexEntry(const QString& body, QByteArray& encryptedTokensOut, const QByteArray& key, qint64 noteId);
double scoreBody(const QString& body, const QVector<QString>& terms);

} // namespace SemanticSearch
