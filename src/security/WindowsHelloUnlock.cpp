#include "security/WindowsHelloUnlock.h"
#include "security/CryptoManager.h"

#include <QSettings>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincred.h>
#endif

namespace WindowsHelloUnlock {

namespace {

QString g_lastError;

#ifdef Q_OS_WIN
bool protectData(const QByteArray& plain, QByteArray& protectedOut) {
    DATA_BLOB input {};
    input.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plain.data()));
    input.cbData = static_cast<DWORD>(plain.size());

    DATA_BLOB output {};
    if (!CryptProtectData(&input, L"GrimLedger Hello Unlock", nullptr, nullptr, nullptr,
            CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        g_lastError = QStringLiteral("Windows data protection failed.");
        return false;
    }

    protectedOut = QByteArray(reinterpret_cast<char*>(output.pbData),
        static_cast<int>(output.cbData));
    LocalFree(output.pbData);
    return true;
}

bool unprotectData(const QByteArray& protectedBlob, QByteArray& plainOut) {
    DATA_BLOB input {};
    input.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(protectedBlob.data()));
    input.cbData = static_cast<DWORD>(protectedBlob.size());

    DATA_BLOB output {};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, 0, &output)) {
        g_lastError = QStringLiteral("Windows Hello unlock was denied or unavailable.");
        return false;
    }

    plainOut = QByteArray(reinterpret_cast<char*>(output.pbData),
        static_cast<int>(output.cbData));
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);
    return true;
}
#endif

} // namespace

bool isPlatformSupported() {
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

bool isConfigured() {
    QSettings settings;
    return settings.contains(QStringLiteral("security/helloProtectedSecret"))
        && settings.contains(QStringLiteral("security/helloWrappedKey"));
}

QString lastError() {
    return g_lastError;
}

bool enable(const QByteArray& vaultKey, const QString& masterPassword) {
    g_lastError.clear();
    if (!isPlatformSupported()) {
        g_lastError = QStringLiteral("Windows Hello unlock is only available on Windows.");
        return false;
    }
    if (vaultKey.size() != CryptoManager::kKeySize || masterPassword.isEmpty()) {
        g_lastError = QStringLiteral("Invalid unlock material.");
        return false;
    }

#ifdef Q_OS_WIN
    QByteArray helloSecret = CryptoManager::randomBytes(CryptoManager::kKeySize);
    QByteArray protectedSecret;
    if (!protectData(helloSecret, protectedSecret)) {
        CryptoManager::secureZero(helloSecret);
        return false;
    }

    const QByteArray aad = QByteArray("grim:hello:v1");
    const auto wrappedKey = CryptoManager::encryptAead(vaultKey, helloSecret, aad);
    CryptoManager::secureZero(helloSecret);
    if (!wrappedKey) {
        g_lastError = QStringLiteral("Could not wrap vault key.");
        return false;
    }

    QSettings settings;
    settings.setValue(QStringLiteral("security/helloProtectedSecret"), protectedSecret);
    settings.setValue(QStringLiteral("security/helloWrappedKey"), *wrappedKey);
    settings.setValue(QStringLiteral("security/helloEnabled"), true);
    return true;
#else
    Q_UNUSED(masterPassword);
    g_lastError = QStringLiteral("Windows Hello unlock is only available on Windows.");
    return false;
#endif
}

bool tryUnlock(QByteArray& vaultKeyOut) {
    g_lastError.clear();
    if (!isConfigured()) {
        g_lastError = QStringLiteral("Windows Hello unlock is not configured.");
        return false;
    }

#ifdef Q_OS_WIN
    QSettings settings;
    const QByteArray protectedSecret =
        settings.value(QStringLiteral("security/helloProtectedSecret")).toByteArray();
    const QByteArray wrappedKey =
        settings.value(QStringLiteral("security/helloWrappedKey")).toByteArray();

    QByteArray helloSecret;
    if (!unprotectData(protectedSecret, helloSecret)) {
        return false;
    }

    const QByteArray aad = QByteArray("grim:hello:v1");
    const auto key = CryptoManager::decryptAead(wrappedKey, helloSecret, aad);
    CryptoManager::secureZero(helloSecret);
    if (!key || key->size() != CryptoManager::kKeySize) {
        g_lastError = QStringLiteral("Stored Hello unlock material is invalid.");
        return false;
    }

    vaultKeyOut = *key;
    return true;
#else
    g_lastError = QStringLiteral("Windows Hello unlock is only available on Windows.");
    return false;
#endif
}

void disable() {
    QSettings settings;
    settings.remove(QStringLiteral("security/helloProtectedSecret"));
    settings.remove(QStringLiteral("security/helloWrappedKey"));
    settings.remove(QStringLiteral("security/helloEnabled"));
}

} // namespace WindowsHelloUnlock
