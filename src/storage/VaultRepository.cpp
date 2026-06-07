#include "storage/VaultRepository.h"
#include "security/PasswordManager.h"
#include "storage/DbTransaction.h"
#include "utils/SecurityLimits.h"
#include "utils/SqliteUtils.h"
#include "utils/TimeUtils.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QThread>
#include <QtEndian>
#include <cstring>
#include <sqlite3.h>

namespace {

constexpr int kVaultVersion = 1;
constexpr char kBackupMagicV1[] = "GRIMBKUP1";
constexpr char kBackupMagicV2[] = "GRIMBKUP2";
constexpr char kCryptoFormatKey[] = "crypto_field_format";
constexpr char kCryptoFormatV2Value[] = "2";

bool readExact(QIODevice& device, char* dest, qint64 size) {
    return device.read(dest, size) == size;
}

std::optional<QByteArray> readBounded(QIODevice& device, qint64 maxBytes) {
    if (device.size() > maxBytes) {
        return std::nullopt;
    }
    return device.readAll();
}

bool tableExists(sqlite3* db, const char* name) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ? LIMIT 1;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

bool validateVaultMetadataRow(sqlite3* db, QString* errorOut) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT vault_version, salt, kdf_ops_limit, kdf_mem_limit "
        "FROM vault_metadata WHERE id = 1;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        if (errorOut) {
            *errorOut = QStringLiteral("Missing vault metadata.");
        }
        return false;
    }

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        VaultInfo info;
        info.version = sqlite3_column_int(stmt, 0);
        const auto* saltPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 1));
        info.salt = QByteArray(saltPtr, sqlite3_column_bytes(stmt, 1));
        info.kdfParams.opsLimit = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 2));
        info.kdfParams.memLimit = static_cast<size_t>(sqlite3_column_int64(stmt, 3));
        ok = info.version == kVaultVersion
            && CryptoManager::isValidSalt(info.salt)
            && CryptoManager::isValidKdfParams(info.kdfParams);
        if (!ok && errorOut) {
            *errorOut = QStringLiteral("Invalid vault metadata.");
        }
    } else if (errorOut) {
        *errorOut = QStringLiteral("Vault metadata row not found.");
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool quickCheckOk(sqlite3* db, QString* errorOut) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "PRAGMA quick_check;", -1, &stmt, nullptr) != SQLITE_OK) {
        if (errorOut) {
            *errorOut = QStringLiteral("Integrity check failed to run.");
        }
        return false;
    }

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        ok = result && qstrcmp(result, "ok") == 0;
        if (!ok && errorOut) {
            *errorOut = QStringLiteral("Database integrity check failed.");
        }
    } else if (errorOut) {
        *errorOut = QStringLiteral("Database integrity check returned no result.");
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool verifyWithKey(sqlite3* db, const QByteArray& key, QString* errorOut) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT value FROM app_metadata WHERE key = 'verify';";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        if (errorOut) {
            *errorOut = QStringLiteral("Verification token missing.");
        }
        return false;
    }

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* blob = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 0));
        const QByteArray stored(blob, sqlite3_column_bytes(stmt, 0));
        const auto plain = CryptoManager::decryptField(
            stored, key, CryptoManager::verifyAssociatedData());
        if (!plain) {
            const auto legacy = CryptoManager::decryptLegacy(stored, key);
            ok = legacy.has_value() && *legacy == QByteArray("GRIMLEDGER_OK");
        } else {
            ok = *plain == QByteArray("GRIMLEDGER_OK");
        }
        if (!ok && errorOut) {
            *errorOut = QStringLiteral("Backup password is incorrect for this vault.");
        }
    } else if (errorOut) {
        *errorOut = QStringLiteral("Verification token missing.");
    }

    sqlite3_finalize(stmt);
    return ok;
}

QByteArray buildBackupHeaderV2(
    const QByteArray& salt,
    const CryptoManager::KdfParams& params,
    quint64 plaintextLen) {
    QByteArray header;
    header.reserve(43);
    header.append(static_cast<char>(1));
    const quint16 vaultVersion = qToLittleEndian<quint16>(kVaultVersion);
    header.append(reinterpret_cast<const char*>(&vaultVersion), sizeof(vaultVersion));
    header.append(salt);
    const quint64 ops = qToLittleEndian<quint64>(params.opsLimit);
    const quint64 mem = qToLittleEndian<quint64>(static_cast<quint64>(params.memLimit));
    const quint64 plain = qToLittleEndian<quint64>(plaintextLen);
    header.append(reinterpret_cast<const char*>(&ops), sizeof(ops));
    header.append(reinterpret_cast<const char*>(&mem), sizeof(mem));
    header.append(reinterpret_cast<const char*>(&plain), sizeof(plain));
    return header;
}

bool parseBackupHeaderV2(const QByteArray& header, QByteArray& saltOut, CryptoManager::KdfParams& paramsOut, quint64& plainLenOut) {
    if (header.size() != 43 || static_cast<unsigned char>(header[0]) != 1) {
        return false;
    }
    quint16 vaultVersionRaw = 0;
    quint64 opsRaw = 0;
    quint64 memRaw = 0;
    quint64 plainRaw = 0;
    std::memcpy(&vaultVersionRaw, header.constData() + 1, sizeof(vaultVersionRaw));
    const quint16 vaultVersion = qFromLittleEndian(vaultVersionRaw);
    if (vaultVersion != kVaultVersion) {
        return false;
    }
    saltOut = header.mid(3, CryptoManager::kSaltSize);
    std::memcpy(&opsRaw, header.constData() + 19, sizeof(opsRaw));
    std::memcpy(&memRaw, header.constData() + 27, sizeof(memRaw));
    std::memcpy(&plainRaw, header.constData() + 35, sizeof(plainRaw));
    paramsOut.opsLimit = qFromLittleEndian(opsRaw);
    paramsOut.memLimit = static_cast<size_t>(qFromLittleEndian(memRaw));
    plainLenOut = qFromLittleEndian(plainRaw);
    return CryptoManager::isValidSalt(saltOut) && CryptoManager::isValidKdfParams(paramsOut);
}

template<typename RowFn>
bool queryAllRows(sqlite3* db, const char* sql, RowFn&& onRow) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    int rc = SQLITE_OK;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (!onRow(stmt)) {
            sqlite3_finalize(stmt);
            return false;
        }
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

} // namespace

VaultRepository::VaultRepository(Database& db)
    : m_db(db) {}

bool VaultRepository::vaultExists() const {
    if (!m_db.isOpen()) {
        return false;
    }

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
    if (!m_db.isOpen()) {
        return std::nullopt;
    }

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
        v.salt = QByteArray(saltPtr, sqlite3_column_bytes(stmt, 1));
        v.kdfParams.opsLimit = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 2));
        v.kdfParams.memLimit = static_cast<size_t>(sqlite3_column_int64(stmt, 3));
        v.createdAt = sqlite3_column_int64(stmt, 4);
        v.updatedAt = sqlite3_column_int64(stmt, 5);
        if (v.version == kVaultVersion
            && CryptoManager::isValidSalt(v.salt)
            && CryptoManager::isValidKdfParams(v.kdfParams)) {
            info = v;
        }
    }

    sqlite3_finalize(stmt);
    return info;
}

bool VaultRepository::storeVaultInfo(const VaultInfo& info, bool requireChanges) {
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

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    if (requireChanges) {
        return SqliteUtils::expectChanges(m_db.handle(), 1);
    }
    return true;
}

bool VaultRepository::verificationTokenExists() const {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT 1 FROM app_metadata WHERE key = 'verify' LIMIT 1;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

bool VaultRepository::writeVerificationToken(const QByteArray& key) {
    const auto enc = CryptoManager::encryptField(
        QByteArray("GRIMLEDGER_OK"), key, CryptoManager::verifyAssociatedData());
    if (!enc) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO app_metadata (key, value) VALUES ('verify', ?);";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_blob(stmt, 1, enc->constData(), enc->size(), SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool VaultRepository::cryptoFormatIsV2() const {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT value FROM app_metadata WHERE key = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, kCryptoFormatKey, -1, SQLITE_STATIC);
    bool v2 = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        v2 = value && qstrcmp(value, kCryptoFormatV2Value) == 0;
    }
    sqlite3_finalize(stmt);
    return v2;
}

bool VaultRepository::setCryptoFormatV2() {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO app_metadata (key, value) VALUES (?, ?);";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, kCryptoFormatKey, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, kCryptoFormatV2Value, -1, SQLITE_STATIC);
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

    auto key = CryptoManager::deriveKey(masterPassword, info.salt, info.kdfParams);
    if (!key) {
        return false;
    }

    DbTransaction tx(m_db);
    if (!tx.isActive()) {
        CryptoManager::secureZero(*key);
        return false;
    }
    if (!storeVaultInfo(info) || !writeVerificationToken(*key)) {
        CryptoManager::secureZero(*key);
        return false;
    }
    if (!setCryptoFormatV2()) {
        CryptoManager::secureZero(*key);
        return false;
    }

    const bool ok = tx.commit();
    CryptoManager::secureZero(*key);
    return ok;
}

bool VaultRepository::unlockVault(const QString& masterPassword, QByteArray& derivedKeyOut) {
    const auto info = loadVaultInfo();
    if (!info) {
        return false;
    }

    if (vaultExists() && !verificationTokenExists()) {
        return false;
    }

    auto key = CryptoManager::deriveKey(masterPassword, info->salt, info->kdfParams);
    if (!key) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT value FROM app_metadata WHERE key = 'verify';";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        CryptoManager::secureZero(*key);
        return false;
    }

    bool verified = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* blob = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 0));
        const QByteArray stored(blob, sqlite3_column_bytes(stmt, 0));
        const auto plain = CryptoManager::decryptField(
            stored, *key, CryptoManager::verifyAssociatedData());
        if (!plain) {
            const auto legacy = CryptoManager::decryptLegacy(stored, *key);
            verified = legacy.has_value() && *legacy == QByteArray("GRIMLEDGER_OK");
        } else {
            verified = *plain == QByteArray("GRIMLEDGER_OK");
        }
    }
    sqlite3_finalize(stmt);

    if (!verified) {
        CryptoManager::secureZero(*key);
        return false;
    }

    derivedKeyOut = std::move(*key);
    if (!cryptoFormatIsV2() && !migrateDomainBoundCrypto(derivedKeyOut)) {
        CryptoManager::secureZero(derivedKeyOut);
        derivedKeyOut.clear();
        return false;
    }
    return true;
}

bool VaultRepository::migrateDomainBoundCrypto(const QByteArray& key) {
    if (cryptoFormatIsV2()) {
        return true;
    }

    DbTransaction tx(m_db);
    if (!tx.isActive()) {
        return false;
    }

    struct NoteRow { qint64 id; QByteArray title; QByteArray body; };
    struct AttachRow { QString id; qint64 noteId; QByteArray data; };
    struct CredRow {
        qint64 id = 0;
        QByteArray label;
        QByteArray username;
        QByteArray password;
        QByteArray url;
        QByteArray notes;
        bool allowSubdomains = false;
    };

    QVector<NoteRow> notes;
    QVector<AttachRow> attachments;
    QVector<CredRow> credentials;

    if (!queryAllRows(
            m_db.handle(),
            "SELECT id, encrypted_title, encrypted_body FROM notes;",
            [&](sqlite3_stmt* notesStmt) {
                NoteRow row;
                row.id = sqlite3_column_int64(notesStmt, 0);
                const auto* tPtr = reinterpret_cast<const char*>(sqlite3_column_blob(notesStmt, 1));
                row.title = QByteArray(tPtr, sqlite3_column_bytes(notesStmt, 1));
                const auto* bPtr = reinterpret_cast<const char*>(sqlite3_column_blob(notesStmt, 2));
                row.body = QByteArray(bPtr, sqlite3_column_bytes(notesStmt, 2));
                notes.append(row);
                return true;
            })
        || !queryAllRows(
            m_db.handle(),
            "SELECT id, note_id, encrypted_data FROM note_attachments;",
            [&](sqlite3_stmt* attachStmt) {
                AttachRow row;
                row.id = QString::fromUtf8(
                    reinterpret_cast<const char*>(sqlite3_column_text(attachStmt, 0)));
                row.noteId = sqlite3_column_int64(attachStmt, 1);
                const auto* blob = reinterpret_cast<const char*>(sqlite3_column_blob(attachStmt, 2));
                row.data = QByteArray(blob, sqlite3_column_bytes(attachStmt, 2));
                attachments.append(row);
                return true;
            })
        || !queryAllRows(
            m_db.handle(),
            "SELECT id, encrypted_label, encrypted_username, encrypted_password, "
            "encrypted_url, encrypted_notes, allow_subdomains FROM credentials;",
            [&](sqlite3_stmt* credStmt) {
                CredRow row;
                row.id = sqlite3_column_int64(credStmt, 0);
                const auto readBlob = [&](int col) {
                    const auto* ptr = reinterpret_cast<const char*>(sqlite3_column_blob(credStmt, col));
                    return QByteArray(ptr, sqlite3_column_bytes(credStmt, col));
                };
                row.label = readBlob(1);
                row.username = readBlob(2);
                row.password = readBlob(3);
                row.url = readBlob(4);
                row.notes = readBlob(5);
                row.allowSubdomains = sqlite3_column_int(credStmt, 6) != 0;
                credentials.append(row);
                return true;
            })) {
        return false;
    }

    struct PreparedNote {
        qint64 id = 0;
        QByteArray title;
        QByteArray body;
    };
    struct PreparedAttach {
        QString id;
        QByteArray data;
    };
    struct PreparedCred {
        qint64 id = 0;
        QByteArray label;
        QByteArray username;
        QByteArray password;
        QByteArray url;
        QByteArray notes;
        QByteArray fillPolicy;
    };

    QVector<PreparedNote> preparedNotes;
    for (const NoteRow& row : notes) {
        auto plainTitle = CryptoManager::decryptField(
            row.title, key, CryptoManager::noteTitleAssociatedData(row.id));
        if (!plainTitle) {
            plainTitle = CryptoManager::decryptLegacy(row.title, key);
        }
        auto plainBody = CryptoManager::decryptField(
            row.body, key, CryptoManager::noteBodyAssociatedData(row.id));
        if (!plainBody) {
            plainBody = CryptoManager::decryptLegacy(row.body, key);
        }
        if (!plainTitle || !plainBody) {
            return false;
        }

        PreparedNote out;
        out.id = row.id;
        const auto encTitle = CryptoManager::encryptField(
            *plainTitle, key, CryptoManager::noteTitleAssociatedData(row.id));
        const auto encBody = CryptoManager::encryptField(
            *plainBody, key, CryptoManager::noteBodyAssociatedData(row.id));
        CryptoManager::secureZero(*plainTitle);
        CryptoManager::secureZero(*plainBody);
        if (!encTitle || !encBody) {
            return false;
        }
        out.title = *encTitle;
        out.body = *encBody;
        preparedNotes.append(out);
    }

    QVector<PreparedAttach> preparedAttachments;
    for (const AttachRow& row : attachments) {
        auto plain = CryptoManager::decryptField(
            row.data, key, CryptoManager::attachmentAssociatedData(row.id));
        if (!plain) {
            plain = CryptoManager::decryptLegacy(row.data, key);
        }
        if (!plain) {
            return false;
        }
        const auto enc = CryptoManager::encryptField(
            *plain, key, CryptoManager::attachmentAssociatedData(row.id));
        CryptoManager::secureZero(*plain);
        if (!enc) {
            return false;
        }
        PreparedAttach out;
        out.id = row.id;
        out.data = *enc;
        preparedAttachments.append(out);
    }

    QVector<PreparedCred> preparedCredentials;
    for (const CredRow& row : credentials) {
        static const char* kFields[] = {"label", "username", "password", "url", "notes"};
        const QByteArray* blobs[] = {&row.label, &row.username, &row.password, &row.url, &row.notes};
        PreparedCred out;
        out.id = row.id;
        QByteArray* outs[] = {&out.label, &out.username, &out.password, &out.url, &out.notes};
        for (int i = 0; i < 5; ++i) {
            auto plain = CryptoManager::decryptField(
                *blobs[i], key, CryptoManager::credentialFieldAssociatedData(row.id, kFields[i]));
            if (!plain) {
                plain = CryptoManager::decryptLegacy(*blobs[i], key);
            }
            if (!plain) {
                return false;
            }
            QByteArray p = *plain;
            const auto enc = CryptoManager::encryptField(
                p, key, CryptoManager::credentialFieldAssociatedData(row.id, kFields[i]));
            CryptoManager::secureZero(p);
            if (!enc) {
                return false;
            }
            *outs[i] = *enc;
        }
        const QByteArray policyPlain(1, row.allowSubdomains ? '\x01' : '\x00');
        const auto encPolicy = CryptoManager::encryptField(
            policyPlain,
            key,
            CryptoManager::credentialFillPolicyAssociatedData(row.id));
        if (!encPolicy) {
            return false;
        }
        out.fillPolicy = *encPolicy;
        preparedCredentials.append(out);
    }

    for (const PreparedNote& row : preparedNotes) {
        sqlite3_stmt* upd = nullptr;
        const char* sql =
            "UPDATE notes SET encrypted_title = ?, encrypted_body = ? WHERE id = ?;";
        if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &upd, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_blob(upd, 1, row.title.constData(), row.title.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 2, row.body.constData(), row.body.size(), SQLITE_TRANSIENT);
        sqlite3_bind_int64(upd, 3, row.id);
        if (!SqliteUtils::stepDone(upd) || !SqliteUtils::expectChanges(m_db.handle(), 1)) {
            sqlite3_finalize(upd);
            return false;
        }
        sqlite3_finalize(upd);
    }

    for (const PreparedAttach& row : preparedAttachments) {
        sqlite3_stmt* upd = nullptr;
        const char* sql = "UPDATE note_attachments SET encrypted_data = ? WHERE id = ?;";
        if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &upd, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_blob(upd, 1, row.data.constData(), row.data.size(), SQLITE_TRANSIENT);
        sqlite3_bind_text(upd, 2, row.id.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        if (!SqliteUtils::stepDone(upd) || !SqliteUtils::expectChanges(m_db.handle(), 1)) {
            sqlite3_finalize(upd);
            return false;
        }
        sqlite3_finalize(upd);
    }

    for (const PreparedCred& row : preparedCredentials) {
        sqlite3_stmt* upd = nullptr;
        const char* sql =
            "UPDATE credentials SET encrypted_label = ?, encrypted_username = ?, "
            "encrypted_password = ?, encrypted_url = ?, encrypted_notes = ?, "
            "encrypted_fill_policy = ? WHERE id = ?;";
        if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &upd, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_blob(upd, 1, row.label.constData(), row.label.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 2, row.username.constData(), row.username.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 3, row.password.constData(), row.password.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 4, row.url.constData(), row.url.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 5, row.notes.constData(), row.notes.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 6, row.fillPolicy.constData(), row.fillPolicy.size(), SQLITE_TRANSIENT);
        sqlite3_bind_int64(upd, 7, row.id);
        if (!SqliteUtils::stepDone(upd) || !SqliteUtils::expectChanges(m_db.handle(), 1)) {
            sqlite3_finalize(upd);
            return false;
        }
        sqlite3_finalize(upd);
    }

    if (!writeVerificationToken(key)) {
        return false;
    }
    if (!setCryptoFormatV2()) {
        return false;
    }
    return tx.commit();
}

bool VaultRepository::changeMasterPassword(
    const QByteArray& currentKey,
    const QString& newPassword,
    QByteArray& newKeyOut) {
    QString err;
    if (!PasswordManager::isValidVaultPassword(newPassword, &err)) {
        return false;
    }

    auto info = loadVaultInfo();
    if (!info) {
        return false;
    }

    const QByteArray newSalt = CryptoManager::randomBytes(CryptoManager::kSaltSize);
    auto newKey = CryptoManager::deriveKey(newPassword, newSalt, info->kdfParams);
    if (!newKey) {
        return false;
    }

    DbTransaction tx(m_db);
    if (!tx.isActive()) {
        CryptoManager::secureZero(*newKey);
        return false;
    }

    struct NoteRow { qint64 id; QByteArray title; QByteArray body; };
    struct AttachRow { QString id; QByteArray data; };
    struct CredRow {
        qint64 id = 0;
        QByteArray label;
        QByteArray username;
        QByteArray password;
        QByteArray url;
        QByteArray notes;
        bool allowSubdomains = false;
        QByteArray fillPolicy;
    };
    QVector<NoteRow> noteRows;
    QVector<AttachRow> attachRows;
    QVector<CredRow> credRows;

    if (!queryAllRows(
            m_db.handle(),
            "SELECT id, encrypted_title, encrypted_body FROM notes;",
            [&](sqlite3_stmt* notesStmt) {
                NoteRow row;
                row.id = sqlite3_column_int64(notesStmt, 0);
                const auto* tPtr = reinterpret_cast<const char*>(sqlite3_column_blob(notesStmt, 1));
                row.title = QByteArray(tPtr, sqlite3_column_bytes(notesStmt, 1));
                const auto* bPtr = reinterpret_cast<const char*>(sqlite3_column_blob(notesStmt, 2));
                row.body = QByteArray(bPtr, sqlite3_column_bytes(notesStmt, 2));
                noteRows.append(row);
                return true;
            })
        || !queryAllRows(
            m_db.handle(),
            "SELECT id, encrypted_data FROM note_attachments;",
            [&](sqlite3_stmt* attachStmt) {
                AttachRow row;
                row.id = QString::fromUtf8(
                    reinterpret_cast<const char*>(sqlite3_column_text(attachStmt, 0)));
                const auto* blob = reinterpret_cast<const char*>(sqlite3_column_blob(attachStmt, 1));
                row.data = QByteArray(blob, sqlite3_column_bytes(attachStmt, 1));
                attachRows.append(row);
                return true;
            })
        || !queryAllRows(
            m_db.handle(),
            "SELECT id, encrypted_label, encrypted_username, encrypted_password, "
            "encrypted_url, encrypted_notes, allow_subdomains, encrypted_fill_policy "
            "FROM credentials;",
            [&](sqlite3_stmt* credStmt) {
                CredRow row;
                row.id = sqlite3_column_int64(credStmt, 0);
                const auto readBlob = [&](int col) {
                    const auto* ptr = reinterpret_cast<const char*>(sqlite3_column_blob(credStmt, col));
                    return QByteArray(ptr, sqlite3_column_bytes(credStmt, col));
                };
                row.label = readBlob(1);
                row.username = readBlob(2);
                row.password = readBlob(3);
                row.url = readBlob(4);
                row.notes = readBlob(5);
                row.allowSubdomains = sqlite3_column_int(credStmt, 6) != 0;
                row.fillPolicy = readBlob(7);
                credRows.append(row);
                return true;
            })) {
        CryptoManager::secureZero(*newKey);
        return false;
    }

    struct PreparedNote { qint64 id; QByteArray title; QByteArray body; };
    struct PreparedAttach { QString id; QByteArray data; };
    struct PreparedCred {
        qint64 id = 0;
        QByteArray label;
        QByteArray username;
        QByteArray password;
        QByteArray url;
        QByteArray notes;
        QByteArray fillPolicy;
    };
    QVector<PreparedNote> preparedNotes;
    QVector<PreparedAttach> preparedAttachments;
    QVector<PreparedCred> preparedCreds;

    for (const NoteRow& row : noteRows) {
        auto plainTitle = CryptoManager::decryptField(
            row.title, currentKey, CryptoManager::noteTitleAssociatedData(row.id));
        if (!plainTitle) {
            plainTitle = CryptoManager::decryptLegacy(row.title, currentKey);
        }
        auto plainBody = CryptoManager::decryptField(
            row.body, currentKey, CryptoManager::noteBodyAssociatedData(row.id));
        if (!plainBody) {
            plainBody = CryptoManager::decryptLegacy(row.body, currentKey);
        }
        if (!plainTitle || !plainBody) {
            CryptoManager::secureZero(*newKey);
            return false;
        }

        QByteArray pt = *plainTitle;
        QByteArray pb = *plainBody;
        const auto encTitle = CryptoManager::encryptField(
            pt, *newKey, CryptoManager::noteTitleAssociatedData(row.id));
        const auto encBody = CryptoManager::encryptField(
            pb, *newKey, CryptoManager::noteBodyAssociatedData(row.id));
        CryptoManager::secureZero(pt);
        CryptoManager::secureZero(pb);
        if (!encTitle || !encBody) {
            CryptoManager::secureZero(*newKey);
            return false;
        }
        preparedNotes.append({row.id, *encTitle, *encBody});
    }

    for (const AttachRow& row : attachRows) {
        auto plain = CryptoManager::decryptField(
            row.data, currentKey, CryptoManager::attachmentAssociatedData(row.id));
        if (!plain) {
            plain = CryptoManager::decryptLegacy(row.data, currentKey);
        }
        if (!plain) {
            CryptoManager::secureZero(*newKey);
            return false;
        }
        QByteArray p = *plain;
        const auto enc = CryptoManager::encryptField(
            p, *newKey, CryptoManager::attachmentAssociatedData(row.id));
        CryptoManager::secureZero(p);
        if (!enc) {
            CryptoManager::secureZero(*newKey);
            return false;
        }
        preparedAttachments.append({row.id, *enc});
    }

    for (const CredRow& row : credRows) {
        static const char* kFields[] = {"label", "username", "password", "url", "notes"};
        const QByteArray* blobs[] = {&row.label, &row.username, &row.password, &row.url, &row.notes};
        PreparedCred out;
        out.id = row.id;
        QByteArray* outs[] = {&out.label, &out.username, &out.password, &out.url, &out.notes};
        for (int i = 0; i < 5; ++i) {
            auto plain = CryptoManager::decryptField(
                *blobs[i], currentKey, CryptoManager::credentialFieldAssociatedData(row.id, kFields[i]));
            if (!plain) {
                plain = CryptoManager::decryptLegacy(*blobs[i], currentKey);
            }
            if (!plain) {
                CryptoManager::secureZero(*newKey);
                return false;
            }
            QByteArray p = *plain;
            const auto enc = CryptoManager::encryptField(
                p, *newKey, CryptoManager::credentialFieldAssociatedData(row.id, kFields[i]));
            CryptoManager::secureZero(p);
            if (!enc) {
                CryptoManager::secureZero(*newKey);
                return false;
            }
            *outs[i] = *enc;
        }

        bool allowSubdomains = row.allowSubdomains;
        if (!row.fillPolicy.isEmpty()) {
            auto policyPlain = CryptoManager::decryptField(
                row.fillPolicy,
                currentKey,
                CryptoManager::credentialFillPolicyAssociatedData(row.id));
            if (policyPlain && !policyPlain->isEmpty()) {
                allowSubdomains = (*policyPlain)[0] != '\x00';
            }
        }
        const QByteArray policyPlainBytes(1, allowSubdomains ? '\x01' : '\x00');
        const auto encPolicy = CryptoManager::encryptField(
            policyPlainBytes,
            *newKey,
            CryptoManager::credentialFillPolicyAssociatedData(row.id));
        if (!encPolicy) {
            CryptoManager::secureZero(*newKey);
            return false;
        }
        out.fillPolicy = *encPolicy;
        preparedCreds.append(out);
    }

    const auto verifyEnc = CryptoManager::encryptField(
        QByteArray("GRIMLEDGER_OK"), *newKey, CryptoManager::verifyAssociatedData());
    if (!verifyEnc) {
        CryptoManager::secureZero(*newKey);
        return false;
    }

    sqlite3_stmt* verifyStmt = nullptr;
    const char* verifySql = "UPDATE app_metadata SET value = ? WHERE key = 'verify';";
    if (sqlite3_prepare_v2(m_db.handle(), verifySql, -1, &verifyStmt, nullptr) != SQLITE_OK) {
        CryptoManager::secureZero(*newKey);
        return false;
    }
    sqlite3_bind_blob(verifyStmt, 1, verifyEnc->constData(), verifyEnc->size(), SQLITE_TRANSIENT);
    if (!SqliteUtils::stepDone(verifyStmt) || !SqliteUtils::expectChanges(m_db.handle(), 1)) {
        sqlite3_finalize(verifyStmt);
        CryptoManager::secureZero(*newKey);
        return false;
    }
    sqlite3_finalize(verifyStmt);

    for (const PreparedNote& row : preparedNotes) {
        sqlite3_stmt* upd = nullptr;
        const char* sql =
            "UPDATE notes SET encrypted_title = ?, encrypted_body = ? WHERE id = ?;";
        if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &upd, nullptr) != SQLITE_OK) {
            CryptoManager::secureZero(*newKey);
            return false;
        }
        sqlite3_bind_blob(upd, 1, row.title.constData(), row.title.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 2, row.body.constData(), row.body.size(), SQLITE_TRANSIENT);
        sqlite3_bind_int64(upd, 3, row.id);
        if (!SqliteUtils::stepDone(upd) || !SqliteUtils::expectChanges(m_db.handle(), 1)) {
            sqlite3_finalize(upd);
            CryptoManager::secureZero(*newKey);
            return false;
        }
        sqlite3_finalize(upd);
    }

    for (const PreparedAttach& row : preparedAttachments) {
        sqlite3_stmt* upd = nullptr;
        const char* sql = "UPDATE note_attachments SET encrypted_data = ? WHERE id = ?;";
        if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &upd, nullptr) != SQLITE_OK) {
            CryptoManager::secureZero(*newKey);
            return false;
        }
        sqlite3_bind_blob(upd, 1, row.data.constData(), row.data.size(), SQLITE_TRANSIENT);
        sqlite3_bind_text(upd, 2, row.id.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        if (!SqliteUtils::stepDone(upd) || !SqliteUtils::expectChanges(m_db.handle(), 1)) {
            sqlite3_finalize(upd);
            CryptoManager::secureZero(*newKey);
            return false;
        }
        sqlite3_finalize(upd);
    }

    for (const PreparedCred& row : preparedCreds) {
        sqlite3_stmt* upd = nullptr;
        const char* sql =
            "UPDATE credentials SET encrypted_label = ?, encrypted_username = ?, "
            "encrypted_password = ?, encrypted_url = ?, encrypted_notes = ?, "
            "encrypted_fill_policy = ? WHERE id = ?;";
        if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &upd, nullptr) != SQLITE_OK) {
            CryptoManager::secureZero(*newKey);
            return false;
        }
        sqlite3_bind_blob(upd, 1, row.label.constData(), row.label.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 2, row.username.constData(), row.username.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 3, row.password.constData(), row.password.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 4, row.url.constData(), row.url.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 5, row.notes.constData(), row.notes.size(), SQLITE_TRANSIENT);
        sqlite3_bind_blob(upd, 6, row.fillPolicy.constData(), row.fillPolicy.size(), SQLITE_TRANSIENT);
        sqlite3_bind_int64(upd, 7, row.id);
        if (!SqliteUtils::stepDone(upd) || !SqliteUtils::expectChanges(m_db.handle(), 1)) {
            sqlite3_finalize(upd);
            CryptoManager::secureZero(*newKey);
            return false;
        }
        sqlite3_finalize(upd);
    }

    info->salt = newSalt;
    info->updatedAt = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());
    if (!storeVaultInfo(*info, true) || !tx.commit()) {
        CryptoManager::secureZero(*newKey);
        return false;
    }

    newKeyOut = std::move(*newKey);
    return true;
}

bool VaultRepository::verifyMasterPassword(const QString& password) const {
    const auto info = loadVaultInfo();
    if (!info) {
        return false;
    }
    auto key = CryptoManager::deriveKey(password, info->salt, info->kdfParams);
    if (!key) {
        return false;
    }
    const bool ok = verifyWithKey(m_db.handle(), *key, nullptr);
    CryptoManager::secureZero(*key);
    return ok;
}

bool VaultRepository::exportEncryptedBackupV2(const QString& destPath, const QString& password) {
    if (!verifyMasterPassword(password)) {
        return false;
    }

    sqlite3* snapshotDb = nullptr;
    if (sqlite3_open(":memory:", &snapshotDb) != SQLITE_OK) {
        return false;
    }
    sqlite3_backup* backup = sqlite3_backup_init(snapshotDb, "main", m_db.handle(), "main");
    if (!backup) {
        sqlite3_close(snapshotDb);
        return false;
    }
    int backupRc = SQLITE_OK;
    int backupRetries = 0;
    constexpr int kMaxBackupRetries = 20;
    do {
        backupRc = sqlite3_backup_step(backup, 128);
        if (backupRc == SQLITE_BUSY || backupRc == SQLITE_LOCKED) {
            if (++backupRetries > kMaxBackupRetries) {
                break;
            }
            QThread::msleep(50);
        }
    } while (backupRc == SQLITE_OK || backupRc == SQLITE_BUSY || backupRc == SQLITE_LOCKED);
    sqlite3_backup_finish(backup);
    if (backupRc != SQLITE_DONE) {
        sqlite3_close(snapshotDb);
        return false;
    }

    QString quickCheckError;
    if (!quickCheckOk(snapshotDb, &quickCheckError)) {
        sqlite3_close(snapshotDb);
        return false;
    }

    sqlite3_int64 serializedSize = 0;
    unsigned char* serialized = sqlite3_serialize(snapshotDb, "main", &serializedSize, 0);
    sqlite3_close(snapshotDb);
    if (!serialized || serializedSize <= 0
        || serializedSize > SecurityLimits::kMaxDatabaseBytes) {
        if (serialized) {
            sqlite3_free(serialized);
        }
        return false;
    }
    QByteArray dbData(reinterpret_cast<const char*>(serialized), static_cast<int>(serializedSize));
    sqlite3_free(serialized);
    if (dbData.isEmpty()) {
        return false;
    }

    const QByteArray salt = CryptoManager::randomBytes(CryptoManager::kSaltSize);
    const CryptoManager::KdfParams params = CryptoManager::defaultKdfParams();
    auto backupKey = CryptoManager::deriveKey(password, salt, params);
    if (!backupKey) {
        return false;
    }

    const QByteArray header = buildBackupHeaderV2(salt, params, static_cast<quint64>(dbData.size()));
    const QByteArray aad = CryptoManager::backupEnvelopeAssociatedData(header);
    const auto encrypted = CryptoManager::encryptAead(dbData, *backupKey, aad);
    CryptoManager::secureZero(*backupKey);
    if (!encrypted) {
        return false;
    }

    QSaveFile dest(destPath);
    if (!dest.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (dest.write(kBackupMagicV2, sizeof(kBackupMagicV2) - 1) != sizeof(kBackupMagicV2) - 1) {
        return false;
    }
    if (dest.write(header) != header.size()) {
        return false;
    }
    if (dest.write(*encrypted) != encrypted->size()) {
        return false;
    }
    return dest.commit();
}

bool VaultRepository::exportEncryptedBackupLegacy(const QByteArray& key, const QString& destPath) {
    QFile src(m_db.path());
    if (!src.open(QIODevice::ReadOnly)) {
        return false;
    }
    if (src.size() > SecurityLimits::kMaxDatabaseBytes) {
        return false;
    }
    const QByteArray dbData = src.readAll();
    src.close();

    const auto encrypted = CryptoManager::encryptLegacy(dbData, key);
    if (!encrypted) {
        return false;
    }

    QSaveFile dest(destPath);
    if (!dest.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (dest.write(kBackupMagicV1, sizeof(kBackupMagicV1) - 1) != sizeof(kBackupMagicV1) - 1) {
        return false;
    }
    if (dest.write(*encrypted) != encrypted->size()) {
        return false;
    }
    return dest.commit();
}

bool VaultRepository::validateVaultFile(const QString& path, QString* errorOut) {
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(
            path.toUtf8().constData(),
            &db,
            SQLITE_OPEN_READONLY,
            nullptr) != SQLITE_OK) {
        if (errorOut) {
            *errorOut = QStringLiteral("Could not open database for validation.");
        }
        return false;
    }

    const bool tablesOk = tableExists(db, "vault_metadata")
        && tableExists(db, "app_metadata")
        && tableExists(db, "notes")
        && tableExists(db, "credentials");
    if (!tablesOk) {
        if (errorOut) {
            *errorOut = QStringLiteral("Restored database schema is incomplete.");
        }
        sqlite3_close(db);
        return false;
    }

    const bool ok = quickCheckOk(db, errorOut) && validateVaultMetadataRow(db, errorOut);
    sqlite3_close(db);
    return ok;
}

RestoreResult VaultRepository::restoreFromBackup(
    const QString& backupPath,
    const QString& password,
    const QString& liveVaultPath,
    QString* errorOut) {
    QFile backup(backupPath);
    if (!backup.open(QIODevice::ReadOnly)) {
        if (errorOut) {
            *errorOut = QStringLiteral("Could not open backup file.");
        }
        return RestoreResult::Failed;
    }
    if (backup.size() > SecurityLimits::kMaxBackupFileBytes) {
        if (errorOut) {
            *errorOut = QStringLiteral("Backup file is too large.");
        }
        return RestoreResult::Failed;
    }

    char magic[9] = {};
    if (!readExact(backup, magic, 9)) {
        if (errorOut) {
            *errorOut = QStringLiteral("Backup file is truncated.");
        }
        return RestoreResult::Failed;
    }

    QByteArray plaintext;
    if (qstrncmp(magic, kBackupMagicV2, 9) == 0) {
        QByteArray header = backup.read(43);
        if (header.size() != 43) {
            if (errorOut) {
                *errorOut = QStringLiteral("Backup header is invalid.");
            }
            return RestoreResult::Failed;
        }

        QByteArray salt;
        CryptoManager::KdfParams params;
        quint64 expectedPlainLen = 0;
        if (!parseBackupHeaderV2(header, salt, params, expectedPlainLen)
            || expectedPlainLen > static_cast<quint64>(SecurityLimits::kMaxDatabaseBytes)) {
            if (errorOut) {
                *errorOut = QStringLiteral("Backup header metadata is invalid.");
            }
            return RestoreResult::Failed;
        }

        const auto payload = readBounded(backup, SecurityLimits::kMaxBackupFileBytes);
        if (!payload || payload->size() < CryptoManager::kNonceSize) {
            if (errorOut) {
                *errorOut = QStringLiteral("Backup payload is invalid.");
            }
            return RestoreResult::Failed;
        }

        auto backupKey = CryptoManager::deriveKey(password, salt, params);
        if (!backupKey) {
            if (errorOut) {
                *errorOut = QStringLiteral("Could not derive backup key.");
            }
            return RestoreResult::Failed;
        }

        const QByteArray aad = CryptoManager::backupEnvelopeAssociatedData(header);
        const auto decrypted = CryptoManager::decryptAead(*payload, *backupKey, aad);
        CryptoManager::secureZero(*backupKey);
        if (!decrypted) {
            if (errorOut) {
                *errorOut = QStringLiteral("Could not decrypt backup. Wrong password or corrupted file.");
            }
            return RestoreResult::Failed;
        }
        if (static_cast<quint64>(decrypted->size()) != expectedPlainLen) {
            if (errorOut) {
                *errorOut = QStringLiteral("Decrypted backup size mismatch.");
            }
            QByteArray tmp = *decrypted;
            CryptoManager::secureZero(tmp);
            return RestoreResult::Failed;
        }
        plaintext = std::move(*decrypted);
    } else if (qstrncmp(magic, kBackupMagicV1, 9) == 0) {
        if (errorOut) {
            *errorOut = QStringLiteral(
                "Legacy backup format requires an unlocked vault session. "
                "Use a GRIMBKUP2 backup for standalone restore.");
        }
        return RestoreResult::Failed;
    } else {
        if (errorOut) {
            *errorOut = QStringLiteral("Unknown backup format.");
        }
        return RestoreResult::Failed;
    }

    const QFileInfo liveInfo(liveVaultPath);
    QDir().mkpath(liveInfo.absolutePath());
    const QString tempPath = liveInfo.absolutePath() + QStringLiteral("/vault.restore.tmp");
    QFile::remove(tempPath);

    QSaveFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        CryptoManager::secureZero(plaintext);
        if (errorOut) {
            *errorOut = QStringLiteral("Could not create temporary restore file.");
        }
        return RestoreResult::Failed;
    }
    if (tempFile.write(plaintext) != plaintext.size()) {
        CryptoManager::secureZero(plaintext);
        if (errorOut) {
            *errorOut = QStringLiteral("Could not write temporary restore file.");
        }
        return RestoreResult::Failed;
    }
    CryptoManager::secureZero(plaintext);
    if (!tempFile.commit()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Could not finalize temporary restore file.");
        }
        return RestoreResult::Failed;
    }

    QString validationError;
    if (!validateVaultFile(tempPath, &validationError)) {
        QFile::remove(tempPath);
        if (errorOut) {
            *errorOut = validationError;
        }
        return RestoreResult::Failed;
    }

    sqlite3* tempDb = nullptr;
    if (sqlite3_open_v2(tempPath.toUtf8().constData(), &tempDb, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        QFile::remove(tempPath);
        if (errorOut) {
            *errorOut = QStringLiteral("Restored database could not be opened for verification.");
        }
        return RestoreResult::Failed;
    }

    VaultInfo info;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT vault_version, salt, kdf_ops_limit, kdf_mem_limit "
        "FROM vault_metadata WHERE id = 1;";
    if (sqlite3_prepare_v2(tempDb, sql, -1, &stmt, nullptr) != SQLITE_OK
        || sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        sqlite3_close(tempDb);
        QFile::remove(tempPath);
        if (errorOut) {
            *errorOut = QStringLiteral("Restored vault metadata is missing.");
        }
        return RestoreResult::Failed;
    }
    info.version = sqlite3_column_int(stmt, 0);
    const auto* saltPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 1));
    info.salt = QByteArray(saltPtr, sqlite3_column_bytes(stmt, 1));
    info.kdfParams.opsLimit = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 2));
    info.kdfParams.memLimit = static_cast<size_t>(sqlite3_column_int64(stmt, 3));
    sqlite3_finalize(stmt);

    auto key = CryptoManager::deriveKey(password, info.salt, info.kdfParams);
    if (!key) {
        sqlite3_close(tempDb);
        QFile::remove(tempPath);
        if (errorOut) {
            *errorOut = QStringLiteral("Could not derive vault key from backup password.");
        }
        return RestoreResult::Failed;
    }

    const bool cryptoOk = verifyWithKey(tempDb, *key, &validationError);
    CryptoManager::secureZero(*key);
    sqlite3_close(tempDb);
    if (!cryptoOk) {
        QFile::remove(tempPath);
        if (errorOut) {
            *errorOut = validationError;
        }
        return RestoreResult::Failed;
    }

    return installValidatedPlaintextVault(tempPath, liveVaultPath, errorOut);
}

RestoreResult VaultRepository::restoreLegacyBackupInSession(
    const QString& backupPath,
    const QByteArray& sessionKey,
    const QString& liveVaultPath,
    QString* errorOut) {
    QFile backup(backupPath);
    if (!backup.open(QIODevice::ReadOnly)) {
        if (errorOut) {
            *errorOut = QStringLiteral("Could not open backup file.");
        }
        return RestoreResult::Failed;
    }
    if (backup.size() > SecurityLimits::kMaxBackupFileBytes) {
        if (errorOut) {
            *errorOut = QStringLiteral("Backup file is too large.");
        }
        return RestoreResult::Failed;
    }

    char magic[9] = {};
    if (!readExact(backup, magic, 9) || qstrncmp(magic, kBackupMagicV1, 9) != 0) {
        if (errorOut) {
            *errorOut = QStringLiteral("Not a legacy GRIMBKUP1 backup.");
        }
        return RestoreResult::Failed;
    }

    const auto payload = readBounded(backup, SecurityLimits::kMaxBackupFileBytes);
    if (!payload) {
        if (errorOut) {
            *errorOut = QStringLiteral("Backup payload is invalid.");
        }
        return RestoreResult::Failed;
    }

    const auto decrypted = CryptoManager::decryptLegacy(*payload, sessionKey);
    if (!decrypted) {
        if (errorOut) {
            *errorOut = QStringLiteral("Could not decrypt legacy backup with the current vault key.");
        }
        return RestoreResult::Failed;
    }
    QByteArray plain = *decrypted;
    QByteArray tmp = std::move(*decrypted);
    CryptoManager::secureZero(tmp);
    if (plain.size() > SecurityLimits::kMaxDatabaseBytes) {
        CryptoManager::secureZero(plain);
        if (errorOut) {
            *errorOut = QStringLiteral("Decrypted backup is too large.");
        }
        return RestoreResult::Failed;
    }

    const QFileInfo liveInfo(liveVaultPath);
    QDir().mkpath(liveInfo.absolutePath());
    const QString tempPath = liveInfo.absolutePath() + QStringLiteral("/vault.restore.tmp");
    QFile::remove(tempPath);

    QSaveFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        CryptoManager::secureZero(plain);
        if (errorOut) {
            *errorOut = QStringLiteral("Could not create temporary restore file.");
        }
        return RestoreResult::Failed;
    }
    if (tempFile.write(plain) != plain.size()) {
        CryptoManager::secureZero(plain);
        if (errorOut) {
            *errorOut = QStringLiteral("Could not write temporary restore file.");
        }
        return RestoreResult::Failed;
    }
    CryptoManager::secureZero(plain);
    if (!tempFile.commit()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Could not finalize temporary restore file.");
        }
        return RestoreResult::Failed;
    }

    QString validationError;
    if (!validateVaultFile(tempPath, &validationError)) {
        QFile::remove(tempPath);
        if (errorOut) {
            *errorOut = validationError;
        }
        return RestoreResult::Failed;
    }

    return installValidatedPlaintextVault(tempPath, liveVaultPath, errorOut);
}

RestoreResult VaultRepository::installStagedVault(
    const QString& stagedPath,
    const QString& liveVaultPath,
    QString* errorOut) {
    return installValidatedPlaintextVault(stagedPath, liveVaultPath, errorOut);
}

RestoreResult VaultRepository::installValidatedPlaintextVault(
    const QString& tempPath,
    const QString& liveVaultPath,
    QString* errorOut) {
    const QFileInfo liveInfo(liveVaultPath);
    const QString backupLivePath = liveInfo.absolutePath() + QStringLiteral("/vault.grim.bak");

    if (QFile::exists(liveVaultPath) && QFile::exists(backupLivePath)) {
        QFile::remove(backupLivePath);
    }
    if (QFile::exists(liveVaultPath) && !QFile::rename(liveVaultPath, backupLivePath)) {
        QFile::remove(tempPath);
        if (errorOut) {
            *errorOut = QStringLiteral("Could not preserve the existing vault before restore.");
        }
        return RestoreResult::Failed;
    }

    if (!QFile::rename(tempPath, liveVaultPath)) {
        if (QFile::exists(backupLivePath)) {
            if (!QFile::remove(liveVaultPath)) {
                QFile::remove(tempPath);
                if (errorOut) {
                    *errorOut = QStringLiteral("Restore install failed and rollback was blocked.");
                }
                return RestoreResult::Failed;
            }
            if (!QFile::rename(backupLivePath, liveVaultPath)) {
                QFile::remove(tempPath);
                if (errorOut) {
                    *errorOut = QStringLiteral("Restore install failed and rollback failed.");
                }
                return RestoreResult::Failed;
            }
        }
        QFile::remove(tempPath);
        if (errorOut) {
            *errorOut = QStringLiteral("Could not install restored vault.");
        }
        return RestoreResult::Failed;
    }

    sqlite3* installedDb = nullptr;
    if (sqlite3_open_v2(liveVaultPath.toUtf8().constData(), &installedDb, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK
        || !quickCheckOk(installedDb, errorOut)) {
        if (installedDb) {
            sqlite3_close(installedDb);
        }
        QFile::remove(liveVaultPath);
        if (QFile::exists(backupLivePath) && !QFile::rename(backupLivePath, liveVaultPath)) {
            if (errorOut) {
                *errorOut = QStringLiteral("Installed vault failed validation and rollback failed.");
            }
            return RestoreResult::Failed;
        }
        if (errorOut && errorOut->isEmpty()) {
            *errorOut = QStringLiteral("Installed vault failed validation and was rolled back.");
        }
        return RestoreResult::Failed;
    }
    sqlite3_close(installedDb);
    if (QFile::exists(backupLivePath) && !QFile::remove(backupLivePath)) {
        if (errorOut) {
            *errorOut = QStringLiteral(
                "Vault restored successfully, but the old vault backup file could not be removed.");
        }
        return RestoreResult::InstalledWithCleanupWarning;
    }
    return RestoreResult::Installed;
}
