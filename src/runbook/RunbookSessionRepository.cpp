#include "runbook/RunbookSessionRepository.h"
#include "security/CryptoManager.h"
#include "utils/SqliteUtils.h"
#include "utils/TimeUtils.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <sqlite3.h>

namespace {

QByteArray serializeState(const RunbookSessionState& state) {
    QJsonObject obj;
    obj.insert(QStringLiteral("currentStep"), state.currentStep);
    obj.insert(QStringLiteral("notes"), state.notes);
    QJsonArray completed;
    for (bool done : state.completed) {
        completed.append(done);
    }
    obj.insert(QStringLiteral("completed"), completed);
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

RunbookSessionState deserializeState(qint64 id, qint64 noteId, const QByteArray& json) {
    RunbookSessionState state;
    state.id = id;
    state.noteId = noteId;
    const QJsonObject obj = QJsonDocument::fromJson(json).object();
    state.currentStep = obj.value(QStringLiteral("currentStep")).toInt();
    state.notes = obj.value(QStringLiteral("notes")).toString();
    for (const QJsonValue& value : obj.value(QStringLiteral("completed")).toArray()) {
        state.completed.append(value.toBool());
    }
    return state;
}

} // namespace

RunbookSessionRepository::RunbookSessionRepository(Database& db)
    : m_db(db) {}

qint64 RunbookSessionRepository::createSession(qint64 noteId, int stepCount, const QByteArray& key) {
    RunbookSessionState state;
    state.noteId = noteId;
    state.currentStep = 0;
    state.completed = QVector<bool>(stepCount, false);

    const qint64 now = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO runbook_sessions (note_id, encrypted_state, created_at, updated_at) "
        "VALUES (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    const QByteArray placeholder = QByteArray("{}");
    sqlite3_bind_int64(stmt, 1, noteId);
    sqlite3_bind_blob(stmt, 2, placeholder.constData(), placeholder.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, now);
    sqlite3_bind_int64(stmt, 4, now);
    if (!SqliteUtils::stepDone(stmt)) {
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);

    const qint64 sessionId = sqlite3_last_insert_rowid(m_db.handle());
    state.id = sessionId;
    if (!saveState(state, key)) {
        return 0;
    }
    return sessionId;
}

bool RunbookSessionRepository::saveState(const RunbookSessionState& state, const QByteArray& key) {
    const auto enc = CryptoManager::encryptField(
        serializeState(state),
        key,
        CryptoManager::runbookSessionAssociatedData(state.id));
    if (!enc) {
        return false;
    }
    const qint64 now = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE runbook_sessions SET encrypted_state = ?, updated_at = ? WHERE id = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_blob(stmt, 1, enc->constData(), enc->size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, now);
    sqlite3_bind_int64(stmt, 3, state.id);
    const bool ok = SqliteUtils::stepDone(stmt);
    sqlite3_finalize(stmt);
    return ok;
}

std::optional<RunbookSessionState> RunbookSessionRepository::loadSession(
    qint64 sessionId,
    const QByteArray& key) const {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT note_id, encrypted_state FROM runbook_sessions WHERE id = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, sessionId);
    std::optional<RunbookSessionState> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const qint64 noteId = sqlite3_column_int64(stmt, 0);
        const auto* ptr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 1));
        const QByteArray blob(ptr, sqlite3_column_bytes(stmt, 1));
        const auto dec = CryptoManager::decryptField(
            blob, key, CryptoManager::runbookSessionAssociatedData(sessionId));
        if (dec) {
            result = deserializeState(sessionId, noteId, *dec);
        }
    }
    sqlite3_finalize(stmt);
    return result;
}
