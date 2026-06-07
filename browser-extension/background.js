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

async function fillCredentials(id, origin, tabId) {
  const nonce = crypto.randomUUID();
  const fill = await nativeRequest({
    action: "fill",
    id,
    origin,
    nonce,
  });

  if (!fill || fill.ok === false) {
    throw new Error(fill?.error || "Bridge fill request failed");
  }
  if (fill.nonce !== nonce) {
    throw new Error("Fill response nonce mismatch");
  }

  const currentTab = await chrome.tabs.get(tabId);
  if (!currentTab?.url || new URL(currentTab.url).origin !== origin) {
    throw new Error("Active tab changed before fill could complete.");
  }

  const fillResult = await chrome.tabs.sendMessage(tabId, {
    action: "fill_on_page",
    username: fill.username,
    password: fill.password,
    expectedOrigin: origin,
  });

  if (!fillResult || !fillResult.ok) {
    throw new Error(fillResult?.error || "Could not find login fields on this page.");
  }

  return { ok: true };
}

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (!message || !message.action) {
    return false;
  }

  if (message.action === "clip_to_grimledger") {
    chrome.tabs.sendMessage(message.tabId, { action: "get_clip_payload" })
      .then((clip) => {
        if (!clip || !clip.ok) {
          throw new Error("Select text on the page before clipping.");
        }
        return nativeRequest({
          action: "clip",
          title: clip.title,
          url: clip.url,
          text: clip.text,
          origin: clip.origin,
        });
      })
      .then((response) => {
        if (response && response.ok === false) {
          sendResponse({ ok: false, error: response.error || "Clip failed" });
          return;
        }
        sendResponse({ ok: true, response });
      })
      .catch((error) => sendResponse({ ok: false, error: error.message || String(error) }));
    return true;
  }

  if (message.action === "fill_credentials") {
    fillCredentials(message.id, message.origin, message.tabId)
      .then((result) => sendResponse(result))
      .catch((error) => sendResponse({ ok: false, error: error.message || String(error) }));
    return true;
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
