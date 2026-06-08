#include "security/PlatformBiometricUnlock.h"

#if defined(Q_OS_ANDROID)
#include "security/AndroidBiometricUnlock.h"
#elif defined(Q_OS_WIN)
#include "security/WindowsHelloUnlock.h"
#endif

namespace PlatformBiometricUnlock {

bool isPlatformSupported() {
#if defined(Q_OS_ANDROID)
    return AndroidBiometricUnlock::isPlatformSupported();
#elif defined(Q_OS_WIN)
    return WindowsHelloUnlock::isPlatformSupported();
#else
    return false;
#endif
}

bool isConfigured() {
#if defined(Q_OS_ANDROID)
    return AndroidBiometricUnlock::isConfigured();
#elif defined(Q_OS_WIN)
    return WindowsHelloUnlock::isConfigured();
#else
    return false;
#endif
}

bool enable(const QByteArray& vaultKey, const QString& masterPassword) {
#if defined(Q_OS_ANDROID)
    return AndroidBiometricUnlock::enable(vaultKey, masterPassword);
#elif defined(Q_OS_WIN)
    return WindowsHelloUnlock::enable(vaultKey, masterPassword);
#else
    Q_UNUSED(vaultKey);
    Q_UNUSED(masterPassword);
    return false;
#endif
}

bool tryUnlock(QByteArray& vaultKeyOut) {
#if defined(Q_OS_ANDROID)
    return AndroidBiometricUnlock::tryUnlock(vaultKeyOut);
#elif defined(Q_OS_WIN)
    return WindowsHelloUnlock::tryUnlock(vaultKeyOut);
#else
    Q_UNUSED(vaultKeyOut);
    return false;
#endif
}

void disable() {
#if defined(Q_OS_ANDROID)
    AndroidBiometricUnlock::disable();
#elif defined(Q_OS_WIN)
    WindowsHelloUnlock::disable();
#endif
}

QString lastError() {
#if defined(Q_OS_ANDROID)
    return AndroidBiometricUnlock::lastError();
#elif defined(Q_OS_WIN)
    return WindowsHelloUnlock::lastError();
#else
    return QString();
#endif
}

} // namespace PlatformBiometricUnlock
