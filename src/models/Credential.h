#pragma once

#include "models/FillTrustLevel.h"

#include <QString>
#include <QDateTime>

struct Credential {
    qint64 id = 0;
    QString label;
    QString username;
    QString password;
    QString url;
    QString notes;
    QString totpSecret;
    FillTrustLevel fillTrustLevel = FillTrustLevel::ExactOrigin;
    bool integrityError = false;
    QDateTime createdAt;
    QDateTime updatedAt;

    bool allowSubdomains() const {
        return fillTrustAllowsSubdomains(fillTrustLevel);
    }
};
