package org.grimseclabs.grimledger

object GrimLedgerBridge {
    init {
        System.loadLibrary("GrimLedger")
    }

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
