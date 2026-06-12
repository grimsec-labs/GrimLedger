// Tests for SearchEngine body search functionality.

#include "search/SearchEngine.h"
#include "models/Note.h"

#include <QCoreApplication>
#include <QHash>
#include <QString>
#include <QVector>

namespace {

int fail(const char* what) {
    qWarning("test_body_search FAILED: %s", what);
    return 1;
}

Note makeNote(qint64 id, const QString& title) {
    Note n;
    n.id = id;
    n.title = title;
    return n;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    QVector<Note> notes;
    notes.append(makeNote(1, "Shopping List"));
    notes.append(makeNote(2, "Meeting Notes"));
    notes.append(makeNote(3, "Secret Plans"));

    QHash<qint64, QString> bodies;
    bodies.insert(1, "Buy milk, eggs, and bread from the store");
    bodies.insert(2, "Discuss Q4 budget and hiring timeline");
    bodies.insert(3, "Title only relevant here");

    // --- 1. Title-only search matches title but not body ---
    {
        auto matches = SearchEngine::search(notes, "Shopping", false);
        if (matches.size() != 1) return fail("title-only: expected 1 match for 'Shopping'");
        if (matches[0].note.id != 1) return fail("title-only: wrong note matched");
    }

    // --- 2. Title-only search does NOT match body text ---
    {
        auto matches = SearchEngine::search(notes, "milk", false);
        if (!matches.isEmpty()) return fail("title-only: should not match body text 'milk'");
    }

    // --- 3. Body search matches body text ---
    {
        auto matches = SearchEngine::search(notes, "milk", true, bodies);
        if (matches.size() != 1) return fail("body search: expected 1 match for 'milk'");
        if (matches[0].note.id != 1) return fail("body search: wrong note for 'milk'");
        if (matches[0].bodyMatchPositions.isEmpty())
            return fail("body search: bodyMatchPositions should be non-empty");
    }

    // --- 4. Body search also matches titles ---
    {
        auto matches = SearchEngine::search(notes, "Meeting", true, bodies);
        if (matches.size() != 1) return fail("body+title: expected 1 match for 'Meeting'");
        if (matches[0].note.id != 2) return fail("body+title: wrong note for 'Meeting'");
    }

    // --- 5. Body search with no matches ---
    {
        auto matches = SearchEngine::search(notes, "xyznonexistent", true, bodies);
        if (!matches.isEmpty()) return fail("body search: should return 0 for nonexistent term");
    }

    // --- 6. Empty query returns all notes (show-all behavior) ---
    {
        auto matches = SearchEngine::search(notes, "", true, bodies);
        if (matches.size() != notes.size())
            return fail("body search: empty query should return all notes");
    }

    return 0;
}
