package org.grimseclabs.grimledger

object GrimLedgerBridge {
    // Native lib is loaded by Qt (android.app.lib_name); do not call loadLibrary here —
    // the APK ships libGrimLedger_<abi>.so, not libGrimLedger.so.

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
