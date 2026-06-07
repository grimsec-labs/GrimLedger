#include "storage/NoteRepository.h"
#include "security/CryptoManager.h"
#include "utils/SecurityLimits.h"
#include "utils/TimeUtils.h"

#include <QFile>
#include <QFileInfo>
#include <sqlite3.h>

#include <algorithm>
#include <optional>

NoteRepository::NoteRepository(Database& db)
    : m_db(db) {}

QByteArray NoteRepository::encryptNoteField(
    const QString& text,
    qint64 noteId,
    bool isBody,
    const QByteArray& key) const {
    const QByteArray aad = isBody
        ? CryptoManager::noteBodyAssociatedData(noteId)
        : CryptoManager::noteTitleAssociatedData(noteId);
    const auto enc = CryptoManager::encryptField(text.toUtf8(), key, aad);
    return enc.value_or(QByteArray());
}

QString NoteRepository::decryptNoteField(
    const QByteArray& blob,
    qint64 noteId,
    bool isBody,
    const QByteArray& key) const {
    const QByteArray aad = isBody
        ? CryptoManager::noteBodyAssociatedData(noteId)
        : CryptoManager::noteTitleAssociatedData(noteId);
    const auto dec = CryptoManager::decryptField(blob, key, aad);
    if (!dec) {
        return QString();
    }
    return QString::fromUtf8(*dec);
}

QVector<QString> NoteRepository::getTagsForNote(qint64 noteId) const {
    QVector<QString> tags;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT t.name FROM tags t "
        "JOIN note_tags nt ON nt.tag_id = t.id "
        "WHERE nt.note_id = ? ORDER BY t.name;";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return tags;
    }
    sqlite3_bind_int64(stmt, 1, noteId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        tags.append(QString::fromUtf8(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
    }
    sqlite3_finalize(stmt);
    return tags;
}

QVector<Note> NoteRepository::listNotes(
    const QByteArray& key,
    NoteSortField sort,
    bool descending,
    qint64 folderId,
    bool favoritesOnly,
    int recentLimit) const {
    QVector<Note> notes;

    QString orderBy = QStringLiteral("n.updated_at");
    switch (sort) {
    case NoteSortField::Title: orderBy = QStringLiteral("n.id"); break; // title encrypted; sort client-side
    case NoteSortField::Created: orderBy = QStringLiteral("n.created_at"); break;
    case NoteSortField::Modified: orderBy = QStringLiteral("n.updated_at"); break;
    case NoteSortField::Folder: orderBy = QStringLiteral("n.folder_id"); break;
    case NoteSortField::Favorite: orderBy = QStringLiteral("n.is_favorite"); break;
    }

    QString sql = QStringLiteral(
        "SELECT n.id, n.encrypted_title, n.encrypted_body, n.folder_id, "
        "n.created_at, n.updated_at, n.is_favorite, COALESCE(f.name, '') "
        "FROM notes n LEFT JOIN folders f ON f.id = n.folder_id WHERE 1=1 ");

    if (folderId >= 0) {
        sql += QStringLiteral("AND n.folder_id = %1 ").arg(folderId);
    }
    if (favoritesOnly) {
        sql += QStringLiteral("AND n.is_favorite = 1 ");
    }

    sql += QStringLiteral("ORDER BY %1 %2").arg(orderBy, descending ? "DESC" : "ASC");
    if (recentLimit > 0) {
        sql += QStringLiteral(" LIMIT %1").arg(recentLimit);
    }
    sql += ';';

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db.handle(), sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) {
        return notes;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Note n;
        n.id = sqlite3_column_int64(stmt, 0);
        const auto* tPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 1));
        n.title = decryptNoteField(
            QByteArray(tPtr, sqlite3_column_bytes(stmt, 1)), n.id, false, key);
        // Body not loaded in list view for performance — only title needed.
        n.folderId = sqlite3_column_int64(stmt, 3);
        n.createdAt = TimeUtils::fromUnix(sqlite3_column_int64(stmt, 4));
        n.updatedAt = TimeUtils::fromUnix(sqlite3_column_int64(stmt, 5));
        n.isFavorite = sqlite3_column_int(stmt, 6) != 0;
        n.folderName = QString::fromUtf8(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        n.tags = getTagsForNote(n.id);
        notes.append(n);
    }
    sqlite3_finalize(stmt);

    if (sort == NoteSortField::Title) {
        std::sort(notes.begin(), notes.end(), [descending](const Note& a, const Note& b) {
            return descending
                ? a.title.compare(b.title, Qt::CaseInsensitive) > 0
                : a.title.compare(b.title, Qt::CaseInsensitive) < 0;
        });
    }

    return notes;
}

std::optional<Note> NoteRepository::getNote(qint64 id, const QByteArray& key) const {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT n.encrypted_title, n.encrypted_body, n.folder_id, "
        "n.created_at, n.updated_at, n.is_favorite, COALESCE(f.name, '') "
        "FROM notes n LEFT JOIN folders f ON f.id = n.folder_id WHERE n.id = ?;";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, id);

    std::optional<Note> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Note n;
        n.id = id;
        const auto* tPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 0));
        n.title = decryptNoteField(
            QByteArray(tPtr, sqlite3_column_bytes(stmt, 0)), n.id, false, key);
        const auto* bPtr = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 1));
        n.body = decryptNoteField(
            QByteArray(bPtr, sqlite3_column_bytes(stmt, 1)), n.id, true, key);
        n.folderId = sqlite3_column_type(stmt, 2) == SQLITE_NULL
            ? 0
            : sqlite3_column_int64(stmt, 2);
        n.createdAt = TimeUtils::fromUnix(sqlite3_column_int64(stmt, 3));
        n.updatedAt = TimeUtils::fromUnix(sqlite3_column_int64(stmt, 4));
        n.isFavorite = sqlite3_column_int(stmt, 5) != 0;
        n.folderName = QString::fromUtf8(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        n.tags = getTagsForNote(id);
        result = n;
    }

    sqlite3_finalize(stmt);
    return result;
}

qint64 NoteRepository::createNote(const Note& note, const QByteArray& key) {
    const qint64 now = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());
    static const QByteArray kPlaceholder(1, '\0');

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO notes (encrypted_title, encrypted_body, folder_id, created_at, updated_at, is_favorite) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_blob(stmt, 1, kPlaceholder.constData(), kPlaceholder.size(), SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, kPlaceholder.constData(), kPlaceholder.size(), SQLITE_STATIC);
    bindFolderId(stmt, 3, note.folderId);
    sqlite3_bind_int64(stmt, 4, now);
    sqlite3_bind_int64(stmt, 5, now);
    sqlite3_bind_int(stmt, 6, note.isFavorite ? 1 : 0);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);

    const qint64 id = sqlite3_last_insert_rowid(m_db.handle());
    Note persisted = note;
    persisted.id = id;
    if (!updateNote(persisted, key)) {
        deleteNote(id);
        return 0;
    }

    setNoteTags(id, note.tags);
    return id;
}

bool NoteRepository::updateNote(const Note& note, const QByteArray& key) {
    const QByteArray encTitle = encryptNoteField(note.title, note.id, false, key);
    const QByteArray encBody = encryptNoteField(note.body, note.id, true, key);
    if (encTitle.isEmpty() && !note.title.isEmpty()) {
        return false;
    }

    const qint64 now = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "UPDATE notes SET encrypted_title=?, encrypted_body=?, folder_id=?, "
        "updated_at=?, is_favorite=? WHERE id=?;";

    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_blob(stmt, 1, encTitle.constData(), encTitle.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 2, encBody.constData(), encBody.size(), SQLITE_TRANSIENT);
    bindFolderId(stmt, 3, note.folderId);
    sqlite3_bind_int64(stmt, 4, now);
    sqlite3_bind_int(stmt, 5, note.isFavorite ? 1 : 0);
    sqlite3_bind_int64(stmt, 6, note.id);

    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (ok) {
        setNoteTags(note.id, note.tags);
    }
    return ok;
}

bool NoteRepository::deleteNote(qint64 id) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM notes WHERE id = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

qint64 NoteRepository::duplicateNote(qint64 id, const QByteArray& key) {
    const auto note = getNote(id, key);
    if (!note) return 0;

    Note copy = *note;
    copy.id = 0;
    copy.title += QStringLiteral(" (copy)");
    return createNote(copy, key);
}

QVector<Folder> NoteRepository::listFolders() const {
    QVector<Folder> folders;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, parent_id FROM folders ORDER BY name;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return folders;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Folder f;
        f.id = sqlite3_column_int64(stmt, 0);
        f.name = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        f.parentId = sqlite3_column_int64(stmt, 2);
        folders.append(f);
    }
    sqlite3_finalize(stmt);
    return folders;
}

qint64 NoteRepository::createFolder(const QString& name, qint64 parentId) {
    const qint64 now = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO folders (name, parent_id, created_at) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, parentId);
    sqlite3_bind_int64(stmt, 3, now);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok ? sqlite3_last_insert_rowid(m_db.handle()) : 0;
}

bool NoteRepository::renameFolder(qint64 id, const QString& name) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE folders SET name = ? WHERE id = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool NoteRepository::deleteFolder(qint64 id) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM folders WHERE id = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

QVector<Tag> NoteRepository::listTags() const {
    QVector<Tag> tags;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name FROM tags ORDER BY name;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return tags;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Tag t;
        t.id = sqlite3_column_int64(stmt, 0);
        t.name = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        tags.append(t);
    }
    sqlite3_finalize(stmt);
    return tags;
}

qint64 NoteRepository::ensureTag(const QString& name) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id FROM tags WHERE name = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const qint64 id = sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
        return id;
    }
    sqlite3_finalize(stmt);

    const qint64 now = TimeUtils::toUnix(QDateTime::currentDateTimeUtc());
    const char* ins = "INSERT INTO tags (name, created_at) VALUES (?, ?);";
    if (sqlite3_prepare_v2(m_db.handle(), ins, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, now);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok ? sqlite3_last_insert_rowid(m_db.handle()) : 0;
}

bool NoteRepository::setNoteTags(qint64 noteId, const QVector<QString>& tags) {
    sqlite3_stmt* del = nullptr;
    const char* delSql = "DELETE FROM note_tags WHERE note_id = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), delSql, -1, &del, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(del, 1, noteId);
    sqlite3_step(del);
    sqlite3_finalize(del);

    for (const QString& tag : tags) {
        const QString trimmed = tag.trimmed();
        if (trimmed.isEmpty()) continue;
        const qint64 tagId = ensureTag(trimmed);
        if (!tagId) continue;

        sqlite3_stmt* ins = nullptr;
        const char* insSql = "INSERT OR IGNORE INTO note_tags (note_id, tag_id) VALUES (?, ?);";
        if (sqlite3_prepare_v2(m_db.handle(), insSql, -1, &ins, nullptr) != SQLITE_OK) {
            continue;
        }
        sqlite3_bind_int64(ins, 1, noteId);
        sqlite3_bind_int64(ins, 2, tagId);
        sqlite3_step(ins);
        sqlite3_finalize(ins);
    }
    return true;
}

bool NoteRepository::importMarkdownFile(const QString& path, const QByteArray& key, qint64 folderId) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    if (f.size() > SecurityLimits::kMaxMarkdownImportBytes) {
        return false;
    }
    const QString content = QString::fromUtf8(f.readAll());
    f.close();

    Note n;
    n.title = QFileInfo(path).completeBaseName();
    n.body = content;
    n.folderId = folderId > 0 ? folderId : defaultFolderId();
    return createNote(n, key) > 0;
}

bool NoteRepository::exportMarkdownFile(qint64 noteId, const QString& path, const QByteArray& key) const {
    const auto note = getNote(noteId, key);
    if (!note) return false;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    f.write(note->body.toUtf8());
    f.close();
    return true;
}

bool NoteRepository::exportAllMarkdown(const QString& dirPath, const QByteArray& key) const {
    const auto notes = listNotes(key);
    for (const Note& n : notes) {
        const auto full = getNote(n.id, key);
        if (!full) continue;
        QString safeName = full->title;
        for (QChar& c : safeName) {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                c = '_';
            }
        }
        const QString path = dirPath + '/' + safeName + QStringLiteral(".md");
        exportMarkdownFile(full->id, path, key);
    }
    return true;
}

void NoteRepository::bindFolderId(sqlite3_stmt* stmt, int index, qint64 folderId) const {
    if (folderId > 0) {
        sqlite3_bind_int64(stmt, index, folderId);
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

QString NoteRepository::getAppMetadata(const QString& key) const {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT value FROM app_metadata WHERE key = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return QString();
    }
    sqlite3_bind_text(stmt, 1, key.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    QString value;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        value = QString::fromUtf8(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return value;
}

bool NoteRepository::setAppMetadata(const QString& key, const QString& value) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO app_metadata (key, value) VALUES (?, ?);";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, key.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

qint64 NoteRepository::ensureDefaultFolder() {
    static constexpr auto kDefaultName = "Inbox";

    // Legacy rows used folder_id=0 which violates the folders foreign key.
    m_db.execute(QStringLiteral("UPDATE notes SET folder_id = NULL WHERE folder_id = 0;"));

    const auto folders = listFolders();
    for (const Folder& folder : folders) {
        if (folder.name.compare(QString::fromUtf8(kDefaultName), Qt::CaseInsensitive) == 0) {
            const qint64 stored = defaultFolderId();
            if (stored <= 0) {
                setDefaultFolderId(folder.id);
            }
            return folder.id;
        }
    }

    const qint64 created = createFolder(QString::fromUtf8(kDefaultName));
    if (created > 0) {
        setDefaultFolderId(created);
    }
    return created;
}

qint64 NoteRepository::defaultFolderId() const {
    const QString stored = getAppMetadata(QStringLiteral("default_folder_id"));
    if (stored.isEmpty()) {
        return 0;
    }

    const qint64 id = stored.toLongLong();
    if (id <= 0) {
        return 0;
    }

    const auto folder = getFolder(id);
    return folder.has_value() ? id : 0;
}

bool NoteRepository::setDefaultFolderId(qint64 folderId) {
    if (folderId <= 0) {
        return false;
    }
    return setAppMetadata(
        QStringLiteral("default_folder_id"),
        QString::number(folderId));
}

std::optional<Folder> NoteRepository::getFolder(qint64 id) const {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, parent_id FROM folders WHERE id = ?;";
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, id);

    std::optional<Folder> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Folder f;
        f.id = sqlite3_column_int64(stmt, 0);
        f.name = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        f.parentId = sqlite3_column_int64(stmt, 2);
        result = f;
    }
    sqlite3_finalize(stmt);
    return result;
}
