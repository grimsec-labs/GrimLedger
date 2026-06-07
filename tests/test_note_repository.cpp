#include "storage/NoteRepository.h"
#include "storage/Database.h"
#include "security/CryptoManager.h"

#include <sodium.h>

#include <QCoreApplication>
#include <QTemporaryDir>

namespace {

bool check(bool condition) {
    return condition;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    if (sodium_init() < 0) {
        return 1;
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return 1;
    }

    Database db;
    if (!db.open(tempDir.path() + QStringLiteral("/test.grim"))) {
        return 1;
    }

    const QByteArray key = CryptoManager::randomBytes(CryptoManager::kKeySize);
    NoteRepository notes(db);
    notes.ensureDefaultFolder();

    Note note;
    note.title = QStringLiteral("Hello");
    note.body = QStringLiteral("World body");
    note.folderId = notes.defaultFolderId();

    const qint64 id = notes.createNote(note, key);
    if (!check(id > 0)) {
        return 1;
    }

    const auto loaded = notes.getNote(id, key);
    if (!check(loaded.has_value())) {
        return 1;
    }
    if (!check(loaded->title == note.title)) {
        return 1;
    }
    if (!check(loaded->body == note.body)) {
        return 1;
    }
    if (!check(!loaded->integrityError)) {
        return 1;
    }

    Note updated = *loaded;
    updated.body = QStringLiteral("Updated body");
    if (!check(notes.updateNote(updated, key))) {
        return 1;
    }

    const auto reloaded = notes.getNote(id, key);
    if (!check(reloaded.has_value() && reloaded->body == updated.body)) {
        return 1;
    }

    return 0;
}
