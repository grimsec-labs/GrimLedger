package org.grimseclabs.grimledger

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.provider.MediaStore
import androidx.core.content.FileProvider
import java.io.File

/**
 * Launches ACTION_IMAGE_CAPTURE into an app-private temp file. The temp file
 * is cleaned up by the C++ caller after sanitization — never written to public storage.
 */
object GrimCameraHelper {
    private const val REQUEST_CODE = 9001
    private var pendingCaptureFile: File? = null

    @JvmStatic
    fun launchCamera(activity: Activity): Boolean {
        val intent = Intent(MediaStore.ACTION_IMAGE_CAPTURE)
        if (intent.resolveActivity(activity.packageManager) == null) {
            return false
        }

        val cacheDir = File(activity.cacheDir, "grimcapture")
        cacheDir.mkdirs()
        val tempFile = File(cacheDir, "capture_${System.currentTimeMillis()}.jpg")
        pendingCaptureFile = tempFile

        val uri: Uri = FileProvider.getUriForFile(
            activity,
            "${activity.packageName}.fileprovider",
            tempFile
        )
        intent.putExtra(MediaStore.EXTRA_OUTPUT, uri)
        intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)

        activity.startActivityForResult(intent, REQUEST_CODE)
        return true
    }

    @JvmStatic
    fun handleActivityResult(requestCode: Int, resultCode: Int): String? {
        if (requestCode != REQUEST_CODE) return null
        if (resultCode != Activity.RESULT_OK) {
            cleanupPendingFile()
            return null
        }
        val file = pendingCaptureFile ?: return null
        pendingCaptureFile = null
        if (file.exists() && file.length() > 0) {
            val path = file.absolutePath
            GrimLedgerBridge.onCameraResult(path)
            return path
        }
        return null
    }

    @JvmStatic
    fun deleteTempFile(path: String) {
        try {
            File(path).delete()
        } catch (_: Exception) {
        }
    }

    private fun cleanupPendingFile() {
        pendingCaptureFile?.let {
            try { it.delete() } catch (_: Exception) {}
        }
        pendingCaptureFile = null
    }
}
