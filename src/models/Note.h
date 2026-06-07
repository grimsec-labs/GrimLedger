#pragma once

#include <QString>
#include <QDateTime>
#include <QVector>

struct Note {
    qint64 id = 0;
    QString title;
    QString body;
    qint64 folderId = 0;
    QString folderName;
    QDateTime createdAt;
    QDateTime updatedAt;
    bool isFavorite = false;
    QVector<QString> tags;
};
