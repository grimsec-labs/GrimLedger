package org.grimseclabs.grimledger

import android.content.Context
import android.util.Base64
import androidx.biometric.BiometricManager
import androidx.security.crypto.EncryptedSharedPreferences
import androidx.security.crypto.MasterKey

object GrimLedgerBiometricUnlock {
    private const val PREFS = "grimledger_biometric"
    private const val KEY_WRAPPED = "wrapped_vault_key"

    fun isSupported(context: Context): Boolean {
        return BiometricManager.from(context).canAuthenticate(
            BiometricManager.Authenticators.BIOMETRIC_STRONG
        ) == BiometricManager.BIOMETRIC_SUCCESS
    }

    fun storeVaultKey(context: Context, keyBytes: ByteArray): Boolean {
        return try {
            val prefs = encryptedPrefs(context)
            val encoded = Base64.encodeToString(keyBytes, Base64.NO_WRAP)
            prefs.edit().putString(KEY_WRAPPED, encoded).commit()
        } catch (_: Exception) {
            false
        }
    }

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
