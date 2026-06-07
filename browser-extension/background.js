const HOST_NAME = "com.grimledger.bridge";

function nativeRequest(payload) {
  return new Promise((resolve, reject) => {
    let port;
    try {
      port = chrome.runtime.connectNative(HOST_NAME);
    } catch (err) {
      reject(err);
      return;
    }

    let settled = false;

    const timeout = setTimeout(() => {
      if (settled) return;
      settled = true;
      try { port.disconnect(); } catch (_) {}
      reject(new Error("Native host timeout"));
    }, 12000);

    port.onMessage.addListener((response) => {
      if (settled) return;
      settled = true;
      clearTimeout(timeout);
      resolve(response);
      try { port.disconnect(); } catch (_) {}
    });

    port.onDisconnect.addListener(() => {
      if (settled) return;
      settled = true;
      clearTimeout(timeout);
      if (chrome.runtime.lastError) {
        reject(new Error(chrome.runtime.lastError.message));
      } else {
        reject(new Error("Native host disconnected"));
      }
    });

    port.postMessage(payload);
  });
}

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (!message || !message.action) {
    return false;
  }

  nativeRequest(message)
    .then((response) => {
      if (response && response.ok === false) {
        sendResponse({
          ok: false,
          error: response.error || "Bridge request failed",
        });
        return;
      }
      sendResponse({ ok: true, response });
    })
    .catch((error) => sendResponse({ ok: false, error: error.message || String(error) }));

  return true;
});
