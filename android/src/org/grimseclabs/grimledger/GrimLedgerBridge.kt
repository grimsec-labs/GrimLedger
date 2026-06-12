package org.grimseclabs.grimledger

object GrimLedgerBridge {
    @JvmStatic
    external fun nativeSetVaultController(ptr: Long)

    @JvmStatic
    external fun nativeCredentialsForOrigin(originUrl: String): String

    @JvmStatic
    external fun nativeFillCredential(credentialId: String, field: String): String

    @JvmStatic
    external fun nativeOnCameraResult(filePath: String)

    @JvmStatic
    fun registerNativeController(ptr: Long) {
        nativeSetVaultController(ptr)
    }

    @JvmStatic
    fun onCameraResult(filePath: String) {
        nativeOnCameraResult(filePath)
    }
}
