#pragma once

#include "models/Credential.h"
#include "models/CredentialSummary.h"
#include "storage/Database.h"
#include "utils/RepositoryResult.h"

#include <QByteArray>
#include <QVector>
#include <optional>

struct sqlite3_stmt;

class CredentialRepository {
public:
    explicit CredentialRepository(Database& db);

    QVector<CredentialSummary> listCredentialSummaries(const QByteArray& key) const;
    QVector<Credential> listCredentials(const QByteArray& key) const;
    std::optional<Credential> getCredential(qint64 id, const QByteArray& key) const;

    qint64 createCredential(const Credential& cred, const QByteArray& key);
    bool updateCredential(const Credential& cred, const QByteArray& key);
    bool deleteCredential(qint64 id);

    QVector<CredentialSummary> searchCredentialSummaries(
        const QString& query,
        const QByteArray& key) const;
    QVector<Credential> searchCredentials(const QString& query, const QByteArray& key) const;

    bool migrateFillPolicies(const QByteArray& key);
    int countCredentialIntegrityErrors(const QByteArray& key) const;
    bool importCredentials(const QVector<Credential>& creds, const QByteArray& key, int* importedOut = nullptr);

private:
    QByteArray encryptField(
        const QString& text,
        qint64 credId,
        const char* field,
        const QByteArray& key) const;
    DecryptResult<QString> decryptField(
        const QByteArray& blob,
        qint64 credId,
        const char* field,
        const QByteArray& key) const;
    Credential rowToCredential(sqlite3_stmt* stmt, const QByteArray& key) const;
    CredentialSummary rowToSummary(sqlite3_stmt* stmt, const QByteArray& key) const;
    QByteArray encryptFillPolicy(FillTrustLevel level, qint64 credId, const QByteArray& key) const;
    bool resolveFillTrustLevel(
        sqlite3_stmt* stmt,
        int plainCol,
        int encCol,
        qint64 credId,
        const QByteArray& key,
        FillTrustLevel& levelOut,
        bool& integrityError) const;

    Database& m_db;
};
