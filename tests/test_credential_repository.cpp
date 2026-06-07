#include "storage/CredentialRepository.h"
#include "storage/Database.h"
#include "security/CryptoManager.h"
#include "models/FillTrustLevel.h"

#include <sodium.h>

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>

namespace {

bool check(bool condition) {
    return condition;
}

QByteArray testKey() {
    return CryptoManager::randomBytes(CryptoManager::kKeySize);
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

    const QString dbPath = tempDir.path() + QStringLiteral("/test.grim");
    Database db;
    if (!db.open(dbPath)) {
        return 1;
    }

    const QByteArray key = testKey();
    CredentialRepository repo(db);

    Credential cred;
    cred.label = QStringLiteral("Test Site");
    cred.username = QStringLiteral("alice");
    cred.password = QStringLiteral("secret-password-value");
    cred.url = QStringLiteral("https://example.com/login");
    cred.notes = QStringLiteral("note text");
    cred.fillTrustLevel = FillTrustLevel::AllowSubdomains;

    const qint64 id = repo.createCredential(cred, key);
    if (!check(id > 0)) {
        return 1;
    }

    const auto summaries = repo.listCredentialSummaries(key);
    if (!check(summaries.size() == 1)) {
        return 1;
    }
    if (!check(summaries[0].url == cred.url)) {
        return 1;
    }
    if (!check(!summaries[0].url.contains(QStringLiteral("secret-password")))) {
        return 1;
    }
    if (!check(summaries[0].fillTrustLevel == FillTrustLevel::AllowSubdomains)) {
        return 1;
    }

    const auto loaded = repo.getCredential(id, key);
    if (!check(loaded.has_value())) {
        return 1;
    }
    if (!check(loaded->password == cred.password)) {
        return 1;
    }
    if (!check(loaded->fillTrustLevel == FillTrustLevel::AllowSubdomains)) {
        return 1;
    }
    if (!check(!loaded->integrityError)) {
        return 1;
    }

    if (!check(repo.migrateFillPolicies(key))) {
        return 1;
    }

    return 0;
}
