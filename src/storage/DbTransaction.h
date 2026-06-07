#pragma once

class Database;

class DbTransaction {
public:
    explicit DbTransaction(Database& db, bool immediate = true);
    ~DbTransaction();

    DbTransaction(const DbTransaction&) = delete;
    DbTransaction& operator=(const DbTransaction&) = delete;

    bool commit();
    bool isActive() const { return m_active; }

private:
    Database& m_db;
    bool m_active = false;
    bool m_committed = false;
};
