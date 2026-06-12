package org.grimseclabs.grimledger

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.net.Uri
import android.os.Bundle
import android.provider.MediaStore
import android.util.Log
import androidx.core.content.FileProvider
import java.io.File
import java.io.FileOutputStream

class GrimCameraActivity : Activity() {
    private var captureFilePath: String? = null
    private var useFileProvider: Boolean = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val restored = savedInstanceState?.getString(KEY_CAPTURE_PATH)
        if (restored != null) {
            captureFilePath = restored
            useFileProvider = savedInstanceState.getBoolean(KEY_USE_FP, false)
            return
        }

        val intent = Intent(MediaStore.ACTION_IMAGE_CAPTURE)

        try {
            val dir = File(cacheDir, "grimcapture")
            dir.mkdirs()
            val tempFile = File(dir, "capture_${System.currentTimeMillis()}.jpg")
            captureFilePath = tempFile.absolutePath

            val uri: Uri = FileProvider.getUriForFile(
                this, "org.grimseclabs.grimledger.fileprovider", tempFile)
            intent.putExtra(MediaStore.EXTRA_OUTPUT, uri)
            intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            useFileProvider = true
        } catch (e: Exception) {
            Log.w(TAG, "FileProvider unavailable, thumbnail mode", e)
            useFileProvider = false
        }

        try {
            startActivityForResult(intent, REQUEST_CODE)
        } catch (e: Exception) {
            Log.e(TAG, "Camera intent failed", e)
            cleanup()
            finish()
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        captureFilePath?.let { outState.putString(KEY_CAPTURE_PATH, it) }
        outState.putBoolean(KEY_USE_FP, useFileProvider)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode != REQUEST_CODE) {
            cleanup()
            finish()
            return
        }

        if (resultCode != RESULT_OK) {
            cleanup()
            finish()
            return
        }

        if (useFileProvider) {
            val path = captureFilePath
            val file = if (path != null) File(path) else null
            if (file != null && file.exists() && file.length() > 0) {
                GrimLedgerBridge.onCameraResult(file.absolutePath)
            } else {
                saveThumbnailFallback(data)
            }
        } else {
            saveThumbnailFallback(data)
        }
        finish()
    }

    private fun saveThumbnailFallback(data: Intent?) {
        val bitmap = data?.extras?.get("data") as? Bitmap
        if (bitmap == null) {
            cleanup()
            return
        }

        try {
            val dir = File(cacheDir, "grimcapture")
            dir.mkdirs()
            val tempFile = File(dir, "capture_${System.currentTimeMillis()}.png")
            FileOutputStream(tempFile).use { out ->
                bitmap.compress(Bitmap.CompressFormat.PNG, 100, out)
            }
            captureFilePath = tempFile.absolutePath
            GrimLedgerBridge.onCameraResult(tempFile.absolutePath)
        } catch (e: Exception) {
            Log.e(TAG, "Thumbnail save failed", e)
            cleanup()
        }
    }

    private fun cleanup() {
        captureFilePath?.let {
            try { File(it).delete() } catch (_: Exception) {}
        }
        captureFilePath = null
    }

    companion object {
        private const val TAG = "GrimCamera"
        private const val REQUEST_CODE = 9001
        private const val KEY_CAPTURE_PATH = "capture_path"
        private const val KEY_USE_FP = "use_fp"

        @JvmStatic
        fun launch(context: Context) {
            val intent = Intent(context, GrimCameraActivity::class.java)
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            context.startActivity(intent)
        }

        @JvmStatic
        fun deleteTempFile(path: String) {
            try { File(path).delete() } catch (_: Exception) {}
        }
    }
}
