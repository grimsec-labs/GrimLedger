#pragma once

#include "models/FillTrustLevel.h"

#include <QString>
#include <QDateTime>

struct CredentialSummary {
    qint64 id = 0;
    QString label;
    QString username;
    QString url;
    FillTrustLevel fillTrustLevel = FillTrustLevel::ExactOrigin;
    QDateTime createdAt;
    QDateTime updatedAt;

    bool allowSubdomains() const {
        return fillTrustAllowsSubdomains(fillTrustLevel);
    }
};
