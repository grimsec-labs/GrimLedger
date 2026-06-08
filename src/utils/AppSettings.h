#pragma once

#include <QDateTime>

class AppSettings {
public:
    static constexpr int kMaxFailedUnlockAttempts = 3;

    static bool selfDestructEnabled();
    static void setSelfDestructEnabled(bool enabled);

    static int failedUnlockAttempts();
    static void incrementFailedUnlockAttempts();
    static void resetFailedUnlockAttempts();

    static bool browserBridgeEnabled();
    static void setBrowserBridgeEnabled(bool enabled);

    static bool autoLockEnabled();
    static void setAutoLockEnabled(bool enabled);
    static int autoLockMinutes();
    static void setAutoLockMinutes(int minutes);

    static bool hibpCheckEnabled();
    static void setHibpCheckEnabled(bool enabled);

    static QDateTime lastEncryptedBackupTime();
    static void setLastEncryptedBackupTime(const QDateTime& when);

    static bool webClipperEnabled();
    static void setWebClipperEnabled(bool enabled);

    static bool semanticSearchEnabled();
    static void setSemanticSearchEnabled(bool enabled);

    static bool lineNumbersEnabled();
    static void setLineNumbersEnabled(bool enabled);
    static bool wordWrapEnabled();
    static void setWordWrapEnabled(bool enabled);

    static void resetToDefaults();
    static void sync();
};
