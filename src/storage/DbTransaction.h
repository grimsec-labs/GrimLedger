#pragma once

class Database;

class DbTransaction {
public:
    explicit DbTransaction(Database& db, bool immediate = true);
    ~DbTransaction();

    DbTransaction(const DbTransaction&) = delete;
    DbTransaction& operator=(const DbTransaction&) = delete;

    bool commit();

private:
    Database& m_db;
    bool m_active = false;
    bool m_committed = false;
};
