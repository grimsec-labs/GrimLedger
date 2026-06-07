#pragma once

#include "models/Folder.h"
#include "models/Note.h"
#include "models/Tag.h"
#include "storage/Database.h"
#include "utils/RepositoryResult.h"

#include <QByteArray>
#include <QVector>
#include <optional>

struct sqlite3_stmt;

enum class NoteSortField {
    Title,
    Created,
    Modified,
    Folder,
    Favorite
};

class NoteRepository {
public:
    explicit NoteRepository(Database& db);

    QVector<Note> listNotes(
        const QByteArray& key,
        NoteSortField sort = NoteSortField::Modified,
        bool descending = true,
        qint64 folderId = -1,
        bool favoritesOnly = false,
        int recentLimit = -1) const;

    std::optional<Note> getNote(qint64 id, const QByteArray& key) const;
    qint64 createNote(const Note& note, const QByteArray& key);
    bool updateNote(const Note& note, const QByteArray& key);
    bool deleteNote(qint64 id);
    qint64 duplicateNote(qint64 id, const QByteArray& key);
    qint64 duplicateNoteWithAttachments(
        qint64 id,
        const QByteArray& key,
        class AttachmentRepository& attachments);

    QVector<Folder> listFolders() const;
    qint64 createFolder(const QString& name, qint64 parentId = 0);
    bool renameFolder(qint64 id, const QString& name);
    bool deleteFolder(qint64 id);

    QVector<Tag> listTags() const;
    qint64 ensureTag(const QString& name);
    bool setNoteTags(qint64 noteId, const QVector<QString>& tags);

    bool importMarkdownFile(const QString& path, const QByteArray& key, qint64 folderId = 0);
    bool exportMarkdownFile(qint64 noteId, const QString& path, const QByteArray& key) const;
    ExportResult exportAllMarkdown(const QString& dirPath, const QByteArray& key) const;

    qint64 ensureDefaultFolder();
    qint64 defaultFolderId() const;
    bool setDefaultFolderId(qint64 folderId);
    std::optional<Folder> getFolder(qint64 id) const;

private:
    QString getAppMetadata(const QString& key) const;
    bool setAppMetadata(const QString& key, const QString& value);
    void bindFolderId(sqlite3_stmt* stmt, int index, qint64 folderId) const;

    Database& m_db;
    QByteArray encryptNoteField(
        const QString& text,
        qint64 noteId,
        bool isBody,
        const QByteArray& key) const;
    DecryptResult<QString> decryptNoteField(
        const QByteArray& blob,
        qint64 noteId,
        bool isBody,
        const QByteArray& key) const;
    QVector<QString> getTagsForNote(qint64 noteId) const;
};
