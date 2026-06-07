#pragma once

#include <QString>
#include <QDateTime>

struct VaultHealthReport {
    qint64 vaultFileBytes = 0;
    qint64 attachmentBytes = 0;
    int credentialCount = 0;
    int noteCount = 0;
    int credentialIntegrityErrors = 0;
    int noteIntegrityErrors = 0;
    bool bridgeEnabled = false;
    bool bridgeListening = false;
    bool vaultLocked = true;
    QDateTime lastLocalBackupTime;
    QDateTime lastEncryptedBackupTime;
    QString vaultPath;
};

namespace VaultHealth {

VaultHealthReport collect(
    const QString& vaultPath,
    int credentialCount,
    int credentialIntegrityErrors,
    int noteCount,
    int noteIntegrityErrors,
    qint64 attachmentBytes,
    bool bridgeEnabled,
    bool bridgeListening,
    bool vaultLocked);

} // namespace VaultHealth
