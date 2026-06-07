#include "storage/SealedBlockRepository.h"
#include "security/CryptoManager.h"
#include "utils/SqliteUtils.h"
#include "utils/TimeUtils.h"

#include <QUuid>
#include <QRegularExpression>
#include <sqlite3.h>

namespace {

const QRegularExpression kSealedAuthorRe(
    QStringLiteral(R"(:::sealed\s+([^\n]+)\n([\s\S]*?)\n:::)"));
const QRegularExpression kSealedMarkerRe(
    QStringLiteral(R"(\[\[sealed:([0-9a-f\-]+):([^\]]+)\]\])"));

QString makeMarker(const QString& blockId, const QString& label) {
    return QStringLiteral("[[sealed:%1:%2]]").arg(blockId, label);
}

} // namespace

SealedBlockRepository::SealedBlockRepository(Database& db)
    : m_db(db) {}

bool SealedBlockRepository::upsertBlock(
    qint64 noteId,
    const QString& blockId,
    const QString& label,
    const QString& content,
    const QByteArray& key) {
    const auto encLabel = CryptoManager::encryptField(
        label.toUtf8(), key, CryptoManager::sealedBlockAssociatedData(noteId, blockId));
    const auto encContent = CryptoManager::encryptField(
        content.toUtf8(), key, CryptoManager::sealedBlockAssociatedData(noteId, blockId + QStringLiteral(":body")));
    if (!encLabel || !encContent) {
        return false;
    }

    const qint64 now = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO sealed_blocks (id, note_id, encrypted_label, encrypted_content, created_at) "
        "VALUES (?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET encrypted_label = excluded.encrypted_label, "
        "encrypted_content = excluded.encrypted_content;";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    const QByteArray idUtf8 = blockId.toUtf8();
    sqlite3_bind_text(stmt, 1, idUtf8.constData(), idUtf8.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, noteId);
    sqlite3_bind_blob(stmt, 3, encLabel->constData(), encLabel->size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 4, encContent->constData(), encContent->size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, now);
    const bool ok = SqliteUtils::stepDone(stmt);
    sqlite3_finalize(stmt);
    return ok;
}

QString SealedBlockRepository::processBodyForSave(
    qint64 noteId,
    QString body,
    const QByteArray& key) {
    QRegularExpressionMatchIterator it = kSealedAuthorRe.globalMatch(body);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString label = match.captured(1).trimmed();
        const QString content = match.captured(2);
        const QString blockId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (upsertBlock(noteId, blockId, label, content, key)) {
            body.replace(match.capturedStart(), match.capturedLength(), makeMarker(blockId, label));
        }
    }
    return body;
}

QString SealedBlockRepository::expandBodyForEdit(
    qint64 noteId,
    const QString& body,
    const QByteArray& key) const {
    Q_UNUSED(noteId);
    Q_UNUSED(key);
    return body;
}

QString SealedBlockRepository::strippedBodyForSearch(qint64 noteId, const QString& body) const {
    Q_UNUSED(noteId);
    QString stripped = body;
    stripped.replace(kSealedMarkerRe, QStringLiteral("[sealed]"));
    stripped.replace(kSealedAuthorRe, QStringLiteral("[sealed]"));
    return stripped;
}

QString SealedBlockRepository::strippedBodyForExport(qint64 noteId, const QString& body) const {
    return strippedBodyForSearch(noteId, body);
}

std::optional<QString> SealedBlockRepository::revealBlock(
    qint64 noteId,
    const QString& blockId,
    const QByteArray& key) const {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT encrypted_content FROM sealed_blocks WHERE id = ? AND note_id = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    const QByteArray idUtf8 = blockId.toUtf8();
    sqlite3_bind_text(stmt, 1, idUtf8.constData(), idUtf8.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, noteId);
    std::optional<QString> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* ptr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 0));
        const QByteArray blob(ptr, sqlite3_column_bytes(stmt, 0));
        const auto dec = CryptoManager::decryptField(
            blob, key, CryptoManager::sealedBlockAssociatedData(noteId, blockId + QStringLiteral(":body")));
        if (dec) {
            result = QString::fromUtf8(*dec);
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

bool SealedBlockRepository::deleteBlocksForNote(qint64 noteId) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM sealed_blocks WHERE note_id = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, noteId);
    const bool ok = SqliteUtils::stepDone(stmt);
    sqlite3_finalize(stmt);
    return ok;
}
