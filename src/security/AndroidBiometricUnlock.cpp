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

bool isConfigured() {
    QSettings settings;
    return settings.contains(QStringLiteral("security/androidBiometricWrappedKey"));
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
    env->SetByteArrayRegion(keyArray, 0, vaultKey.size(),
        reinterpret_cast<const jbyte*>(vaultKey.constData()));

    const QJniObject wrapped = QJniObject::callStaticObjectMethod(
        "org/grimseclabs/grimledger/GrimLedgerBiometricUnlock",
        "wrapVaultKey",
        "(Landroid/content/Context;[B)Ljava/lang/String;",
        androidContext(),
        keyArray);

    env->DeleteLocalRef(keyArray);

    if (!wrapped.isValid()) {
        g_lastError = QStringLiteral("Failed to store biometric unlock data.");
        return false;
    }

    QSettings settings;
    settings.setValue(QStringLiteral("security/androidBiometricWrappedKey"), wrapped.toString());
    return true;
}

bool tryUnlock(QByteArray& vaultKeyOut) {
    g_lastError.clear();
    QSettings settings;
    const QString wrapped = settings.value(QStringLiteral("security/androidBiometricWrappedKey")).toString();
    if (wrapped.isEmpty()) {
        g_lastError = QStringLiteral("Biometric unlock is not configured.");
        return false;
    }

    const QJniObject bytes = QJniObject::callStaticObjectMethod(
        "org/grimseclabs/grimledger/GrimLedgerBiometricUnlock",
        "unwrapVaultKey",
        "(Landroid/content/Context;Ljava/lang/String;)[B",
        androidContext(),
        QJniObject::fromString(wrapped).object<jstring>());

    if (!bytes.isValid()) {
        g_lastError = QStringLiteral("Biometric unlock was denied or unavailable.");
        return false;
    }

    QJniEnvironment env;
    const jbyteArray array = bytes.object<jbyteArray>();
    const jsize len = env->GetArrayLength(array);
    jbyte* data = env->GetByteArrayElements(array, nullptr);
    vaultKeyOut = QByteArray(reinterpret_cast<char*>(data), len);
    env->ReleaseByteArrayElements(array, data, JNI_ABORT);
    return vaultKeyOut.size() == CryptoManager::kKeySize;
}

void disable() {
    QSettings settings;
    settings.remove(QStringLiteral("security/androidBiometricWrappedKey"));
    QJniObject::callStaticMethod<void>(
        "org/grimseclabs/grimledger/GrimLedgerBiometricUnlock",
        "clear",
        "(Landroid/content/Context;)V",
        androidContext());
}

} // namespace AndroidBiometricUnlock
#endif
