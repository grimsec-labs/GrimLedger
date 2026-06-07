#pragma once

#include "storage/Database.h"

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QVector>

struct ChronicleEntry {
    qint64 id = 0;
    QString eventType;
    QString summary;
    QByteArray prevHash;
    QByteArray entryHash;
    QDateTime createdAt;
};

class SecurityChronicle {
public:
    explicit SecurityChronicle(Database& db);

    bool appendEvent(const QString& eventType, const QString& summary, const QByteArray& key);
    QVector<ChronicleEntry> listEntries(const QByteArray& key, int limit = 200) const;
    bool verifyChain(const QByteArray& key) const;

private:
    Database& m_db;
};
