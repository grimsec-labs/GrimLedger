#pragma once

#include "models/Credential.h"

#include <QString>
#include <QVector>

struct ImportedCredential {
    Credential credential;
    QString sourceLabel;
};

struct ImportParseResult {
    bool ok = false;
    QString error;
    QVector<ImportedCredential> items;
};

namespace CredentialImport {

ImportParseResult parseBitwardenCsv(const QString& filePath);
ImportParseResult parseKeePassXml(const QString& filePath);

} // namespace CredentialImport
