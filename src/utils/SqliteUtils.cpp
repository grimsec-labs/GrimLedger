#include "utils/SqliteUtils.h"

#include <sqlite3.h>

namespace SqliteUtils {

bool stepDone(sqlite3_stmt* stmt) {
    if (!stmt) {
        return false;
    }
    const int rc = sqlite3_step(stmt);
    return rc == SQLITE_DONE;
}

bool expectChanges(sqlite3* db, int expected) {
    if (!db) {
        return false;
    }
    return sqlite3_changes(db) == expected;
}

} // namespace SqliteUtils
