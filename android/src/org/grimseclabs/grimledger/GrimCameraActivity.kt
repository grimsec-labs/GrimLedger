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
    private var captureFile: File? = null
    private var useFileProvider: Boolean = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.d(TAG, "onCreate started")

        val intent = Intent(MediaStore.ACTION_IMAGE_CAPTURE)

        try {
            val dir = File(cacheDir, "grimcapture")
            dir.mkdirs()
            val tempFile = File(dir, "capture_${System.currentTimeMillis()}.jpg")
            captureFile = tempFile

            val uri: Uri = FileProvider.getUriForFile(
                this, "org.grimseclabs.grimledger.fileprovider", tempFile)
            intent.putExtra(MediaStore.EXTRA_OUTPUT, uri)
            intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            useFileProvider = true
            Log.d(TAG, "FileProvider URI created: $uri")
        } catch (e: Exception) {
            Log.w(TAG, "FileProvider failed, falling back to thumbnail mode", e)
            useFileProvider = false
        }

        try {
            startActivityForResult(intent, REQUEST_CODE)
            Log.d(TAG, "Camera intent launched")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to launch camera intent", e)
            cleanup()
            finish()
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        Log.d(TAG, "onActivityResult: requestCode=$requestCode resultCode=$resultCode")

        if (requestCode != REQUEST_CODE) {
            cleanup()
            finish()
            return
        }

        if (resultCode != RESULT_OK) {
            Log.d(TAG, "Camera cancelled or failed")
            cleanup()
            finish()
            return
        }

        if (useFileProvider) {
            val file = captureFile
            if (file != null && file.exists() && file.length() > 0) {
                Log.d(TAG, "FileProvider capture OK: ${file.absolutePath} (${file.length()} bytes)")
                GrimLedgerBridge.onCameraResult(file.absolutePath)
            } else {
                Log.w(TAG, "FileProvider file missing or empty, trying thumbnail fallback")
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
            Log.e(TAG, "No thumbnail bitmap in intent extras")
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
            captureFile = tempFile
            Log.d(TAG, "Thumbnail saved: ${tempFile.absolutePath} (${tempFile.length()} bytes)")
            GrimLedgerBridge.onCameraResult(tempFile.absolutePath)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to save thumbnail", e)
            cleanup()
        }
    }

    private fun cleanup() {
        captureFile?.let {
            try { it.delete() } catch (_: Exception) {}
        }
        captureFile = null
    }

    companion object {
        private const val TAG = "GrimCamera"
        private const val REQUEST_CODE = 9001

        @JvmStatic
        fun launch(context: Context) {
            Log.d(TAG, "launch() called")
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
