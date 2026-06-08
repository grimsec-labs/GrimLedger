package org.grimseclabs.grimledger

import android.content.Context
import android.util.Base64
import androidx.biometric.BiometricManager
import androidx.security.crypto.EncryptedSharedPreferences
import androidx.security.crypto.MasterKey

/**
 * Biometric / fast-unlock storage for the vault key.
 *
 * Security model (see GL-SEC-002):
 *  - The wrapped vault key is stored ONLY inside EncryptedSharedPreferences, whose
 *    MasterKey lives in the hardware-backed Android Keystore and is gated by
 *    `setUserAuthenticationRequired(true, ...)`. Decryption therefore requires the
 *    user to have authenticated (device credential / biometric) recently; the OS
 *    enforces this at the Keystore boundary.
 *  - The key is NEVER written to QSettings or any plaintext store, and there is no
 *    plaintext fallback path. The native side keeps only a non-secret boolean marker.
 *
 * Follow-up (recommended, deferred): gate `loadVaultKey` behind an explicit
 * androidx.biometric.BiometricPrompt + CryptoObject so a fresh biometric gesture is
 * required for every unlock rather than relying on the Keystore validity window.
 */
object GrimLedgerBiometricUnlock {
    private const val PREFS = "grimledger_biometric"
    private const val KEY_WRAPPED = "wrapped_vault_key"

    fun isSupported(context: Context): Boolean {
        return BiometricManager.from(context).canAuthenticate(
            BiometricManager.Authenticators.BIOMETRIC_STRONG
        ) == BiometricManager.BIOMETRIC_SUCCESS
    }

    /** Store the vault key in Keystore-backed EncryptedSharedPreferences only.
     *  Returns true on success. Never returns or persists the key in plaintext. */
    fun storeVaultKey(context: Context, keyBytes: ByteArray): Boolean {
        return try {
            val prefs = encryptedPrefs(context)
            val encoded = Base64.encodeToString(keyBytes, Base64.NO_WRAP)
            // commit() (synchronous) so the caller's success signal reflects the write.
            prefs.edit().putString(KEY_WRAPPED, encoded).commit()
        } catch (_: Exception) {
            false
        }
    }

    /** Load the vault key from Keystore-backed storage. Returns null if absent or if
     *  the user has not satisfied the Keystore auth requirement. No plaintext fallback. */
    fun loadVaultKey(context: Context): ByteArray? {
        return try {
            val prefs = encryptedPrefs(context)
            val encoded = prefs.getString(KEY_WRAPPED, null) ?: return null
            Base64.decode(encoded, Base64.NO_WRAP)
        } catch (_: Exception) {
            null
        }
    }

    fun isStored(context: Context): Boolean {
        return try {
            encryptedPrefs(context).contains(KEY_WRAPPED)
        } catch (_: Exception) {
            false
        }
    }

    fun clear(context: Context) {
        try {
            encryptedPrefs(context).edit().clear().apply()
        } catch (_: Exception) {
            // best effort
        }
    }

    private fun encryptedPrefs(context: Context) =
        EncryptedSharedPreferences.create(
            context,
            PREFS,
            MasterKey.Builder(context)
                .setKeyScheme(MasterKey.KeyScheme.AES256_GCM)
                .setUserAuthenticationRequired(true, 30)
                .build(),
            EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
            EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM
        )
}
