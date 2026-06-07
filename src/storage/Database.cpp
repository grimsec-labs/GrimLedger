#include "storage/Database.h"

#include <QDir>
#include <QStandardPaths>
#include <sqlite3.h>

Database::Database() = default;

Database::~Database() {
    close();
}

QString Database::defaultVaultPath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/vault.grim");
}

bool Database::open(const QString& path) {
    close();
    m_path = path;

    if (sqlite3_open(path.toUtf8().constData(), &m_db) != SQLITE_OK) {
        m_db = nullptr;
        return false;
    }

    if (!execute(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
        close();
        return false;
    }
    if (!execute(QStringLiteral("PRAGMA secure_delete = ON;"))) {
        close();
        return false;
    }
    if (!execute(QStringLiteral("PRAGMA trusted_schema = OFF;"))) {
        close();
        return false;
    }
    return initializeSchema();
}

void Database::close() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool Database::execute(const QString& sql) {
    if (!m_db) return false;
    char* err = nullptr;
    const int rc = sqlite3_exec(m_db, sql.toUtf8().constData(), nullptr, nullptr, &err);
    if (err) {
        sqlite3_free(err);
    }
    return rc == SQLITE_OK;
}

bool Database::initializeSchema() {
    const char* schema = R"SQL(
        CREATE TABLE IF NOT EXISTS vault_metadata (
            id INTEGER PRIMARY KEY CHECK (id = 1),
            vault_version INTEGER NOT NULL,
            salt BLOB NOT NULL,
            kdf_ops_limit INTEGER NOT NULL,
            kdf_mem_limit INTEGER NOT NULL,
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS app_metadata (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS folders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            parent_id INTEGER DEFAULT 0,
            created_at INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            created_at INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS notes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            encrypted_title BLOB NOT NULL,
            encrypted_body BLOB NOT NULL,
            folder_id INTEGER DEFAULT 0,
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL,
            is_favorite INTEGER NOT NULL DEFAULT 0,
            FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE SET NULL
        );

        CREATE TABLE IF NOT EXISTS note_tags (
            note_id INTEGER NOT NULL,
            tag_id INTEGER NOT NULL,
            PRIMARY KEY (note_id, tag_id),
            FOREIGN KEY (note_id) REFERENCES notes(id) ON DELETE CASCADE,
            FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS note_attachments (
            id TEXT PRIMARY KEY,
            note_id INTEGER NOT NULL,
            encrypted_data BLOB NOT NULL,
            mime_type TEXT NOT NULL,
            original_name TEXT,
            created_at INTEGER NOT NULL,
            FOREIGN KEY (note_id) REFERENCES notes(id) ON DELETE CASCADE
        );

        CREATE INDEX IF NOT EXISTS idx_notes_updated ON notes(updated_at DESC);
        CREATE INDEX IF NOT EXISTS idx_notes_favorite ON notes(is_favorite);
        CREATE INDEX IF NOT EXISTS idx_tags_name ON tags(name);
        CREATE INDEX IF NOT EXISTS idx_attachments_note ON note_attachments(note_id);

        CREATE TABLE IF NOT EXISTS credentials (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            encrypted_label BLOB NOT NULL,
            encrypted_username BLOB NOT NULL,
            encrypted_password BLOB NOT NULL,
            encrypted_url BLOB NOT NULL,
            encrypted_notes BLOB NOT NULL,
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_credentials_updated ON credentials(updated_at DESC);
    )SQL";

    if (!execute(QString::fromUtf8(schema))) {
        return false;
    }

    sqlite3_stmt* pragmaStmt = nullptr;
    bool hasAllowSubdomains = false;
    if (sqlite3_prepare_v2(
            m_db,
            "PRAGMA table_info(credentials);",
            -1,
            &pragmaStmt,
            nullptr) == SQLITE_OK) {
        while (sqlite3_step(pragmaStmt) == SQLITE_ROW) {
            const char* name = reinterpret_cast<const char*>(sqlite3_column_text(pragmaStmt, 1));
            if (name && qstrcmp(name, "allow_subdomains") == 0) {
                hasAllowSubdomains = true;
                break;
            }
        }
        sqlite3_finalize(pragmaStmt);
    }

    if (!hasAllowSubdomains) {
        return execute(QStringLiteral(
            "ALTER TABLE credentials ADD COLUMN allow_subdomains INTEGER NOT NULL DEFAULT 0;"));
    }

    return true;
}
