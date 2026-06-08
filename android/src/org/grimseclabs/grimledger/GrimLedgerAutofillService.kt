package org.grimseclabs.grimledger

import android.app.assist.AssistStructure
import android.os.CancellationSignal
import android.service.autofill.AutofillService
import android.service.autofill.Dataset
import android.service.autofill.FillCallback
import android.service.autofill.FillRequest
import android.service.autofill.FillResponse
import android.view.autofill.AutofillId
import android.view.autofill.AutofillValue
import android.widget.RemoteViews
import org.json.JSONArray

class GrimLedgerAutofillService : AutofillService() {

    override fun onFillRequest(request: FillRequest, cancellationSignal: CancellationSignal, callback: FillCallback) {
        val structure = request.fillContexts.lastOrNull()?.structure ?: run {
            callback.onSuccess(null)
            return
        }

        var webOrigin = ""
        var usernameId: AutofillId? = null
        var passwordId: AutofillId? = null

        for (i in 0 until structure.windowNodeCount) {
            val root = structure.getWindowNodeAt(i).rootViewNode
            visitNode(root) { node ->
                val hint = node.hint ?: node.htmlInfo?.tag ?: ""
                if (usernameId == null && (hint.contains("username", true) || hint.contains("email", true))) {
                    usernameId = node.autofillId
                }
                if (passwordId == null && hint.contains("password", true)) {
                    passwordId = node.autofillId
                }
                if (webOrigin.isEmpty()) {
                    webOrigin = node.webDomain ?: ""
                }
            }
        }

        if (usernameId == null || passwordId == null) {
            callback.onSuccess(null)
            return
        }

        val usernameFieldId = usernameId!!
        val passwordFieldId = passwordId!!

        // GL-SEC-003: do NOT trust an app-supplied webDomain unless the request
        // originates from a known browser. A malicious app can set webDomain to any
        // value to phish credentials, so for non-browser callers we key the lookup
        // off the verified requesting package name instead.
        val requestingPackage = structure.activityComponent?.packageName ?: ""
        val lookupOrigin = resolveLookupOrigin(requestingPackage, webOrigin)
        if (lookupOrigin.isEmpty()) {
            callback.onSuccess(null)
            return
        }

        val json = GrimLedgerBridge.nativeCredentialsForOrigin(lookupOrigin)
        val array = JSONArray(json)
        if (array.length() == 0) {
            callback.onSuccess(null)
            return
        }

        val first = array.getJSONObject(0)
        val credId = first.getString("id")
        val username = GrimLedgerBridge.nativeFillCredential(credId, "username")
        val password = GrimLedgerBridge.nativeFillCredential(credId, "password")

        val presentation = RemoteViews(packageName, android.R.layout.simple_list_item_1).apply {
            setTextViewText(android.R.id.text1, first.optString("label", "GrimLedger"))
        }

        val dataset = Dataset.Builder(presentation)
            .setValue(usernameFieldId, AutofillValue.forText(username))
            .setValue(passwordFieldId, AutofillValue.forText(password))
            .build()

        callback.onSuccess(FillResponse.Builder().addDataset(dataset).build())
    }

    override fun onSaveRequest(request: android.service.autofill.SaveRequest, callback: android.service.autofill.SaveCallback) {
        callback.onSuccess()
    }

    private fun visitNode(node: AssistStructure.ViewNode, block: (AssistStructure.ViewNode) -> Unit) {
        block(node)
        for (i in 0 until node.childCount) {
            visitNode(node.getChildAt(i), block)
        }
    }

    /**
     * Decide which origin key to look credentials up by. A web domain is only honored
     * when the requesting package is a recognized browser; otherwise the verified
     * package name is used so that an arbitrary app cannot impersonate a website.
     */
    private fun resolveLookupOrigin(requestingPackage: String, webDomain: String): String {
        if (webDomain.isNotEmpty() && TRUSTED_BROWSERS.contains(requestingPackage)) {
            return "https://$webDomain"
        }
        if (requestingPackage.isNotEmpty()) {
            return "app://$requestingPackage"
        }
        // Unknown caller with no verifiable identity: fail closed, offer nothing.
        return ""
    }

    companion object {
        // Allowlist of browser packages whose reported webDomain we are willing to
        // trust for autofill matching.
        private val TRUSTED_BROWSERS = setOf(
            "com.android.chrome",
            "com.chrome.beta",
            "com.chrome.dev",
            "com.chrome.canary",
            "org.mozilla.firefox",
            "org.mozilla.firefox_beta",
            "org.mozilla.fenix",
            "com.microsoft.emmx",            // Edge
            "com.brave.browser",
            "com.opera.browser",
            "com.opera.gx",
            "com.duckduckgo.mobile.android",
            "com.sec.android.app.sbrowser",  // Samsung Internet
            "com.vivaldi.browser",
            "com.kiwibrowser.browser",
            "com.android.browser"
        )
    }
}
