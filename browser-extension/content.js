function findLoginFields() {
  const passwordInput = document.querySelector('input[type="password"]');
  if (!passwordInput) {
    return null;
  }

  let usernameInput = null;
  const form = passwordInput.closest("form");
  const candidates = form
    ? form.querySelectorAll('input[type="email"], input[type="text"], input:not([type])')
    : document.querySelectorAll('input[type="email"], input[type="text"], input:not([type])');

  for (const input of candidates) {
    if (input === passwordInput) continue;
    const type = (input.getAttribute("type") || "").toLowerCase();
    if (type === "hidden" || type === "submit" || type === "button") continue;
    if (input.autocomplete === "new-password") continue;
    usernameInput = input;
    break;
  }

  return { usernameInput, passwordInput };
}

function fillFields(username, password) {
  const fields = findLoginFields();
  if (!fields) {
    return false;
  }

  if (fields.usernameInput && username) {
    fields.usernameInput.focus();
    fields.usernameInput.value = username;
    fields.usernameInput.dispatchEvent(new Event("input", { bubbles: true }));
    fields.usernameInput.dispatchEvent(new Event("change", { bubbles: true }));
  }

  if (fields.passwordInput && password) {
    fields.passwordInput.focus();
    fields.passwordInput.value = password;
    fields.passwordInput.dispatchEvent(new Event("input", { bubbles: true }));
    fields.passwordInput.dispatchEvent(new Event("change", { bubbles: true }));
  }

  return true;
}

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (sender.id !== chrome.runtime.id) {
    return false;
  }

  if (message.action === "fill_on_page") {
    if (message.expectedOrigin && window.location.origin !== message.expectedOrigin) {
      sendResponse({ ok: false, error: "origin mismatch" });
      return true;
    }
    sendResponse({ ok: fillFields(message.username || "", message.password || "") });
    return true;
  }
  return false;
});
