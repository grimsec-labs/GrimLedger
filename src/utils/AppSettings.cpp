#include "utils/AppSettings.h"

#include <QSettings>

namespace {

constexpr auto kFailedAttemptsKey = "security/failedUnlockAttempts";

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
    QSettings settings;
    return settings.value(QLatin1String(kFailedAttemptsKey), 0).toInt();
}

void AppSettings::incrementFailedUnlockAttempts() {
    QSettings settings;
    const int next = settings.value(QLatin1String(kFailedAttemptsKey), 0).toInt() + 1;
    settings.setValue(QLatin1String(kFailedAttemptsKey), next);
    settings.sync();
}

void AppSettings::resetFailedUnlockAttempts() {
    QSettings settings;
    settings.setValue(QLatin1String(kFailedAttemptsKey), 0);
    settings.sync();
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

bool AppSettings::hibpCheckEnabled() {
    QSettings settings;
    return settings.value(QStringLiteral("security/hibpCheckEnabled"), false).toBool();
}

void AppSettings::setHibpCheckEnabled(bool enabled) {
    QSettings settings;
    settings.setValue(QStringLiteral("security/hibpCheckEnabled"), enabled);
}

QDateTime AppSettings::lastEncryptedBackupTime() {
    QSettings settings;
    return settings.value(QStringLiteral("vault/lastEncryptedBackupUtc")).toDateTime().toUTC();
}

void AppSettings::setLastEncryptedBackupTime(const QDateTime& when) {
    QSettings settings;
    settings.setValue(QStringLiteral("vault/lastEncryptedBackupUtc"), when.toUTC());
}

bool AppSettings::webClipperEnabled() {
    QSettings settings;
    return settings.value(QStringLiteral("security/webClipperEnabled"), false).toBool();
}

void AppSettings::setWebClipperEnabled(bool enabled) {
    QSettings settings;
    settings.setValue(QStringLiteral("security/webClipperEnabled"), enabled);
}

bool AppSettings::semanticSearchEnabled() {
    QSettings settings;
    return settings.value(QStringLiteral("search/semanticSearchEnabled"), false).toBool();
}

void AppSettings::setSemanticSearchEnabled(bool enabled) {
    QSettings settings;
    settings.setValue(QStringLiteral("search/semanticSearchEnabled"), enabled);
}

bool AppSettings::lineNumbersEnabled() {
    QSettings settings;
    return settings.value(QStringLiteral("appearance/lineNumbers"), false).toBool();
}

void AppSettings::setLineNumbersEnabled(bool enabled) {
    QSettings settings;
    settings.setValue(QStringLiteral("appearance/lineNumbers"), enabled);
}

bool AppSettings::wordWrapEnabled() {
    QSettings settings;
    return settings.value(QStringLiteral("appearance/wordWrap"), true).toBool();
}

void AppSettings::setWordWrapEnabled(bool enabled) {
    QSettings settings;
    settings.setValue(QStringLiteral("appearance/wordWrap"), enabled);
}

void AppSettings::resetToDefaults() {
    setLineNumbersEnabled(false);
    setWordWrapEnabled(true);
    setAutoLockEnabled(true);
    setAutoLockMinutes(15);
    setBrowserBridgeEnabled(false);
    setHibpCheckEnabled(false);
    setSelfDestructEnabled(false);
    setWebClipperEnabled(false);
    setSemanticSearchEnabled(false);
    sync();
}

void AppSettings::sync() {
    QSettings settings;
    settings.sync();
}
