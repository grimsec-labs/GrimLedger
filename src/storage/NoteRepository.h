#pragma once

#include "models/Folder.h"
#include "models/Note.h"
#include "models/Tag.h"
#include "storage/Database.h"

#include <QByteArray>
#include <QVector>
#include <optional>

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

    QVector<Folder> listFolders() const;
    qint64 createFolder(const QString& name, qint64 parentId = 0);
    bool renameFolder(qint64 id, const QString& name);
    bool deleteFolder(qint64 id);

    QVector<Tag> listTags() const;
    qint64 ensureTag(const QString& name);
    bool setNoteTags(qint64 noteId, const QVector<QString>& tags);

    bool importMarkdownFile(const QString& path, const QByteArray& key, qint64 folderId = 0);
    bool exportMarkdownFile(qint64 noteId, const QString& path, const QByteArray& key) const;
    bool exportAllMarkdown(const QString& dirPath, const QByteArray& key) const;

private:
    QByteArray encryptField(const QString& text, const QByteArray& key) const;
    QString decryptField(const QByteArray& blob, const QByteArray& key) const;
    QVector<QString> getTagsForNote(qint64 noteId) const;

    Database& m_db;
};
