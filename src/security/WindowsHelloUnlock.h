#pragma once

#include <QByteArray>
#include <QString>

// NOTE (GL-SEC-001): Despite the historical "Hello" name, this module does NOT use
// the Windows Hello / KeyCredentialManager biometric API. It wraps the vault key with
// a secret protected by user-scoped DPAPI (CryptProtectData/CryptUnprotectData). This
// is account-bound convenience unlock, NOT a biometric gesture gate: any code running
// as the signed-in Windows user can recover the key. User-facing strings call this
// "Quick Unlock (Windows account)". A real biometric gate would require the WinRT
// KeyCredentialManager / Passport-NGC APIs (recommended follow-up).
namespace WindowsHelloUnlock {

bool isPlatformSupported();
bool isConfigured();
bool enable(const QByteArray& vaultKey, const QString& masterPassword);
bool tryUnlock(QByteArray& vaultKeyOut);
void disable();
QString lastError();

} // namespace WindowsHelloUnlock
