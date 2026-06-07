#pragma once

struct sqlite3;
struct sqlite3_stmt;

namespace SqliteUtils {

bool stepDone(sqlite3_stmt* stmt);
bool expectChanges(sqlite3* db, int expected);

} // namespace SqliteUtils
