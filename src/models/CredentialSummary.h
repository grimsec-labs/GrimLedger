#pragma once

#include <QString>
#include <QDateTime>

struct CredentialSummary {
    qint64 id = 0;
    QString label;
    QString username;
    QString url;
    bool allowSubdomains = false;
    QDateTime createdAt;
    QDateTime updatedAt;
};
