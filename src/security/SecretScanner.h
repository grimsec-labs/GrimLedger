#pragma once

#include <QString>
#include <QVector>

struct SecretFinding {
    QString reason;
    QString snippet;
    int start = 0;
    int length = 0;
};

namespace SecretScanner {

QVector<SecretFinding> scanText(const QString& text);
QString redactSnippet(const QString& snippet);

} // namespace SecretScanner
