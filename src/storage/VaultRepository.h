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

class VaultRepository {
public:
    explicit VaultRepository(Database& db);

    bool vaultExists() const;
    std::optional<VaultInfo> loadVaultInfo() const;

    bool createVault(const QString& masterPassword);
    bool unlockVault(const QString& masterPassword, QByteArray& derivedKeyOut);

    bool changeMasterPassword(
        const QByteArray& currentKey,
        const QString& newPassword);

    bool exportEncryptedBackup(const QByteArray& key, const QString& destPath);
    bool importEncryptedBackup(const QString& srcPath, const QString& newMasterPassword);

private:
    bool storeVaultInfo(const VaultInfo& info);

    Database& m_db;
};
