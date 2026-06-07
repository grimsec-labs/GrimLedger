#include "storage/VaultRepository.h"
#include "security/PasswordManager.h"
#include "utils/TimeUtils.h"

#include <QFile>
#include <QDateTime>
#include <sqlite3.h>

namespace {
constexpr int kVaultVersion = 1;
constexpr char kBackupMagic[] = "GRIMBKUP1";
}

VaultRepository::VaultRepository(Database& db)
    : m_db(db) {}

bool VaultRepository::vaultExists() const {
    if (!m_db.isOpen()) return false;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COUNT(*) FROM vault_metadata WHERE id = 1;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    return exists;
}

std::optional<VaultInfo> VaultRepository::loadVaultInfo() const {
    if (!m_db.isOpen()) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT vault_version, salt, kdf_ops_limit, kdf_mem_limit, created_at, updated_at "
        "FROM vault_metadata WHERE id = 1;";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    std::optional<VaultInfo> info;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        VaultInfo v;
        v.version = sqlite3_column_int(stmt, 0);
        const auto* saltPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 1));
        const int saltLen = sqlite3_column_bytes(stmt, 1);
        v.salt = QByteArray(saltPtr, saltLen);
        v.kdfParams.opsLimit = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 2));
        v.kdfParams.memLimit = static_cast<size_t>(sqlite3_column_int64(stmt, 3));
        v.createdAt = sqlite3_column_int64(stmt, 4);
        v.updatedAt = sqlite3_column_int64(stmt, 5);
        info = v;
    }

    sqlite3_finalize(stmt);
    return info;
}

bool VaultRepository::storeVaultInfo(const VaultInfo& info) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT OR REPLACE INTO vault_metadata "
        "(id, vault_version, salt, kdf_ops_limit, kdf_mem_limit, created_at, updated_at) "
        "VALUES (1, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, info.version);
    sqlite3_bind_blob(stmt, 2, info.salt.constData(), info.salt.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(info.kdfParams.opsLimit));
    sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(info.kdfParams.memLimit));
    sqlite3_bind_int64(stmt, 5, info.createdAt);
    sqlite3_bind_int64(stmt, 6, info.updatedAt);

    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool VaultRepository::createVault(const QString& masterPassword) {
    QString err;
    if (!PasswordManager::isValidVaultPassword(masterPassword, &err)) {
        return false;
    }

    VaultInfo info;
    info.version = kVaultVersion;
    info.salt = CryptoManager::randomBytes(CryptoManager::kSaltSize);
    info.kdfParams = CryptoManager::defaultKdfParams();
    info.createdAt = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());
    info.updatedAt = info.createdAt;

    if (!storeVaultInfo(info)) {
        return false;
    }

    auto key = CryptoManager::deriveKey(masterPassword, info.salt, info.kdfParams);
    if (!key) {
        return false;
    }
    CryptoManager::secureZero(*key);
    return true;
}

bool VaultRepository::unlockVault(const QString& masterPassword, QByteArray& derivedKeyOut) {
    const auto info = loadVaultInfo();
    if (!info) {
        return false;
    }

    auto key = CryptoManager::deriveKey(masterPassword, info->salt, info->kdfParams);
    if (!key) {
        return false;
    }

    // Verify key by attempting to decrypt a sentinel stored in app_metadata.
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT value FROM app_metadata WHERE key = 'verify';";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        CryptoManager::secureZero(*key);
        return false;
    }

    bool verified = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* blob = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 0));
        const int len = sqlite3_column_bytes(stmt, 0);
        const QByteArray stored(blob, len);
        const auto plain = CryptoManager::decrypt(stored, *key);
        verified = plain.has_value() && *plain == QByteArray("GRIMLEDGER_OK");
    } else {
        // First unlock after create — write verification token.
        const QByteArray token = QByteArray("GRIMLEDGER_OK");
        const auto enc = CryptoManager::encrypt(token, *key);
        if (!enc) {
            sqlite3_finalize(stmt);
            CryptoManager::secureZero(*key);
            return false;
        }

        sqlite3_stmt* ins = nullptr;
        const char* insSql = "INSERT OR REPLACE INTO app_metadata (key, value) VALUES ('verify', ?);";
        if (sqlite3_prepare_v2(m_db.handle(), insSql, -1, &ins, nullptr) == SQLITE_OK) {
            sqlite3_bind_blob(ins, 1, enc->constData(), enc->size(), SQLITE_TRANSIENT);
            verified = sqlite3_step(ins) == SQLITE_DONE;
            sqlite3_finalize(ins);
        }
    }

    sqlite3_finalize(stmt);

    if (!verified) {
        CryptoManager::secureZero(*key);
        return false;
    }

    derivedKeyOut = std::move(*key);
    return true;
}

bool VaultRepository::changeMasterPassword(
    const QByteArray& currentKey,
    const QString& newPassword) {
    QString err;
    if (!PasswordManager::isValidVaultPassword(newPassword, &err)) {
        return false;
    }

    auto info = loadVaultInfo();
    if (!info) return false;

    const QByteArray newSalt = CryptoManager::randomBytes(CryptoManager::kSaltSize);
    auto newKey = CryptoManager::deriveKey(newPassword, newSalt, info->kdfParams);
    if (!newKey) return false;

    // Re-encrypt verification token with new key.
    const auto enc = CryptoManager::encrypt(QByteArray("GRIMLEDGER_OK"), *newKey);
    if (!enc) {
        CryptoManager::secureZero(*newKey);
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE app_metadata SET value = ? WHERE key = 'verify';";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        CryptoManager::secureZero(*newKey);
        return false;
    }
    sqlite3_bind_blob(stmt, 1, enc->constData(), enc->size(), SQLITE_TRANSIENT);
    const bool metaOk = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!metaOk) {
        CryptoManager::secureZero(*newKey);
        return false;
    }

    // Re-encrypt all notes with new key — done via NoteRepository callback pattern.
    // For simplicity, re-encrypt notes inline here.
    const char* notesSql = "SELECT id, encrypted_title, encrypted_body FROM notes;";
    sqlite3_stmt* notesStmt = nullptr;
    if (sqlite3_prepare_v2(m_db.handle(), notesSql, -1, &notesStmt, nullptr) != SQLITE_OK) {
        CryptoManager::secureZero(*newKey);
        return false;
    }

    struct NoteRow { qint64 id; QByteArray title; QByteArray body; };
    QVector<NoteRow> rows;

    while (sqlite3_step(notesStmt) == SQLITE_ROW) {
        NoteRow row;
        row.id = sqlite3_column_int64(notesStmt, 0);
        const auto* tPtr = reinterpret_cast<const char*>(sqlite3_column_blob(notesStmt, 1));
        row.title = QByteArray(tPtr, sqlite3_column_bytes(notesStmt, 1));
        const auto* bPtr = reinterpret_cast<const char*>(sqlite3_column_blob(notesStmt, 2));
        row.body = QByteArray(bPtr, sqlite3_column_bytes(notesStmt, 2));
        rows.append(row);
    }
    sqlite3_finalize(notesStmt);

    for (const auto& row : rows) {
        const auto plainTitleOpt = CryptoManager::decrypt(row.title, currentKey);
        const auto plainBodyOpt = CryptoManager::decrypt(row.body, currentKey);
        if (!plainTitleOpt || !plainBodyOpt) {
            CryptoManager::secureZero(*newKey);
            return false;
        }

        QByteArray plainTitle = *plainTitleOpt;
        QByteArray plainBody = *plainBodyOpt;
        const auto newTitle = CryptoManager::encrypt(plainTitle, *newKey);
        const auto newBody = CryptoManager::encrypt(plainBody, *newKey);
        CryptoManager::secureZero(plainTitle);
        CryptoManager::secureZero(plainBody);

        if (!newTitle || !newBody) {
            CryptoManager::secureZero(*newKey);
            return false;
        }

        sqlite3_stmt* upd = nullptr;
        const char* updSql =
            "UPDATE notes SET encrypted_title = ?, encrypted_body = ? WHERE id = ?;";
        if (sqlite3_prepare_v2(m_db.handle(), updSql, -1, &upd, nullptr) != SQLITE_OK) {
            CryptoManager::secureZero(*newKey);
            return false;
        }
        sqlite3_bind_blob(upd, 1, newTitle->constData(), newTitle->size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 2, newBody->constData(), newBody->size(), SQLITE_TRANSIENT);
        sqlite3_bind_int64(upd, 3, row.id);
        const bool ok = sqlite3_step(upd) == SQLITE_DONE;
        sqlite3_finalize(upd);
        if (!ok) {
            CryptoManager::secureZero(*newKey);
            return false;
        }
    }

    info->salt = newSalt;
    info->updatedAt = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());
    if (!storeVaultInfo(*info)) {
        CryptoManager::secureZero(*newKey);
        return false;
    }

    // Caller must update session key.
    CryptoManager::secureZero(*newKey);
    return true;
}

bool VaultRepository::exportEncryptedBackup(const QByteArray& key, const QString& destPath) {
    QFile src(m_db.path());
    if (!src.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray dbData = src.readAll();
    src.close();

    const auto encrypted = CryptoManager::encrypt(dbData, key);
    if (!encrypted) return false;

    QFile dest(destPath);
    if (!dest.open(QIODevice::WriteOnly)) return false;
    dest.write(kBackupMagic, sizeof(kBackupMagic) - 1);
    dest.write(*encrypted);
    dest.close();
    return true;
}

bool VaultRepository::importEncryptedBackup(const QString& srcPath, const QString& newMasterPassword) {
    QFile src(srcPath);
    if (!src.open(QIODevice::ReadOnly)) return false;

    const QByteArray magic = src.read(sizeof(kBackupMagic) - 1);
    if (magic != QByteArray(kBackupMagic, sizeof(kBackupMagic) - 1)) {
        return false;
    }

    const QByteArray encData = src.readAll();
    src.close();

    // Import requires unlocking with backup's original key — simplified: backup is encrypted
    // with current session key. For restore, user must unlock first then import overwrites.
    Q_UNUSED(newMasterPassword);
    return false; // Handled in Settings via session-key decrypt path
}
