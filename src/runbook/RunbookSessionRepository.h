#pragma once

#include "storage/Database.h"

#include <QByteArray>
#include <QString>
#include <QVector>
#include <optional>

struct RunbookSessionState {
    qint64 id = 0;
    qint64 noteId = 0;
    int currentStep = 0;
    QVector<bool> completed;
    QString notes;
};

class RunbookSessionRepository {
public:
    explicit RunbookSessionRepository(Database& db);

    qint64 createSession(qint64 noteId, int stepCount, const QByteArray& key);
    bool saveState(const RunbookSessionState& state, const QByteArray& key);
    std::optional<RunbookSessionState> loadSession(qint64 sessionId, const QByteArray& key) const;

private:
    Database& m_db;
};
