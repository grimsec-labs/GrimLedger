#include "utils/SqliteUtils.h"

#include <sqlite3.h>

int main() {
    sqlite3* db = nullptr;
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
        return 1;
    }
    if (sqlite3_exec(db, "CREATE TABLE t(id INTEGER PRIMARY KEY, v TEXT);", nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    if (sqlite3_exec(db, "INSERT INTO t(v) VALUES('a');", nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "UPDATE t SET v = ? WHERE id = 1;", -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_text(stmt, 1, "b", -1, SQLITE_TRANSIENT);
    if (!SqliteUtils::stepDone(stmt) || !SqliteUtils::expectChanges(db, 1)) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}
