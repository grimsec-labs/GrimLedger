#include "security/SecretScanner.h"
#include "share/GrimShare.h"
#include "runbook/RunbookParser.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QString>

namespace {

bool check(bool condition) {
    return condition;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    const QVector<SecretFinding> findings = SecretScanner::scanText(
        QStringLiteral("api_key=AKIAIOSFODNN7EXAMPLE"));
    if (!check(!findings.isEmpty())) {
        return 1;
    }

    const QVector<RunbookStep> steps = RunbookParser::parseSteps(
        QStringLiteral("- [ ] Step one\n- [ ] Step two\n"));
    if (!check(steps.size() == 2)) {
        return 1;
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return 1;
    }

    GrimShareNotePayload note;
    note.title = QStringLiteral("Shared");
    note.body = QStringLiteral("Body text");
    const QString path = tempDir.path() + QStringLiteral("/test.grimshare");
    const GrimSharePackResult packed = GrimShare::packNotes(path, {note}, QStringLiteral("test-passphrase-123"));
    if (!check(packed.ok)) {
        return 1;
    }
    const GrimShareUnpackResult unpacked = GrimShare::unpackFile(path, QStringLiteral("test-passphrase-123"));
    if (!check(unpacked.ok && unpacked.notes.size() == 1 && unpacked.notes[0].title == note.title)) {
        return 1;
    }

    return 0;
}
