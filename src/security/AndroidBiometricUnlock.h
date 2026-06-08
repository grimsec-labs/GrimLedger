#pragma once

#include <QByteArray>
#include <QString>

namespace AndroidBiometricUnlock {

bool isPlatformSupported();
bool isConfigured();
bool enable(const QByteArray& vaultKey, const QString& masterPassword);
bool tryUnlock(QByteArray& vaultKeyOut);
void disable();
QString lastError();

} // namespace AndroidBiometricUnlock
