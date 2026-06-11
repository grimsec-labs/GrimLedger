# Android app aesthetics plan — match desktop GrimLedger

This document describes how to evolve the Qt Quick Android app (`mobile/`) so it looks and feels like the desktop app (`src/ui/` + `resources/styles/grimledger_dark.qss`), without rewriting the UI in native Kotlin.

**Goal:** Same dark infernal / hacker aesthetic — not a generic Material Android app.

**Non-goal:** Pixel-perfect clone of every desktop widget. Mobile gets the same *design language*, adapted for touch and smaller screens.

---

## Current state

| Area | Desktop | Android (today) |
|------|---------|-----------------|
| UI stack | Qt Widgets + QSS | Qt Quick (QML) |
| Theming | Central `Theme.cpp` + `grimledger_dark.qss` | Hardcoded hex colors in each `.qml` file |
| Accent | 4 presets, persisted, live preview | `vault.accentColor` exists in C++ but no picker; many labels still `#cc2200` |
| Components | `#PrimaryButton`, `#PasswordField`, list selection, dialogs | Stock `Button`, `TextField`, `ItemDelegate` |
| Login | Gradient background, monospace prompt, styled primary button | Flat `#0a0a0c`, default buttons |
| `Theme.qml` | N/A | Stub with one function — **not imported anywhere** |

**What already matches (palette only):**

- Background `#0a0a0c`, surfaces `#111114`, borders `#331111`
- Text `#e8e8ec` / `#d4d4dc`, muted `#998877`
- Terminal green prompt `#33cc66`, error `#ff4444`
- Default accent `#cc2200`

**What’s missing:** systematic tokens, reusable styled controls, hover/pressed/selected states, login atmosphere, typography, and accent wiring end-to-end.

---

## Strategy: Qt Quick design system, not a rewrite

Keep the current architecture:

```
grimledger_core  →  GrimVaultController  →  QML screens
                           ↓
              Kotlin (autofill + biometric only)
```

Do **not** rebuild the UI in Jetpack Compose. Shared crypto/storage and future iOS reuse stay intact. Kotlin remains for Android system integration only (`android/src/org/grimseclabs/grimledger/`).

---

## Phase 1 — Design tokens (foundation)

### 1.1 Turn `Theme.qml` into a singleton

**File:** `mobile/qml/Theme.qml`

Register as a QML singleton in `mobile/CMakeLists.txt` / `mobile.qrc` (via `qt_add_qml_module` or `qmldir`).

Mirror desktop tokens from `grimledger_dark.qss` and `src/utils/Theme.cpp`:

| Token | Desktop source | Value (default) |
|-------|----------------|-----------------|
| `background` | `QMainWindow` | `#0a0a0c` |
| `surface` | inputs / panels | `#111114` |
| `surfaceRaised` | toolbar, tab bar | `#0e0e12` |
| `listBackground` | `QListWidget` | `#0c0c10` |
| `border` | general | `#2a2a30` |
| `borderInfernal` | login / infernal panels | `#331111` |
| `textPrimary` | body | `#d4d4dc` |
| `textBright` | editor text | `#e8e8ec` |
| `textMuted` | tabs inactive, hints | `#998877` |
| `textDim` | empty states | `#665566` |
| `terminalGreen` | `#LoginPrompt` | `#33cc66` |
| `error` | login errors | `#ff4444` |
| `selectionBg` | text selection | `#442200` |
| `selectionText` | text selection | `#ffcc88` |
| `accent` | `@@ACCENT@@` | `#cc2200` (dynamic) |
| `accentHover` | `@@ACCENT_HOVER@@` | computed: `accent.lighter(125)` |
| `accentSoft` | `@@ACCENT_SOFT@@` | computed: `accent.darker(150)` |

**Accent presets** (same as `SettingsWindow.cpp`):

| Name | Hex |
|------|-----|
| Hellfire Red | `#cc2200` |
| Ember Orange | `#ff6600` |
| Terminal Green | `#00cc66` |
| Abyss Purple | `#8844cc` |

Expose on the singleton:

- `readonly property color accent` — bound to `vault.accentColor`
- `function accentHover()`, `function accentSoft()` — QColor math in QML or small C++ helper on `GrimVaultController`
- `readonly property string monoFont` — `"Consolas", "Courier New", monospace`
- `readonly property string uiFont` — `"Segoe UI", sans-serif` (Android: `"Roboto", sans-serif` fallback)

### 1.2 Wire accent persistence

Desktop uses `Theme::saveAccent()` → `QSettings` key `appearance/accent`.

**Tasks:**

1. In `GrimVaultController`, load/save accent via the same `QSettings` key on startup and when changed.
2. Add `Q_INVOKABLE void setAccentPreset(int index)` or `setAccentColor(QString hex)` callable from Settings QML.
3. Emit `accentColorChanged()` so all `Theme`-aware components update live.

### 1.3 App-wide defaults in `main.cpp`

Before loading QML:

```cpp
QQuickStyle::setStyle("Basic");  // or "Fusion" — avoid Material
```

Import `Theme` in `main.qml` and set:

```qml
ApplicationWindow {
    color: Theme.background
    font.family: Theme.uiFont
}
```

---

## Phase 2 — Reusable components

Create `mobile/qml/controls/` and use them everywhere instead of raw Qt Quick Controls.

Each component imports `Theme` and accepts optional `primary: true` where relevant.

| Component | Replaces | Desktop QSS reference |
|-----------|----------|------------------------|
| `GrimButton.qml` | `Button` | `#PrimaryButton`, default `QPushButton` |
| `GrimTextField.qml` | `TextField` | `#PasswordField`, `QLineEdit` |
| `GrimTextArea.qml` | `TextArea` | editor fields |
| `GrimListItem.qml` | `ItemDelegate` | `QListWidget::item:selected` — left accent bar |
| `GrimTabButton.qml` | custom `TabButton` content | tab bar in `main.qml` |
| `GrimSectionLabel.qml` | hardcoded red `Label` | settings section headers |
| `GrimCheckBox.qml` | `CheckBox` | styled tick + infernal border |
| `GrimDialog.qml` | future dialogs | `#GrimDialog` |

### `GrimButton` behavior

**Primary** (Unlock, Create Vault, Save):

- Background `#1a0808`, border `Theme.accent`, text `Theme.accent`, bold
- Pressed: background `#2a1010`, border `Theme.accentHover`

**Secondary** (New, Reset):

- Background `#111114`, border `#2a2a30`, text `#d4d4dc`

Use `contentItem` + `background` rectangles; do not rely on implicit Material styling.

### `GrimTextField` behavior

- Background `#111114`, border `#331111`, radius `3`, monospace for password fields
- Focus: border `Theme.accent` (matches `#PasswordField:focus`)

### `GrimListItem` behavior

- Background transparent; selected: `#1a1010`, text `Theme.accent`, **2px left border** `Theme.accent`
- Matches desktop list selection in QSS

Add all controls to `mobile.qrc` and import via `import "controls"`.

---

## Phase 3 — Screen-by-screen restyle

### 3.1 Login — `LoginScreen.qml`

Target: evoke `#LoginContent` gradient + `#LoginPrompt` terminal feel.

| Element | Change |
|---------|--------|
| Background | `LinearGradient` — stops `#08080a`, `#0e0a0c`, `#0a0808` (see QSS `#LoginContent`) |
| Title | `GRIMLEDGER`, monospace, letter-spacing ~2px, `Theme.accent` |
| Prompt | `> enter master key:`, `Theme.terminalGreen`, monospace |
| Password | `GrimTextField` with `monospace: true`, `echoMode: Password` |
| Actions | `GrimButton { primary: true }` for Unlock / Create; secondary for Biometric |
| Errors | `Theme.error`, word wrap |

Optional: compact ASCII logo (desktop has full banner in login; mobile uses 3–5 line subset or `resources/icon.png` above title).

### 3.2 Shell — `main.qml`

| Element | Change |
|---------|--------|
| `TabBar` background | `Theme.surfaceRaised`, bottom border `#331111` |
| Tabs | `GrimTabButton` — checked `Theme.accent`, unchecked `#998877`, monospace |
| Content area | `Theme.background` |
| Lock navigation | On `vault.unlocked` false → `stack.replace(loginScreen)` (fix current gap) |

### 3.3 Notes — `NotesScreen.qml`

| Element | Change |
|---------|--------|
| Toolbar row | `GrimButton` secondary for + New, primary for Save |
| Note list | `GrimListItem` delegates, `Theme.listBackground`, fixed height ~140dp or use split layout on tablets |
| Title / body | `GrimTextField` + `GrimTextArea` |
| Empty state | centered `#665566` monospace hint (like `#EmptyEditor`) |

Later (Phase 4): preview pane with markdown styling — not required for aesthetic parity of chrome.

### 3.4 Credentials — `CredentialsScreen.qml`

| Element | Change |
|---------|--------|
| List | `GrimListItem` — title bright, URL `#998877` below |
| Section header | `GrimSectionLabel` |

### 3.5 Settings — `SettingsScreen.qml`

| Element | Change |
|---------|--------|
| Section headers | `GrimSectionLabel` instead of hardcoded `#cc2200` |
| Controls | `GrimCheckBox`, `GrimTextField`, `GrimButton` |
| **Accent picker** | Row of 4 color swatches (same presets as desktop) → `vault.setAccentColor()` |
| Lock button | primary `GrimButton`; must navigate back to login |

---

## Phase 4 — Typography and motion

### Fonts

Desktop uses **Consolas** for terminal/hacker elements and **Segoe UI** for general UI.

On Android:

1. Bundle **JetBrains Mono** or **Fira Code** as a lightweight monospace TTF in `mobile/resources/fonts/` (Consolas is not reliably available on Android).
2. Load via `FontLoader` in `Theme.qml`.
3. Use monospace for: login prompt, tab labels, note list titles, empty states.
4. Use system sans (Roboto) for long body text in editor if readability suffers at small sizes.

### Motion (subtle)

- Tab switch: 150ms opacity fade on `StackLayout` (optional)
- Button press: 80ms background color transition
- Avoid bouncy Material ripples — use flat color shifts like desktop hover states

---

## Phase 5 — Brand assets

| Asset | Purpose |
|-------|---------|
| Launcher icon | Adaptive icon from `resources/icon.png` — infernal red on dark |
| Splash screen | `#0a0a0c` + centered logo + `GRIMLEDGER` monospace |
| Status bar | Dark icons on dark background (`Qt.application` / Android theme flags) |

**Files to add:**

- `android/res/mipmap-*/ic_launcher.png` (generated from master icon)
- `android/res/drawable/splash.xml` or Qt splash config

---

## Phase 6 — Polish and parity checklist

Use this checklist before calling aesthetics “done”:

```
[ ] Theme.qml singleton — all colors from tokens, zero magic hex in screens
[ ] Grim* controls used on every screen
[ ] Accent picker in Settings matches desktop presets
[ ] Accent persists and updates UI live
[ ] Login gradient + primary button match desktop feel
[ ] List selection shows accent left bar
[ ] Lock returns to LoginScreen
[ ] Material style disabled
[ ] Monospace font bundled and applied
[ ] Launcher icon + splash on brand
[ ] Tablet: optional two-pane notes layout (list | editor) — same tokens
```

---

## Implementation order (recommended)

| Step | Work | Files touched |
|------|------|---------------|
| 1 | Theme singleton + accent C++ wiring | `Theme.qml`, `GrimVaultController.*`, `mobile.qrc`, `main.cpp` |
| 2 | `GrimButton`, `GrimTextField`, `GrimSectionLabel` | `mobile/qml/controls/*` |
| 3 | Restyle `LoginScreen.qml` | login gradient, components |
| 4 | Restyle `main.qml` tab bar + lock flow | shell |
| 5 | Restyle `NotesScreen`, `CredentialsScreen`, `SettingsScreen` | all screens |
| 6 | `GrimListItem`, `GrimCheckBox`, accent swatches | settings + lists |
| 7 | Fonts + splash + icon | `mobile/resources/`, `android/res/` |

**Estimated focus:** Steps 1–5 deliver the biggest visual jump; 6–7 finish the polish.

---

## File layout (target)

```
mobile/
  ANDROID-AESTHETICS-PLAN.md    ← this document
  qml/
    Theme.qml                   ← singleton tokens
    main.qml
    LoginScreen.qml
    NotesScreen.qml
    CredentialsScreen.qml
    SettingsScreen.qml
    controls/
      GrimButton.qml
      GrimTextField.qml
      GrimTextArea.qml
      GrimListItem.qml
      GrimTabButton.qml
      GrimSectionLabel.qml
      GrimCheckBox.qml
      qmldir
  resources/
    fonts/
      JetBrainsMono-Regular.ttf
```

---

## Reference map: QSS → QML

When implementing a control, open `resources/styles/grimledger_dark.qss` and find the matching `#ObjectName` or widget rule. Examples:

| QSS selector | QML component |
|--------------|---------------|
| `#LoginContent` | `LoginScreen` root gradient |
| `#LoginPrompt` | prompt `Label` |
| `#PasswordField` | `GrimTextField` |
| `#PrimaryButton` | `GrimButton { primary: true }` |
| `QListWidget::item:selected` | `GrimListItem` selected state |
| `#TitleBarTitle` | future in-app header (monospace + letter-spacing) |

Accent substitution in QSS (`@@ACCENT@@`) maps to `Theme.accent` in QML.

---

## What not to do

- **Do not** rewrite UI in Kotlin/Compose — duplicates effort and breaks core sharing.
- **Do not** use default Material theme — it will never match infernal desktop.
- **Do not** hardcode `#cc2200` in screens after Phase 1 — always `Theme.accent`.
- **Do not** block aesthetics work on markdown preview / attachments — those are feature phases, separate from look-and-feel.

---

## Related docs

- Desktop theme: `resources/styles/grimledger_dark.qss`, `src/utils/Theme.cpp`
- Android build: `installer/build-android.ps1` (add `README-android.md` when ready)
- Feature gaps (non-aesthetic): see exploration notes in repo — credentials CRUD, search, backup, etc.

---

## Success criteria

The Android app succeeds aesthetically when:

1. A desktop user opens the mobile app and immediately recognizes **the same product** (colors, typography, button style, login mood).
2. Changing accent in Settings updates the whole app consistently, as on desktop.
3. No stock Android/Material buttons or checkboxes remain visible in primary flows.

Feature parity with desktop is a separate roadmap; this plan covers **look and feel only**.
