#include "utils/AppSettings.h"

#include <QSettings>

namespace {

int g_failedUnlockAttempts = 0;

} // namespace

bool AppSettings::selfDestructEnabled() {
    QSettings settings;
    return settings.value(QStringLiteral("security/selfDestructOnFailedAttempts"), false).toBool();
}

void AppSettings::setSelfDestructEnabled(bool enabled) {
    QSettings settings;
    settings.setValue(QStringLiteral("security/selfDestructOnFailedAttempts"), enabled);
    if (!enabled) {
        resetFailedUnlockAttempts();
    }
}

int AppSettings::failedUnlockAttempts() {
    return g_failedUnlockAttempts;
}

void AppSettings::incrementFailedUnlockAttempts() {
    ++g_failedUnlockAttempts;
}

void AppSettings::resetFailedUnlockAttempts() {
    g_failedUnlockAttempts = 0;
}
