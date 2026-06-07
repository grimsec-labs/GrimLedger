#pragma once

#include "storage/Database.h"

#include <QByteArray>
#include <QString>
#include <QVector>
#include <optional>

struct SealedBlock {
    QString id;
    qint64 noteId = 0;
    QString label;
    QString content;
};

class SealedBlockRepository {
public:
    explicit SealedBlockRepository(Database& db);

    QString processBodyForSave(qint64 noteId, QString body, const QByteArray& key);
    QString expandBodyForEdit(qint64 noteId, const QString& body, const QByteArray& key) const;
    QString strippedBodyForSearch(qint64 noteId, const QString& body) const;
    QString strippedBodyForExport(qint64 noteId, const QString& body) const;
    std::optional<QString> revealBlock(
        qint64 noteId,
        const QString& blockId,
        const QByteArray& key) const;
    bool deleteBlocksForNote(qint64 noteId);

private:
    bool upsertBlock(
        qint64 noteId,
        const QString& blockId,
        const QString& label,
        const QString& content,
        const QByteArray& key);
    Database& m_db;
};
