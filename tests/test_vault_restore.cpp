#include "storage/VaultRepository.h"
#include "storage/Database.h"

#include <sodium.h>

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>

namespace {

bool check(bool condition) {
    return condition;
}

bool createVaultAt(const QString& path, const QString& password) {
    Database db;
    if (!db.open(path)) {
        return false;
    }
    VaultRepository vault(db);
    if (!vault.createVault(password)) {
        return false;
    }
    QByteArray key;
    return vault.unlockVault(password, key);
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

    const QString livePath = tempDir.path() + QStringLiteral("/vault.grim");
    const QString stagedPath = tempDir.path() + QStringLiteral("/vault.grim.new");
    const QString password = QStringLiteral("TestVaultPassword123!");

    if (!createVaultAt(stagedPath, password)) {
        return 1;
    }

    QString error;
    const RestoreResult result = VaultRepository::installStagedVault(stagedPath, livePath, &error);
    if (!check(result == RestoreResult::Installed)) {
        return 1;
    }
    if (!check(QFile::exists(livePath))) {
        return 1;
    }

    Database db;
    if (!check(db.open(livePath))) {
        return 1;
    }
    VaultRepository vault(db);
    QByteArray key;
    if (!check(vault.unlockVault(password, key))) {
        return 1;
    }

    return 0;
}
