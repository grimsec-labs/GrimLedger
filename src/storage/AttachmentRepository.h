#pragma once

#include "storage/Database.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariantMap>
#include <optional>

class AttachmentRepository {
public:
    static constexpr const char* kGrimScheme = "grim://attachment/";

    explicit AttachmentRepository(Database& db);

    QString storeAttachment(
        qint64 noteId,
        const QByteArray& imageData,
        const QString& mimeType,
        const QString& originalName,
        const QByteArray& key);

    std::optional<QByteArray> loadAttachment(
        const QString& attachmentId,
        qint64 noteId,
        const QByteArray& key) const;

    QList<QVariantMap> listAttachmentsForNote(qint64 noteId) const;

    QList<QVariantMap> listAllImageAttachments() const;

    bool deleteAttachment(const QString& attachmentId, qint64 noteId);

    QHash<QString, QString> duplicateAttachments(
        qint64 fromNoteId,
        qint64 toNoteId,
        const QByteArray& key) const;

    QString remapAttachmentUrls(
        const QString& body,
        const QHash<QString, QString>& idMap) const;

    QString rewriteBodyForExport(
        const QString& body,
        qint64 noteId,
        const QString& exportDir,
        const QByteArray& key) const;

    static bool isGrimAttachmentUrl(const QString& url);
    static QString attachmentIdFromUrl(const QString& url);
    static QList<QPair<QString, QString>> extractImageRefs(const QString& markdown);

private:
    Database& m_db;
};
