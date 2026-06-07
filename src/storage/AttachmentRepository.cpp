#include "storage/AttachmentRepository.h"
#include "security/CryptoManager.h"
#include "storage/DbTransaction.h"
#include "utils/SecurityLimits.h"
#include "utils/TimeUtils.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>
#include <sqlite3.h>

AttachmentRepository::AttachmentRepository(Database& db)
    : m_db(db) {}

namespace {

qint64 totalAttachmentPlaintextBytes(sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COALESCE(SUM(length(encrypted_data)), 0) FROM note_attachments;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    qint64 total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return total;
}

} // namespace

bool AttachmentRepository::isGrimAttachmentUrl(const QString& url) {
    return url.startsWith(QString::fromUtf8(kGrimScheme));
}

QString AttachmentRepository::attachmentIdFromUrl(const QString& url) {
    if (!isGrimAttachmentUrl(url)) {
        return QString();
    }
    return url.mid(QString::fromUtf8(kGrimScheme).size());
}

QString AttachmentRepository::storeAttachment(
    qint64 noteId,
    const QByteArray& imageData,
    const QString& mimeType,
    const QString& originalName,
    const QByteArray& key) {
    if (noteId <= 0 || imageData.isEmpty() || key.isEmpty()) {
        return QString();
    }

    const qint64 currentBytes = totalAttachmentPlaintextBytes(m_db.handle());
    if (currentBytes + imageData.size() > SecurityLimits::kMaxVaultAttachmentBytes) {
        return QString();
    }

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto encrypted = CryptoManager::encryptField(
        imageData,
        key,
        CryptoManager::attachmentAssociatedData(id));
    if (!encrypted) {
        return QString();
    }
    const qint64 now = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO note_attachments (id, note_id, encrypted_data, mime_type, original_name, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return QString();
    }

    sqlite3_bind_text(stmt, 1, id.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, noteId);
    sqlite3_bind_blob(stmt, 3, encrypted->constData(), encrypted->size(), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, mimeType.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, originalName.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, now);

    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok ? id : QString();
}

std::optional<QByteArray> AttachmentRepository::loadAttachment(
    const QString& attachmentId,
    qint64 noteId,
    const QByteArray& key) const {
    if (attachmentId.isEmpty() || noteId <= 0 || key.isEmpty()) {
        return std::nullopt;
    }

    static const QRegularExpression uuidRe(
        QStringLiteral("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"));
    if (!uuidRe.match(attachmentId).hasMatch()) {
        return std::nullopt;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT encrypted_data FROM note_attachments WHERE id = ? AND note_id = ?;";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, attachmentId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, noteId);

    std::optional<QByteArray> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* blob = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 0));
        const int len = sqlite3_column_bytes(stmt, 0);
        const auto decrypted = CryptoManager::decryptField(
            QByteArray(blob, len),
            key,
            CryptoManager::attachmentAssociatedData(attachmentId));
        if (decrypted) {
            result = *decrypted;
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

QHash<QString, QString> AttachmentRepository::duplicateAttachments(
    qint64 fromNoteId,
    qint64 toNoteId,
    const QByteArray& key) const {
    QHash<QString, QString> idMap;
    if (fromNoteId <= 0 || toNoteId <= 0) {
        return idMap;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, encrypted_data, mime_type, original_name, created_at "
        "FROM note_attachments WHERE note_id = ?;";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return idMap;
    }

    sqlite3_bind_int64(stmt, 1, fromNoteId);

    struct Row {
        QString id;
        QByteArray data;
        QString mime;
        QString name;
        qint64 createdAt = 0;
    };

    QList<Row> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Row row;
        row.id = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        const auto* blob = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 1));
        row.data = QByteArray(blob, sqlite3_column_bytes(stmt, 1));
        row.mime = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        row.name = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        row.createdAt = sqlite3_column_int64(stmt, 4);
        rows.append(row);
    }
    sqlite3_finalize(stmt);

    const char* insSql =
        "INSERT INTO note_attachments (id, note_id, encrypted_data, mime_type, original_name, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    DbTransaction tx(m_db);
    if (!tx.isActive()) {
        return idMap;
    }

    for (const Row& row : rows) {
        const QString newId = QUuid::createUuid().toString(QUuid::WithoutBraces);

        auto plain = CryptoManager::decryptField(
            row.data, key, CryptoManager::attachmentAssociatedData(row.id));
        if (!plain) {
            plain = CryptoManager::decryptLegacy(row.data, key);
        }
        if (!plain) {
            return idMap;
        }

        const auto encrypted = CryptoManager::encryptField(
            *plain, key, CryptoManager::attachmentAssociatedData(newId));
        CryptoManager::secureZero(*plain);
        if (!encrypted) {
            return idMap;
        }

        sqlite3_stmt* ins = nullptr;
        if (sqlite3_prepare_v2(m_db.handle(), insSql, -1, &ins, nullptr) != SQLITE_OK) {
            return idMap;
        }

        sqlite3_bind_text(ins, 1, newId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(ins, 2, toNoteId);
        sqlite3_bind_blob(ins, 3, encrypted->constData(), encrypted->size(), SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 4, row.mime.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 5, row.name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(ins, 6, row.createdAt);
        const bool ok = sqlite3_step(ins) == SQLITE_DONE;
        sqlite3_finalize(ins);
        if (!ok) {
            return QHash<QString, QString>();
        }
        idMap.insert(row.id, newId);
    }

    if (!tx.commit()) {
        return QHash<QString, QString>();
    }
    return idMap;
}

QString AttachmentRepository::remapAttachmentUrls(
    const QString& body,
    const QHash<QString, QString>& idMap) const {
    QString result = body;
    for (auto it = idMap.constBegin(); it != idMap.constEnd(); ++it) {
        const QString oldUrl = QString::fromUtf8(kGrimScheme) + it.key();
        const QString newUrl = QString::fromUtf8(kGrimScheme) + it.value();
        result.replace(oldUrl, newUrl);
    }
    return result;
}

QString AttachmentRepository::rewriteBodyForExport(
    const QString& body,
    qint64 noteId,
    const QString& exportDir,
    const QByteArray& key) const {
    QDir().mkpath(exportDir);

    static const QRegularExpression imageRe(
        QStringLiteral(R"(!\[([^\]]*)\]\((grim://attachment/[^)]+)\))"));

    QString result = body;
    QRegularExpressionMatchIterator it = imageRe.globalMatch(body);
    QList<QRegularExpressionMatch> matches;
    while (it.hasNext()) {
        matches.append(it.next());
    }

    for (const QRegularExpressionMatch& match : matches) {
        const QString grimUrl = match.captured(2);
        const QString attachmentId = attachmentIdFromUrl(grimUrl);
        const auto data = loadAttachment(attachmentId, noteId, key);
        if (!data) {
            continue;
        }

        const QString fileName = attachmentId + QStringLiteral(".png");
        const QString filePath = exportDir + QLatin1Char('/') + fileName;
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            continue;
        }
        file.write(*data);
        file.close();

        const QString relative = QFileInfo(exportDir).fileName() + QLatin1Char('/') + fileName;
        result.replace(grimUrl, relative);
    }

    return result;
}
