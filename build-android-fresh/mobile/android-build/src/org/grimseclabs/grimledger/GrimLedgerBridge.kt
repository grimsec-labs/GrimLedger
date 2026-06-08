package org.grimseclabs.grimledger

object GrimLedgerBridge {
    // NOTE: Do NOT call System.loadLibrary() here. The native JNI symbols
    // (Java_org_grimseclabs_grimledger_GrimLedgerBridge_*) are compiled into the
    // application's main Qt library (libGrimLedger_<abi>.so), which Qt has already
    // loaded by the time any Kotlin runs. A bare System.loadLibrary("GrimLedger")
    // resolves to "libGrimLedger.so" (no ABI suffix), which does not exist in the
    // APK, and throws UnsatisfiedLinkError during controller registration in the
    // GrimVaultController constructor — crashing the app at launch.

    @JvmStatic
    external fun nativeSetVaultController(ptr: Long)

    @JvmStatic
    external fun nativeCredentialsForOrigin(originUrl: String): String

    @JvmStatic
    external fun nativeFillCredential(credentialId: String, field: String): String

    @JvmStatic
    fun registerNativeController(ptr: Long) {
        nativeSetVaultController(ptr)
    }
}
