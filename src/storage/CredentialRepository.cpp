#include "storage/CredentialRepository.h"
#include "security/CryptoManager.h"
#include "utils/TimeUtils.h"

#include <QDateTime>
#include <sqlite3.h>

CredentialRepository::CredentialRepository(Database& db)
    : m_db(db) {}

QByteArray CredentialRepository::encryptField(
    const QString& text,
    qint64 credId,
    const char* field,
    const QByteArray& key) const {
    const QByteArray aad = CryptoManager::credentialFieldAssociatedData(credId, field);
    const auto enc = CryptoManager::encryptField(text.toUtf8(), key, aad);
    return enc.value_or(QByteArray());
}

QString CredentialRepository::decryptField(
    const QByteArray& blob,
    qint64 credId,
    const char* field,
    const QByteArray& key) const {
    const QByteArray aad = CryptoManager::credentialFieldAssociatedData(credId, field);
    const auto dec = CryptoManager::decryptField(blob, key, aad);
    if (!dec) {
        const auto legacy = CryptoManager::decryptLegacy(blob, key);
        if (legacy) {
            return QString::fromUtf8(*legacy);
        }
        return QString();
    }
    return QString::fromUtf8(*dec);
}

namespace {

QString decryptCredField(
    const QByteArray& blob,
    qint64 credId,
    const char* field,
    const QByteArray& key) {
    const QByteArray aad = CryptoManager::credentialFieldAssociatedData(credId, field);
    const auto dec = CryptoManager::decryptField(blob, key, aad);
    if (!dec) {
        const auto legacy = CryptoManager::decryptLegacy(blob, key);
        if (legacy) {
            return QString::fromUtf8(*legacy);
        }
        return QString();
    }
    return QString::fromUtf8(*dec);
}

Credential rowToCredential(sqlite3_stmt* stmt, const QByteArray& key) {
    Credential c;
    c.id = sqlite3_column_int64(stmt, 0);
    const auto* lPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 1));
    const auto* uPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 2));
    const auto* pPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 3));
    const auto* urlPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 4));
    const auto* nPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 5));
    c.label = decryptCredField(QByteArray(lPtr, sqlite3_column_bytes(stmt, 1)), c.id, "label", key);
    c.username = decryptCredField(QByteArray(uPtr, sqlite3_column_bytes(stmt, 2)), c.id, "username", key);
    c.password = decryptCredField(QByteArray(pPtr, sqlite3_column_bytes(stmt, 3)), c.id, "password", key);
    c.url = decryptCredField(QByteArray(urlPtr, sqlite3_column_bytes(stmt, 4)), c.id, "url", key);
    c.notes = decryptCredField(QByteArray(nPtr, sqlite3_column_bytes(stmt, 5)), c.id, "notes", key);
    c.createdAt = TimeUtils::fromUnix(sqlite3_column_int64(stmt, 6));
    c.updatedAt = TimeUtils::fromUnix(sqlite3_column_int64(stmt, 7));
    return c;
}

} // namespace

QVector<Credential> CredentialRepository::listCredentials(const QByteArray& key) const {
    QVector<Credential> creds;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, encrypted_label, encrypted_username, encrypted_password, "
        "encrypted_url, encrypted_notes, created_at, updated_at "
        "FROM credentials ORDER BY updated_at DESC;";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return creds;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        creds.append(rowToCredential(stmt, key));
    }
    sqlite3_finalize(stmt);
    return creds;
}

std::optional<Credential> CredentialRepository::getCredential(
    qint64 id,
    const QByteArray& key) const {
    if (id <= 0) {
        return std::nullopt;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, encrypted_label, encrypted_username, encrypted_password, "
        "encrypted_url, encrypted_notes, created_at, updated_at "
        "FROM credentials WHERE id = ?;";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, id);

    std::optional<Credential> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = rowToCredential(stmt, key);
    }
    sqlite3_finalize(stmt);
    return result;
}

qint64 CredentialRepository::createCredential(const Credential& cred, const QByteArray& key) {
    const qint64 now = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO credentials "
        "(encrypted_label, encrypted_username, encrypted_password, encrypted_url, "
        "encrypted_notes, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    const QByteArray placeholderLabel = encryptField(cred.label, 0, "label", key);
    const QByteArray placeholderUser = encryptField(cred.username, 0, "username", key);
    const QByteArray placeholderPass = encryptField(cred.password, 0, "password", key);
    const QByteArray placeholderUrl = encryptField(cred.url, 0, "url", key);
    const QByteArray placeholderNotes = encryptField(cred.notes, 0, "notes", key);

    if (placeholderLabel.isEmpty() || placeholderUser.isEmpty() || placeholderPass.isEmpty()
        || placeholderUrl.isEmpty() || placeholderNotes.isEmpty()) {
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_bind_blob(stmt, 1, placeholderLabel.constData(), placeholderLabel.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 2, placeholderUser.constData(), placeholderUser.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 3, placeholderPass.constData(), placeholderPass.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 4, placeholderUrl.constData(), placeholderUrl.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 5, placeholderNotes.constData(), placeholderNotes.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, now);
    sqlite3_bind_int64(stmt, 7, now);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);

    const qint64 newId = sqlite3_last_insert_rowid(m_db.handle());
    if (newId <= 0) {
        return 0;
    }

    Credential updated = cred;
    updated.id = newId;
    if (!updateCredential(updated, key)) {
        deleteCredential(newId);
        return 0;
    }
    return newId;
}

bool CredentialRepository::updateCredential(const Credential& cred, const QByteArray& key) {
    if (cred.id <= 0) {
        return false;
    }

    const qint64 now = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());
    const QByteArray encLabel = encryptField(cred.label, cred.id, "label", key);
    const QByteArray encUser = encryptField(cred.username, cred.id, "username", key);
    const QByteArray encPass = encryptField(cred.password, cred.id, "password", key);
    const QByteArray encUrl = encryptField(cred.url, cred.id, "url", key);
    const QByteArray encNotes = encryptField(cred.notes, cred.id, "notes", key);

    if (encLabel.isEmpty() || encUser.isEmpty() || encPass.isEmpty()
        || encUrl.isEmpty() || encNotes.isEmpty()) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "UPDATE credentials SET encrypted_label = ?, encrypted_username = ?, "
        "encrypted_password = ?, encrypted_url = ?, encrypted_notes = ?, updated_at = ? "
        "WHERE id = ?;";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_blob(stmt, 1, encLabel.constData(), encLabel.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 2, encUser.constData(), encUser.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 3, encPass.constData(), encPass.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 4, encUrl.constData(), encUrl.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 5, encNotes.constData(), encNotes.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, now);
    sqlite3_bind_int64(stmt, 7, cred.id);

    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool CredentialRepository::deleteCredential(qint64 id) {
    if (id <= 0) {
        return false;
    }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM credentials WHERE id = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

QVector<Credential> CredentialRepository::searchCredentials(
    const QString& query,
    const QByteArray& key) const {
    const QString q = query.trimmed().toLower();
    if (q.isEmpty()) {
        return listCredentials(key);
    }

    QVector<Credential> matches;
    for (const Credential& c : listCredentials(key)) {
        if (c.label.toLower().contains(q)
            || c.username.toLower().contains(q)
            || c.url.toLower().contains(q)
            || c.notes.toLower().contains(q)) {
            matches.append(c);
        }
    }
    return matches;
}
