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

async function getActiveTabOrigin() {
  const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
  if (!tab?.url || !tab.url.startsWith("http")) {
    return null;
  }
  return new URL(tab.url).origin;
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

    const origin = await getActiveTabOrigin();
    if (!origin) {
      statusEl.textContent = "Open an http(s) page to match vault keys.";
      return;
    }

    const result = await nativeRequest({ action: "list_matches", origin });
    const matches = result.matches || [];
    if (matches.length === 0) {
      statusEl.textContent = `No vault keys for ${origin}`;
      return;
    }

    statusEl.textContent = `${matches.length} match(es) for ${origin}`;
    for (const match of matches) {
      const btn = document.createElement("button");
      btn.className = "match";
      btn.textContent = `${match.label} — ${match.username || "(no username)"}`;
      btn.addEventListener("click", () => fillMatch(match.id, origin));
      matchesEl.appendChild(btn);
    }
  } catch (error) {
    statusEl.textContent = error.message;
    statusEl.classList.add("error");
  }
}

async function fillMatch(id, origin) {
  try {
    const fill = await nativeRequest({ action: "fill", id, origin });
    const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
    await chrome.tabs.sendMessage(tab.id, {
      action: "fill_on_page",
      username: fill.username,
      password: fill.password,
    });
    statusEl.textContent = "Filled. Confirm in GrimLedger if prompted.";
  } catch (error) {
    statusEl.textContent = error.message;
    statusEl.classList.add("error");
  }
}

document.getElementById("refresh").addEventListener("click", refresh);
refresh();
