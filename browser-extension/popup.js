const statusEl = document.getElementById("status");
const matchesEl = document.getElementById("matches");

function nativeRequest(payload) {
  return new Promise((resolve, reject) => {
    chrome.runtime.sendMessage(payload, (response) => {
      if (chrome.runtime.lastError) {
        reject(new Error(chrome.runtime.lastError.message));
        return;
      }
      if (!response || !response.ok) {
        reject(new Error(response?.error || "Bridge request failed"));
        return;
      }
      resolve(response.response);
    });
  });
}

async function getActiveTab() {
  const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
  if (!tab?.id || !tab.url || !tab.url.startsWith("http")) {
    return null;
  }
  return {
    tabId: tab.id,
    origin: new URL(tab.url).origin,
    url: tab.url,
  };
}

async function refresh() {
  matchesEl.innerHTML = "";
  statusEl.textContent = "Checking vault...";
  statusEl.classList.remove("error");

  try {
    const ping = await nativeRequest({ action: "ping" });
    if (ping.locked) {
      statusEl.textContent = "Vault is locked. Unlock GrimLedger first.";
      return;
    }

    const tab = await getActiveTab();
    if (!tab) {
      statusEl.textContent = "Open an http(s) page to match vault keys.";
      return;
    }

    const result = await nativeRequest({ action: "list_matches", origin: tab.origin });
    const matches = result.matches || [];
    if (matches.length === 0) {
      statusEl.textContent = `No vault keys for ${tab.origin}`;
      return;
    }

    statusEl.textContent = `${matches.length} match(es) for ${tab.origin}`;
    for (const match of matches) {
      const btn = document.createElement("button");
      btn.className = "match";
      btn.textContent = `${match.label} — ${match.username || "(no username)"}`;
      btn.addEventListener("click", () => fillMatch(match.id, tab));
      matchesEl.appendChild(btn);
    }
  } catch (error) {
    statusEl.textContent = error.message;
    statusEl.classList.add("error");
  }
}

async function fillMatch(id, tab) {
  const nonce = crypto.randomUUID();
  try {
    const fill = await nativeRequest({
      action: "fill",
      id,
      origin: tab.origin,
      nonce,
    });

    const currentTab = await chrome.tabs.get(tab.tabId);
    if (!currentTab?.url || new URL(currentTab.url).origin !== tab.origin) {
      throw new Error("Active tab changed before fill could complete.");
    }

    const fillResult = await chrome.tabs.sendMessage(tab.tabId, {
      action: "fill_on_page",
      username: fill.username,
      password: fill.password,
      expectedOrigin: tab.origin,
    });

    if (!fillResult || !fillResult.ok) {
      throw new Error("Could not find login fields on this page.");
    }

    statusEl.textContent = "Filled successfully.";
  } catch (error) {
    statusEl.textContent = error.message;
    statusEl.classList.add("error");
  }
}

document.getElementById("refresh").addEventListener("click", refresh);
refresh();
