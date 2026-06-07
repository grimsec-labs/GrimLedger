#pragma once

#include <QString>
#include <memory>

struct sqlite3;

// Thin SQLite wrapper. Uses prepared statements only.
// Does not handle encryption — repositories encrypt fields before storage.
class Database {
public:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool open(const QString& path);
    void close();
    bool isOpen() const { return m_db != nullptr; }

    QString path() const { return m_path; }

    bool execute(const QString& sql);
    sqlite3* handle() const { return m_db; }

    static QString defaultVaultPath();

private:
    bool initializeSchema();

    sqlite3* m_db = nullptr;
    QString m_path;
};
