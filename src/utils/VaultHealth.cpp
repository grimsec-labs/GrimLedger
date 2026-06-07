#include "utils/VaultHealth.h"
#include "utils/AppSettings.h"

#include <QFile>
#include <QFileInfo>

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
    bool vaultLocked) {
    VaultHealthReport report;
    report.vaultPath = vaultPath;
    report.credentialCount = credentialCount;
    report.credentialIntegrityErrors = credentialIntegrityErrors;
    report.noteCount = noteCount;
    report.noteIntegrityErrors = noteIntegrityErrors;
    report.attachmentBytes = attachmentBytes;
    report.bridgeEnabled = bridgeEnabled;
    report.bridgeListening = bridgeListening;
    report.vaultLocked = vaultLocked;

    const QFileInfo vaultInfo(vaultPath);
    if (vaultInfo.exists()) {
        report.vaultFileBytes = vaultInfo.size();
    }

    const QFileInfo localBackup(vaultPath + QStringLiteral(".bak"));
    if (localBackup.exists()) {
        report.lastLocalBackupTime = localBackup.lastModified().toUTC();
    }

    report.lastEncryptedBackupTime = AppSettings::lastEncryptedBackupTime();
    return report;
}

} // namespace VaultHealth
