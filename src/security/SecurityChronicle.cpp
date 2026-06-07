#include "security/SecurityChronicle.h"
#include "security/CryptoManager.h"
#include "utils/SqliteUtils.h"
#include "utils/TimeUtils.h"

#include <QCryptographicHash>
#include <sqlite3.h>

namespace {

QByteArray lastEntryHash(sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT entry_hash FROM security_chronicle ORDER BY id DESC LIMIT 1;";
    QByteArray hash;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK
        && sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* ptr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 0));
        hash = QByteArray(ptr, sqlite3_column_bytes(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return hash;
}

QByteArray computeEntryHash(
    const QByteArray& prevHash,
    const QString& eventType,
    const QString& summary,
    qint64 createdAt) {
    QCryptographicHash hasher(QCryptographicHash::Sha256);
    hasher.addData(prevHash);
    hasher.addData(eventType.toUtf8());
    hasher.addData(summary.toUtf8());
    hasher.addData(QByteArray::number(createdAt));
    return hasher.result();
}

} // namespace

SecurityChronicle::SecurityChronicle(Database& db)
    : m_db(db) {}

bool SecurityChronicle::appendEvent(
    const QString& eventType,
    const QString& summary,
    const QByteArray& key) {
    const qint64 now = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());
    const QByteArray prevHash = lastEntryHash(m_db.handle());
    const QByteArray entryHash = computeEntryHash(prevHash, eventType, summary, now);

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO security_chronicle "
        "(event_type, encrypted_metadata, prev_hash, entry_hash, created_at) "
        "VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    const QByteArray eventUtf8 = eventType.toUtf8();
    const QByteArray summaryUtf8 = summary.toUtf8();
    const auto encMeta = CryptoManager::encryptField(summaryUtf8, key, QByteArray("grim:chronicle:pending"));
    if (!encMeta) {
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_bind_text(stmt, 1, eventUtf8.constData(), eventUtf8.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 2, encMeta->constData(), encMeta->size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 3, prevHash.constData(), prevHash.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 4, entryHash.constData(), entryHash.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, now);
    if (!SqliteUtils::stepDone(stmt)) {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);

    const qint64 entryId = sqlite3_last_insert_rowid(m_db.handle());
    const auto encMetaFinal = CryptoManager::encryptField(
        summaryUtf8, key, CryptoManager::chronicleEntryAssociatedData(entryId));
    if (!encMetaFinal) {
        return false;
    }
    sqlite3_stmt* upd = nullptr;
    const char* updSql = "UPDATE security_chronicle SET encrypted_metadata = ? WHERE id = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), updSql, -1, &upd, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_blob(upd, 1, encMetaFinal->constData(), encMetaFinal->size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(upd, 2, entryId);
    const bool ok = SqliteUtils::stepDone(upd);
    sqlite3_finalize(upd);
    return ok;
}

QVector<ChronicleEntry> SecurityChronicle::listEntries(const QByteArray& key, int limit) const {
    QVector<ChronicleEntry> entries;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, event_type, encrypted_metadata, prev_hash, entry_hash, created_at "
        "FROM security_chronicle ORDER BY id DESC LIMIT ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return entries;
    }
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ChronicleEntry entry;
        entry.id = sqlite3_column_int64(stmt, 0);
        entry.eventType = QString::fromUtf8(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        const auto* metaPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 2));
        const QByteArray metaBlob(metaPtr, sqlite3_column_bytes(stmt, 2));
        const auto dec = CryptoManager::decryptField(
            metaBlob, key, CryptoManager::chronicleEntryAssociatedData(entry.id));
        if (dec) {
            entry.summary = QString::fromUtf8(*dec);
        }
        const auto* prevPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 3));
        entry.prevHash = QByteArray(prevPtr, sqlite3_column_bytes(stmt, 3));
        const auto* hashPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 4));
        entry.entryHash = QByteArray(hashPtr, sqlite3_column_bytes(stmt, 4));
        entry.createdAt = TimeUtils::fromUnix(sqlite3_column_int64(stmt, 5));
        entries.append(entry);
    }
    sqlite3_finalize(stmt);
    return entries;
}

bool SecurityChronicle::verifyChain(const QByteArray& key) const {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, event_type, encrypted_metadata, prev_hash, entry_hash, created_at "
        "FROM security_chronicle ORDER BY id ASC;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    QByteArray expectedPrev;
    bool ok = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const qint64 entryId = sqlite3_column_int64(stmt, 0);
        const QString eventType = QString::fromUtf8(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        const auto* hashPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 4));
        const QByteArray storedHash(hashPtr, sqlite3_column_bytes(stmt, 4));
        const auto* prevPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 3));
        const QByteArray prevHash(prevPtr, sqlite3_column_bytes(stmt, 3));
        const qint64 createdAt = sqlite3_column_int64(stmt, 5);

        if (prevHash != expectedPrev) {
            ok = false;
            break;
        }
        const auto* metaPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 2));
        const QByteArray metaBlob(metaPtr, sqlite3_column_bytes(stmt, 2));
        QString summary;
        if (auto dec = CryptoManager::decryptField(
                metaBlob, key, CryptoManager::chronicleEntryAssociatedData(entryId))) {
            summary = QString::fromUtf8(*dec);
        }
        const QByteArray recomputed = computeEntryHash(prevHash, eventType, summary, createdAt);
        if (recomputed != storedHash) {
            ok = false;
            break;
        }
        expectedPrev = storedHash;
    }
    sqlite3_finalize(stmt);
    return ok;
}
