#include "security/AndroidBiometricUnlock.h"
#include "security/CryptoManager.h"

#include <QSettings>

#ifndef Q_OS_ANDROID

namespace AndroidBiometricUnlock {

bool isPlatformSupported() { return false; }
bool isConfigured() { return false; }
bool enable(const QByteArray&, const QString&) { return false; }
bool tryUnlock(QByteArray&) { return false; }
void disable() {}
QString lastError() { return QStringLiteral("Android biometric unlock is only available on Android."); }

} // namespace AndroidBiometricUnlock

#else

#include <QJniObject>
#include <QJniEnvironment>

namespace AndroidBiometricUnlock {

namespace {

QString g_lastError;

jobject androidContext() {
    return QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative",
        "activity",
        "()Landroid/app/Activity;").object();
}

} // namespace

bool isPlatformSupported() {
    return QJniObject::callStaticMethod<jboolean>(
        "org/grimseclabs/grimledger/GrimLedgerBiometricUnlock",
        "isSupported",
        "(Landroid/content/Context;)Z",
        androidContext());
}

static const QString kEnabledKey = QStringLiteral("security/androidBiometricEnabled");

bool isConfigured() {
    QSettings settings;
    return settings.value(kEnabledKey, false).toBool();
}

QString lastError() {
    return g_lastError;
}

bool enable(const QByteArray& vaultKey, const QString& masterPassword) {
    Q_UNUSED(masterPassword);
    g_lastError.clear();
    if (!isPlatformSupported()) {
        g_lastError = QStringLiteral("Biometric unlock is not available on this device.");
        return false;
    }

    QJniEnvironment env;
    const jbyteArray keyArray = env->NewByteArray(vaultKey.size());
    if (!keyArray) {
        g_lastError = QStringLiteral("Could not allocate key buffer.");
        return false;
    }
    env->SetByteArrayRegion(keyArray, 0, vaultKey.size(),
        reinterpret_cast<const jbyte*>(vaultKey.constData()));

    const jboolean stored = QJniObject::callStaticMethod<jboolean>(
        "org/grimseclabs/grimledger/GrimLedgerBiometricUnlock",
        "storeVaultKey",
        "(Landroid/content/Context;[B)Z",
        androidContext(),
        keyArray);

    env->SetByteArrayRegion(keyArray, 0, vaultKey.size(),
        reinterpret_cast<const jbyte*>(QByteArray(vaultKey.size(), '\0').constData()));
    env->DeleteLocalRef(keyArray);

    if (!stored) {
        g_lastError = QStringLiteral("Failed to store biometric unlock data.");
        return false;
    }

    QSettings settings;
    settings.setValue(kEnabledKey, true);
    return true;
}

bool tryUnlock(QByteArray& vaultKeyOut) {
    g_lastError.clear();
    if (!isConfigured()) {
        g_lastError = QStringLiteral("Biometric unlock is not configured.");
        return false;
    }

    const QJniObject bytes = QJniObject::callStaticObjectMethod(
        "org/grimseclabs/grimledger/GrimLedgerBiometricUnlock",
        "loadVaultKey",
        "(Landroid/content/Context;)[B",
        androidContext());

    if (!bytes.isValid()) {
        g_lastError = QStringLiteral("Biometric unlock was denied or unavailable.");
        return false;
    }

    QJniEnvironment env;
    const jbyteArray array = bytes.object<jbyteArray>();
    if (!array) {
        g_lastError = QStringLiteral("Biometric unlock returned no key.");
        return false;
    }
    const jsize len = env->GetArrayLength(array);
    jbyte* data = env->GetByteArrayElements(array, nullptr);
    if (!data) {
        g_lastError = QStringLiteral("Biometric unlock returned no key.");
        return false;
    }
    vaultKeyOut = QByteArray(reinterpret_cast<char*>(data), len);
    env->ReleaseByteArrayElements(array, data, JNI_ABORT);
    return vaultKeyOut.size() == CryptoManager::kKeySize;
}

void disable() {
    QSettings settings;
    settings.remove(kEnabledKey);
    QJniObject::callStaticMethod<void>(
        "org/grimseclabs/grimledger/GrimLedgerBiometricUnlock",
        "clear",
        "(Landroid/content/Context;)V",
        androidContext());
}

} // namespace AndroidBiometricUnlock
#endif
