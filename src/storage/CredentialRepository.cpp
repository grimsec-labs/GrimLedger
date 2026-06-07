#include "storage/CredentialRepository.h"
#include "security/CryptoManager.h"
#include "storage/DbTransaction.h"
#include "utils/SqliteUtils.h"
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

DecryptResult<QString> CredentialRepository::decryptField(
    const QByteArray& blob,
    qint64 credId,
    const char* field,
    const QByteArray& key) const {
    DecryptResult<QString> result;
    if (blob.isEmpty()) {
        result.status = DecryptStatus::Empty;
        return result;
    }

    const QByteArray aad = CryptoManager::credentialFieldAssociatedData(credId, field);
    const auto dec = CryptoManager::decryptField(blob, key, aad);
    if (dec) {
        result.status = dec->isEmpty() ? DecryptStatus::Empty : DecryptStatus::Ok;
        result.value = QString::fromUtf8(*dec);
        return result;
    }

    const auto legacy = CryptoManager::decryptLegacy(blob, key);
    if (legacy) {
        result.status = legacy->isEmpty() ? DecryptStatus::Empty : DecryptStatus::Ok;
        result.value = QString::fromUtf8(*legacy);
        return result;
    }

    result.status = DecryptStatus::IntegrityError;
    return result;
}

QByteArray CredentialRepository::encryptFillPolicy(
    bool allowSubdomains,
    qint64 credId,
    const QByteArray& key) const {
    const QByteArray plain(1, allowSubdomains ? '\x01' : '\x00');
    const auto enc = CryptoManager::encryptField(
        plain,
        key,
        CryptoManager::credentialFillPolicyAssociatedData(credId));
    return enc.value_or(QByteArray());
}

bool CredentialRepository::resolveAllowSubdomains(
    sqlite3_stmt* stmt,
    int plainCol,
    int encCol,
    qint64 credId,
    const QByteArray& key,
    bool& integrityError) const {
    const auto* encPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, encCol));
    const QByteArray encBlob(encPtr, sqlite3_column_bytes(stmt, encCol));
    if (!encBlob.isEmpty()) {
        const auto dec = CryptoManager::decryptField(
            encBlob,
            key,
            CryptoManager::credentialFillPolicyAssociatedData(credId));
        if (!dec || dec->isEmpty()) {
            integrityError = true;
            return false;
        }
        return (*dec)[0] != '\x00';
    }

    const int type = sqlite3_column_type(stmt, plainCol);
    if (type == SQLITE_NULL) {
        return false;
    }
    return sqlite3_column_int(stmt, plainCol) != 0;
}

namespace {

const char* kSelectColumns =
    "SELECT id, encrypted_label, encrypted_username, encrypted_password, "
    "encrypted_url, encrypted_notes, created_at, updated_at, allow_subdomains, "
    "encrypted_fill_policy FROM credentials";

const char* kSummarySelectColumns =
    "SELECT id, encrypted_label, encrypted_username, encrypted_url, "
    "created_at, updated_at, allow_subdomains, encrypted_fill_policy FROM credentials";

constexpr int kSummaryColLabel = 1;
constexpr int kSummaryColUsername = 2;
constexpr int kSummaryColUrl = 3;
constexpr int kSummaryColCreatedAt = 4;
constexpr int kSummaryColUpdatedAt = 5;
constexpr int kSummaryColAllowSubdomains = 6;
constexpr int kSummaryColEncryptedFillPolicy = 7;

} // namespace

Credential CredentialRepository::rowToCredential(sqlite3_stmt* stmt, const QByteArray& key) const {
    Credential c;
    c.id = sqlite3_column_int64(stmt, 0);
    c.integrityError = false;
    c.allowSubdomains = resolveAllowSubdomains(
        stmt,
        8,
        9,
        c.id,
        key,
        c.integrityError);
    if (c.integrityError) {
        return c;
    }

    static const char* kFields[] = {"label", "username", "password", "url", "notes"};
    QString* targets[] = {&c.label, &c.username, &c.password, &c.url, &c.notes};

    for (int i = 0; i < 5; ++i) {
        const auto* ptr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 1 + i));
        const QByteArray blob(ptr, sqlite3_column_bytes(stmt, 1 + i));
        const DecryptResult<QString> dec = decryptField(blob, c.id, kFields[i], key);
        if (dec.status == DecryptStatus::IntegrityError) {
            c.integrityError = true;
            return c;
        }
        *targets[i] = dec.value;
    }

    c.createdAt = TimeUtils::fromUnix(sqlite3_column_int64(stmt, 6));
    c.updatedAt = TimeUtils::fromUnix(sqlite3_column_int64(stmt, 7));
    return c;
}

CredentialSummary CredentialRepository::rowToSummary(sqlite3_stmt* stmt, const QByteArray& key) const {
    CredentialSummary summary;
    summary.id = sqlite3_column_int64(stmt, 0);
    bool integrityError = false;
    summary.allowSubdomains = resolveAllowSubdomains(
        stmt,
        kSummaryColAllowSubdomains,
        kSummaryColEncryptedFillPolicy,
        summary.id,
        key,
        integrityError);

    struct FieldMapping {
        int column;
        const char* field;
        QString* target;
    };
    const FieldMapping mappings[] = {
        {kSummaryColLabel, "label", &summary.label},
        {kSummaryColUsername, "username", &summary.username},
        {kSummaryColUrl, "url", &summary.url},
    };
    for (const FieldMapping& mapping : mappings) {
        const auto* ptr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, mapping.column));
        const QByteArray blob(ptr, sqlite3_column_bytes(stmt, mapping.column));
        const DecryptResult<QString> dec = decryptField(blob, summary.id, mapping.field, key);
        if (dec.status == DecryptStatus::IntegrityError || integrityError) {
            summary.label = QStringLiteral("(integrity error)");
            summary.username.clear();
            summary.url.clear();
            return summary;
        }
        *mapping.target = dec.value;
    }

    summary.createdAt = TimeUtils::fromUnix(sqlite3_column_int64(stmt, kSummaryColCreatedAt));
    summary.updatedAt = TimeUtils::fromUnix(sqlite3_column_int64(stmt, kSummaryColUpdatedAt));
    return summary;
}

QVector<CredentialSummary> CredentialRepository::listCredentialSummaries(const QByteArray& key) const {
    QVector<CredentialSummary> creds;
    sqlite3_stmt* stmt = nullptr;
    const QByteArray sql = QByteArray(kSummarySelectColumns) + " ORDER BY updated_at DESC;";

    if (sqlite3_prepare_v2(m_db.handle(), sql.constData(), -1, &stmt, nullptr) != SQLITE_OK) {
        return creds;
    }

    int rc = SQLITE_OK;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        creds.append(rowToSummary(stmt, key));
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        creds.clear();
    }
    return creds;
}

QVector<Credential> CredentialRepository::listCredentials(const QByteArray& key) const {
    QVector<Credential> creds;
    for (const CredentialSummary& summary : listCredentialSummaries(key)) {
        if (auto cred = getCredential(summary.id, key)) {
            creds.append(*cred);
        }
    }
    return creds;
}

std::optional<Credential> CredentialRepository::getCredential(
    qint64 id,
    const QByteArray& key) const {
    if (id <= 0) {
        return std::nullopt;
    }

    sqlite3_stmt* stmt = nullptr;
    const QByteArray sql = QByteArray(kSelectColumns) + " WHERE id = ?;";

    if (sqlite3_prepare_v2(m_db.handle(), sql.constData(), -1, &stmt, nullptr) != SQLITE_OK) {
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

    DbTransaction tx(m_db);
    if (!tx.isActive()) {
        return 0;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO credentials "
        "(encrypted_label, encrypted_username, encrypted_password, encrypted_url, "
        "encrypted_notes, created_at, updated_at, allow_subdomains) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

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
    sqlite3_bind_int(stmt, 8, cred.allowSubdomains ? 1 : 0);

    if (!SqliteUtils::stepDone(stmt)) {
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
        return 0;
    }

    if (!tx.commit()) {
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
    const QByteArray encPolicy = encryptFillPolicy(cred.allowSubdomains, cred.id, key);

    if (encLabel.isEmpty() || encUser.isEmpty() || encPass.isEmpty()
        || encUrl.isEmpty() || encNotes.isEmpty() || encPolicy.isEmpty()) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "UPDATE credentials SET encrypted_label = ?, encrypted_username = ?, "
        "encrypted_password = ?, encrypted_url = ?, encrypted_notes = ?, "
        "updated_at = ?, allow_subdomains = ?, encrypted_fill_policy = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_blob(stmt, 1, encLabel.constData(), encLabel.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 2, encUser.constData(), encUser.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 3, encPass.constData(), encPass.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 4, encUrl.constData(), encUrl.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 5, encNotes.constData(), encNotes.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, now);
    sqlite3_bind_int(stmt, 7, cred.allowSubdomains ? 1 : 0);
    sqlite3_bind_blob(stmt, 8, encPolicy.constData(), encPolicy.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 9, cred.id);

    if (!SqliteUtils::stepDone(stmt)) {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return SqliteUtils::expectChanges(m_db.handle(), 1);
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
    if (!SqliteUtils::stepDone(stmt)) {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return SqliteUtils::expectChanges(m_db.handle(), 1);
}

bool CredentialRepository::migrateFillPolicies(const QByteArray& key) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, allow_subdomains FROM credentials "
        "WHERE encrypted_fill_policy IS NULL OR length(encrypted_fill_policy) = 0;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    QVector<std::pair<qint64, bool>> pending;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const qint64 id = sqlite3_column_int64(stmt, 0);
        const bool allow = sqlite3_column_int(stmt, 1) != 0;
        pending.append({id, allow});
    }
    sqlite3_finalize(stmt);

    DbTransaction tx(m_db);
    if (!tx.isActive()) {
        return false;
    }

    for (const auto& row : pending) {
        const QByteArray encPolicy = encryptFillPolicy(row.second, row.first, key);
        if (encPolicy.isEmpty()) {
            return false;
        }

        sqlite3_stmt* upd = nullptr;
        const char* updSql = "UPDATE credentials SET encrypted_fill_policy = ? WHERE id = ?;";
        if (sqlite3_prepare_v2(m_db.handle(), updSql, -1, &upd, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_blob(upd, 1, encPolicy.constData(), encPolicy.size(), SQLITE_TRANSIENT);
        sqlite3_bind_int64(upd, 2, row.first);
        if (!SqliteUtils::stepDone(upd) || !SqliteUtils::expectChanges(m_db.handle(), 1)) {
            sqlite3_finalize(upd);
            return false;
        }
        sqlite3_finalize(upd);
    }

    return tx.commit();
}

QVector<CredentialSummary> CredentialRepository::searchCredentialSummaries(
    const QString& query,
    const QByteArray& key) const {
    const QString q = query.trimmed().toLower();
    if (q.isEmpty()) {
        return listCredentialSummaries(key);
    }

    QVector<CredentialSummary> matches;
    for (const CredentialSummary& c : listCredentialSummaries(key)) {
        if (c.label.toLower().contains(q)
            || c.username.toLower().contains(q)
            || c.url.toLower().contains(q)) {
            matches.append(c);
        }
    }
    return matches;
}

QVector<Credential> CredentialRepository::searchCredentials(
    const QString& query,
    const QByteArray& key) const {
    QVector<Credential> matches;
    for (const CredentialSummary& summary : searchCredentialSummaries(query, key)) {
        if (auto cred = getCredential(summary.id, key)) {
            matches.append(*cred);
        }
    }
    return matches;
}
