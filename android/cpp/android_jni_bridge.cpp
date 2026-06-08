#include "GrimVaultController.h"

#include <QJniEnvironment>
#include <QJniObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

static GrimVaultController* g_vaultController = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_org_grimseclabs_grimledger_GrimLedgerBridge_nativeSetVaultController(JNIEnv*, jobject, jlong ptr) {
    g_vaultController = reinterpret_cast<GrimVaultController*>(static_cast<intptr_t>(ptr));
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_grimseclabs_grimledger_GrimLedgerBridge_nativeCredentialsForOrigin(
    JNIEnv* env, jobject, jstring originUrl) {
    if (!g_vaultController || !g_vaultController->isUnlocked() || !originUrl) {
        return env->NewStringUTF("[]");
    }

    const char* originChars = env->GetStringUTFChars(originUrl, nullptr);
    if (!originChars) {
        return env->NewStringUTF("[]");
    }
    const QString origin = QString::fromUtf8(originChars);
    env->ReleaseStringUTFChars(originUrl, originChars);

    const QJsonArray matches = g_vaultController->credentialsForOrigin(origin);
    const QByteArray json = QJsonDocument(matches).toJson(QJsonDocument::Compact);
    return env->NewStringUTF(json.constData());
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_grimseclabs_grimledger_GrimLedgerBridge_nativeFillCredential(
    JNIEnv* env, jobject, jstring credentialId, jstring field) {
    if (!g_vaultController || !g_vaultController->isUnlocked() || !credentialId || !field) {
        return env->NewStringUTF("");
    }

    const char* idChars = env->GetStringUTFChars(credentialId, nullptr);
    const char* fieldChars = env->GetStringUTFChars(field, nullptr);
    if (!idChars || !fieldChars) {
        if (idChars) env->ReleaseStringUTFChars(credentialId, idChars);
        if (fieldChars) env->ReleaseStringUTFChars(field, fieldChars);
        return env->NewStringUTF("");
    }
    const QString value = g_vaultController->credentialField(
        QString::fromUtf8(idChars),
        QString::fromUtf8(fieldChars));
    env->ReleaseStringUTFChars(credentialId, idChars);
    env->ReleaseStringUTFChars(field, fieldChars);
    return env->NewStringUTF(value.toUtf8().constData());
}

void AndroidJniBridge_registerController(GrimVaultController* controller) {
    g_vaultController = controller;
    QJniObject::callStaticMethod<void>(
        "org/grimseclabs/grimledger/GrimLedgerBridge",
        "registerNativeController",
        "(J)V",
        static_cast<jlong>(reinterpret_cast<intptr_t>(controller)));
}
