#pragma once

#include "models/Credential.h"
#include "storage/Database.h"

#include <QByteArray>
#include <QVector>
#include <optional>

class CredentialRepository {
public:
    explicit CredentialRepository(Database& db);

    QVector<Credential> listCredentials(const QByteArray& key) const;
    std::optional<Credential> getCredential(qint64 id, const QByteArray& key) const;

    qint64 createCredential(const Credential& cred, const QByteArray& key);
    bool updateCredential(const Credential& cred, const QByteArray& key);
    bool deleteCredential(qint64 id);

    QVector<Credential> searchCredentials(const QString& query, const QByteArray& key) const;

private:
    QByteArray encryptField(
        const QString& text,
        qint64 credId,
        const char* field,
        const QByteArray& key) const;
    QString decryptField(
        const QByteArray& blob,
        qint64 credId,
        const char* field,
        const QByteArray& key) const;

    Database& m_db;
};
