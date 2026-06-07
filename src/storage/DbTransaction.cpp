#include "storage/DbTransaction.h"
#include "storage/Database.h"

#include <sqlite3.h>

DbTransaction::DbTransaction(Database& db, bool immediate)
    : m_db(db) {
    if (!m_db.isOpen()) {
        return;
    }
    const char* sql = immediate ? "BEGIN IMMEDIATE;" : "BEGIN;";
    m_active = m_db.execute(QString::fromUtf8(sql));
}

DbTransaction::~DbTransaction() {
    if (m_active && !m_committed) {
        m_db.execute(QStringLiteral("ROLLBACK;"));
    }
}

bool DbTransaction::commit() {
    if (!m_active || m_committed) {
        return false;
    }
    if (!m_db.execute(QStringLiteral("COMMIT;"))) {
        return false;
    }
    m_committed = true;
    return true;
}
