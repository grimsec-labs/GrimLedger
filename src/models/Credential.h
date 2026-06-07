#pragma once

#include <QString>
#include <QDateTime>

struct Credential {
    qint64 id = 0;
    QString label;
    QString username;
    QString password;
    QString url;
    QString notes;
    QDateTime createdAt;
    QDateTime updatedAt;
};
