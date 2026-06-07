#include "import/CredentialImport.h"
#include "utils/TotpGenerator.h"

#include <QFile>
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace CredentialImport {

namespace {

QVector<QString> parseCsvRow(const QString& line) {
    QVector<QString> fields;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line[i];
        if (inQuotes) {
            if (ch == QLatin1Char('"')) {
                if (i + 1 < line.size() && line[i + 1] == QLatin1Char('"')) {
                    field.append(QLatin1Char('"'));
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field.append(ch);
            }
        } else if (ch == QLatin1Char('"')) {
            inQuotes = true;
        } else if (ch == QLatin1Char(',')) {
            fields.append(field);
            field.clear();
        } else {
            field.append(ch);
        }
    }
    fields.append(field);
    return fields;
}

int columnIndex(const QVector<QString>& headers, const QString& name) {
    for (int i = 0; i < headers.size(); ++i) {
        if (headers[i].compare(name, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }
    return -1;
}

QString fieldAt(const QVector<QString>& row, int index) {
    if (index < 0 || index >= row.size()) {
        return QString();
    }
    return row[index].trimmed();
}

} // namespace

ImportParseResult parseBitwardenCsv(const QString& filePath) {
    ImportParseResult result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = QStringLiteral("Could not open CSV file.");
        return result;
    }

    const QString content = QString::fromUtf8(file.readAll());
    const QStringList lines = content.split(QRegularExpression(QStringLiteral("[\r\n]+")),
        Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        result.error = QStringLiteral("CSV file is empty.");
        return result;
    }

    const QVector<QString> headers = parseCsvRow(lines[0]);
    if (headers.isEmpty()) {
        result.error = QStringLiteral("CSV file is empty.");
        return result;
    }

    const int nameCol = columnIndex(headers, QStringLiteral("name"));
    const int userCol = columnIndex(headers, QStringLiteral("login_username"));
    const int passCol = columnIndex(headers, QStringLiteral("login_password"));
    const int uriCol = columnIndex(headers, QStringLiteral("login_uri"));
    const int notesCol = columnIndex(headers, QStringLiteral("notes"));
    const int totpCol = columnIndex(headers, QStringLiteral("login_totp"));
    const int typeCol = columnIndex(headers, QStringLiteral("type"));

    if (nameCol < 0 || userCol < 0 || passCol < 0) {
        result.error = QStringLiteral("Not a recognized Bitwarden CSV export.");
        return result;
    }

    for (int lineIndex = 1; lineIndex < lines.size(); ++lineIndex) {
        const QVector<QString> row = parseCsvRow(lines[lineIndex]);
        if (row.isEmpty() || row.size() == 1 && row[0].isEmpty()) {
            continue;
        }

        const QString type = fieldAt(row, typeCol);
        if (!type.isEmpty() && type.compare(QStringLiteral("login"), Qt::CaseInsensitive) != 0) {
            continue;
        }

        ImportedCredential item;
        item.sourceLabel = fieldAt(row, nameCol);
        item.credential.label = item.sourceLabel.isEmpty()
            ? QStringLiteral("Imported Login")
            : item.sourceLabel;
        item.credential.username = fieldAt(row, userCol);
        item.credential.password = fieldAt(row, passCol);
        item.credential.url = fieldAt(row, uriCol);
        item.credential.notes = fieldAt(row, notesCol);
        item.credential.totpSecret = TotpGenerator::normalizeSecret(fieldAt(row, totpCol));
        item.credential.fillTrustLevel = FillTrustLevel::ExactOrigin;

        if (item.credential.label.isEmpty() && item.credential.username.isEmpty()
            && item.credential.password.isEmpty()) {
            continue;
        }
        result.items.append(item);
    }

    if (result.items.isEmpty()) {
        result.error = QStringLiteral("No login records found in CSV.");
        return result;
    }

    result.ok = true;
    return result;
}

ImportParseResult parseKeePassXml(const QString& filePath) {
    ImportParseResult result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("Could not open KeePass XML file.");
        return result;
    }

    QXmlStreamReader xml(&file);
    ImportedCredential current;
    bool inEntry = false;
    QString currentKey;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == QStringLiteral("Entry")) {
                inEntry = true;
                current = ImportedCredential {};
                current.credential.fillTrustLevel = FillTrustLevel::ExactOrigin;
                current.sourceLabel.clear();
            } else if (inEntry && xml.name() == QStringLiteral("String")) {
                currentKey = xml.attributes().value(QStringLiteral("Key")).toString();
            } else if (inEntry && xml.name() == QStringLiteral("Value")) {
                const QString value = xml.readElementText();
                if (currentKey == QStringLiteral("Title")) {
                    current.credential.label = value.trimmed();
                    current.sourceLabel = current.credential.label;
                } else if (currentKey == QStringLiteral("UserName")) {
                    current.credential.username = value;
                } else if (currentKey == QStringLiteral("Password")) {
                    current.credential.password = value;
                } else if (currentKey == QStringLiteral("URL")) {
                    current.credential.url = value.trimmed();
                } else if (currentKey == QStringLiteral("Notes")) {
                    current.credential.notes = value;
                } else if (currentKey == QStringLiteral("TOTP")) {
                    current.credential.totpSecret = TotpGenerator::normalizeSecret(value);
                }
            }
        } else if (xml.isEndElement() && xml.name() == QStringLiteral("Entry") && inEntry) {
            inEntry = false;
            if (current.credential.label.isEmpty()) {
                current.credential.label = QStringLiteral("Imported Login");
            }
            if (!current.credential.username.isEmpty()
                || !current.credential.password.isEmpty()
                || !current.credential.url.isEmpty()) {
                result.items.append(current);
            }
        }
    }

    if (xml.hasError()) {
        result.error = QStringLiteral("KeePass XML parse error: %1").arg(xml.errorString());
        return result;
    }
    if (result.items.isEmpty()) {
        result.error = QStringLiteral("No entries found in KeePass XML.");
        return result;
    }

    result.ok = true;
    return result;
}

} // namespace CredentialImport
