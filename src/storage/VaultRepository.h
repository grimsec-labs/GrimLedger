#pragma once

#include "security/CryptoManager.h"
#include "storage/Database.h"

#include <QByteArray>
#include <QString>
#include <optional>

struct VaultInfo {
    int version = 1;
    QByteArray salt;
    CryptoManager::KdfParams kdfParams;
    qint64 createdAt = 0;
    qint64 updatedAt = 0;
};

enum class RestoreResult {
    Failed,
    Installed,
    InstalledWithCleanupWarning,
};

class VaultRepository {
public:
    explicit VaultRepository(Database& db);

    bool vaultExists() const;
    std::optional<VaultInfo> loadVaultInfo() const;

    bool createVault(const QString& masterPassword);
    bool unlockVault(const QString& masterPassword, QByteArray& derivedKeyOut);

    bool changeMasterPassword(
        const QByteArray& currentKey,
        const QString& newPassword,
        QByteArray& newKeyOut);

    bool migrateDomainBoundCrypto(const QByteArray& key);

    bool verifyMasterPassword(const QString& password) const;
    bool exportEncryptedBackupV2(const QString& destPath, const QString& password);
    bool exportEncryptedBackupLegacy(const QByteArray& key, const QString& destPath);

    static RestoreResult restoreFromBackup(
        const QString& backupPath,
        const QString& password,
        const QString& liveVaultPath,
        QString* errorOut = nullptr);

    static RestoreResult restoreLegacyBackupInSession(
        const QString& backupPath,
        const QByteArray& sessionKey,
        const QString& liveVaultPath,
        QString* errorOut = nullptr);

    static bool validateVaultFile(const QString& path, QString* errorOut = nullptr);

private:
    static RestoreResult installValidatedPlaintextVault(
        const QString& tempPath,
        const QString& liveVaultPath,
        QString* errorOut);
    bool storeVaultInfo(const VaultInfo& info);
    bool writeVerificationToken(const QByteArray& key);
    bool verificationTokenExists() const;
    bool cryptoFormatIsV2() const;
    bool setCryptoFormatV2();

    Database& m_db;
};
