function getNativeInputValueSetter(input) {
  const prototype = Object.getPrototypeOf(input);
  const descriptor = Object.getOwnPropertyDescriptor(prototype, "value");
  return descriptor && descriptor.set;
}

function setNativeInputValue(input, value) {
  if (!input) {
    return;
  }
  const setter = getNativeInputValueSetter(input);
  if (setter) {
    setter.call(input, value);
  } else {
    input.value = value;
  }
  input.dispatchEvent(new Event("input", { bubbles: true }));
  input.dispatchEvent(new Event("change", { bubbles: true }));
}

function collectRoots(root, out) {
  out.push(root);
  const walker = document.createTreeWalker(root, NodeFilter.SHOW_ELEMENT);
  let node = walker.nextNode();
  while (node) {
    if (node.shadowRoot) {
      collectRoots(node.shadowRoot, out);
    }
    node = walker.nextNode();
  }
}

function queryAllDeep(selector) {
  const roots = [document];
  collectRoots(document, roots);
  const matches = [];
  for (const root of roots) {
    matches.push(...root.querySelectorAll(selector));
  }
  return matches;
}

function findLoginFields() {
  const passwordCandidates = queryAllDeep(
    'input[autocomplete="current-password"], input[type="password"]'
  );
  const passwordInput = passwordCandidates.find((input) => input.offsetParent !== null) || passwordCandidates[0];
  if (!passwordInput) {
    return null;
  }

  let usernameInput = queryAllDeep('input[autocomplete="username"], input[autocomplete="email"]')
    .find((input) => input.offsetParent !== null);

  if (!usernameInput) {
    const form = passwordInput.closest("form");
    const scope = form || document;
    const candidates = scope.querySelectorAll(
      'input[type="email"], input[type="text"], input:not([type])'
    );
    for (const input of candidates) {
      if (input === passwordInput) continue;
      const type = (input.getAttribute("type") || "").toLowerCase();
      if (type === "hidden" || type === "submit" || type === "button") continue;
      if (input.autocomplete === "new-password") continue;
      usernameInput = input;
      break;
    }
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
    setNativeInputValue(fields.usernameInput, username);
  }

  if (fields.passwordInput && password) {
    fields.passwordInput.focus();
    setNativeInputValue(fields.passwordInput, password);
  }

  return true;
}

function getSelectionText() {
  const selection = window.getSelection();
  if (!selection || selection.isCollapsed) {
    return "";
  }
  return selection.toString().trim();
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

  if (message.action === "get_clip_payload") {
    const text = getSelectionText();
    sendResponse({
      ok: text.length > 0,
      title: document.title || "Clipped page",
      url: window.location.href,
      text,
      origin: window.location.origin,
    });
    return true;
  }

  return false;
});
