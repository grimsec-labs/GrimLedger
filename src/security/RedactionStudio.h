#pragma once

#include "security/SecretScanner.h"

#include <QString>
#include <QVector>

struct RedactionHit {
    int start = 0;
    int length = 0;
    QString label;
    bool approved = true;
};

namespace RedactionStudio {

QVector<RedactionHit> detectHits(const QString& text);
QString applyRedactions(const QString& text, const QVector<RedactionHit>& hits);

} // namespace RedactionStudio
