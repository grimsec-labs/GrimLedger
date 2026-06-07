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

bool AppSettings::browserBridgeEnabled() {
    QSettings settings;
    return settings.value(QStringLiteral("security/browserBridgeEnabled"), false).toBool();
}

void AppSettings::setBrowserBridgeEnabled(bool enabled) {
    QSettings settings;
    settings.setValue(QStringLiteral("security/browserBridgeEnabled"), enabled);
}

bool AppSettings::autoLockEnabled() {
    QSettings settings;
    return settings.value(QStringLiteral("security/autoLockEnabled"), true).toBool();
}

void AppSettings::setAutoLockEnabled(bool enabled) {
    QSettings settings;
    settings.setValue(QStringLiteral("security/autoLockEnabled"), enabled);
}

int AppSettings::autoLockMinutes() {
    QSettings settings;
    return settings.value(QStringLiteral("security/autoLockMinutes"), 15).toInt();
}

void AppSettings::setAutoLockMinutes(int minutes) {
    QSettings settings;
    settings.setValue(QStringLiteral("security/autoLockMinutes"), minutes);
}
