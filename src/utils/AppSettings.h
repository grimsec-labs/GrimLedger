#pragma once

class AppSettings {
public:
    static constexpr int kMaxFailedUnlockAttempts = 3;

    static bool selfDestructEnabled();
    static void setSelfDestructEnabled(bool enabled);

    static int failedUnlockAttempts();
    static void incrementFailedUnlockAttempts();
    static void resetFailedUnlockAttempts();
};
