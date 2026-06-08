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

        if (usernameId == null || passwordId == null || webOrigin.isEmpty()) {
            callback.onSuccess(null)
            return
        }

        val usernameFieldId = usernameId!!
        val passwordFieldId = passwordId!!

        val json = GrimLedgerBridge.nativeCredentialsForOrigin("https://$webOrigin")
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
}
