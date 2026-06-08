# GrimLedger — Security Remediation

- **Date:** 2026-06-08
- **Branch:** `security-remediation-android-fix`
- **Baseline audit:** `docs/Opus-4-8-SECURITY_AUDIT.md` (findings GL-SEC-001 … GL-SEC-014)
- **Build/test status:** Desktop `grimledger_core` + test suite built and run with the MinGW (Qt 6.11.1 `mingw_64`) kit — see "Verification" at the end. Android Kotlin/JNI changes are **compile-unverified in this environment** (no NDK/SDK build run, no device); they are static, pattern-matched changes.

> Crypto invariants preserved: no change to Argon2id, XChaCha20-Poly1305, `crypto_secretbox` legacy, or the domain-bound AAD scheme. Changes are minimal and localized.

---

## Status summary

| ID | Severity | Status | One-line change |
|----|----------|--------|-----------------|
| GL-SEC-001 | High | **Fixed (Path B: honest rename)** | "Windows Hello" UI/text → "Quick Unlock (Windows account)"; documents DPAPI, not biometric |
| GL-SEC-002 | High | **Fixed** | Vault key no longer stored in plaintext `QSettings`; Keystore-only; plaintext fallback removed |
| GL-SEC-003 | High | **Fixed** | Autofill trusts `webDomain` only from an allowlisted browser; else keys off verified package |
| GL-SEC-004 | Medium | **Fixed** | Token/endpoint files 0600 on all platforms; local socket restricted to current user |
| GL-SEC-005 | Medium | **Fixed** | Failed-unlock/self-destruct counter persisted in `QSettings` (survives restart) |
| GL-SEC-006 | Medium | **Fixed (best-effort) + documented** | `clearQString` now overwrites its buffer; limitations documented |
| GL-SEC-007 | Low | **Deferred (rationale below)** | Attachment metadata still plaintext — needs schema migration |
| GL-SEC-008 | Low | **Fixed** | Copied secrets marked to skip Windows Clipboard History / cloud clipboard |
| GL-SEC-009 | Low | **Fixed** | libsodium pinned to immutable commit SHA |
| GL-SEC-010 | Low | **Fixed** | TOTP modulus corrected to `10^digits` (RFC 4226/6238) |
| GL-SEC-011 | Low | **Fixed** | HIBP request sends `Add-Padding: true`; padded (count 0) entries ignored |
| GL-SEC-012 | Low | **Fixed** | JNI `GetStringUTFChars`/string args null-checked at the autofill bridge |
| GL-SEC-013 | Low | **Fixed** | Master-password minimum raised 8 → 12 (creation/change only; existing vaults still open) |
| GL-SEC-014 | Informational | **Fixed** | POSIX hardlink identity check (`st_dev`/`st_ino`) added |

---

## Detailed changes

### GL-SEC-001 — Windows "Hello" is DPAPI, not biometric → honest rename (Phase 1)
**Status: Fixed (Path B).** A real Windows Hello implementation (`KeyCredentialManager`/NGC) was *not* attempted in this pass because it cannot be built or tested in this environment; instead the feature is renamed to stop misrepresenting the protection.
- User-facing strings changed from "Windows Hello" → **"Quick Unlock (Windows account)"**:
  - `src/ui/LoginWindow.cpp` (login button)
  - `src/ui/SettingsWindow.cpp` (enable button + reset-confirmation text)
  - `src/ui/MainWindow.cpp` (`onEnableHelloUnlock`/`onDisableHelloUnlock` dialog titles & bodies). The enable dialog now **explicitly states** it uses DPAPI/Windows-account protection, requires no biometric gesture, and that anyone signed into the Windows account can unlock without the master password.
  - `src/App.cpp` (stale-unlock error message).
- `src/security/WindowsHelloUnlock.h` — added a header note documenting that this module is DPAPI-based, not Hello/biometric, and pointing to `KeyCredentialManager` as the real-Hello follow-up.
- Internal symbol names (`onEnableHelloUnlock`, `helloUnlockRequested`, `WindowsHelloUnlock` namespace) were intentionally **left unchanged** to keep the diff minimal (not user-visible).
- **Docs:** `SECURITY.md` updated (see below).
- **Follow-up (recommended):** implement genuine biometric gating via WinRT `KeyCredentialManager` so a gesture is required and the wrapping key is non-exportable.

### GL-SEC-002 — Android biometric: raw key in plaintext QSettings → Keystore-only (Phase 1)
**Status: Fixed.**
- `android/src/.../GrimLedgerBiometricUnlock.kt` rewritten:
  - `storeVaultKey()` writes the key only into Keystore-backed `EncryptedSharedPreferences` and returns a **boolean** (never the key).
  - `loadVaultKey(context)` reads from ESP only — **no plaintext default-value fallback** (was `getString(KEY_WRAPPED, wrapped)`).
  - Keeps `setUserAuthenticationRequired(true, 30)`, so the OS enforces recent user authentication (device credential / biometric) at the Keystore boundary before the key can be decrypted.
- `src/security/AndroidBiometricUnlock.cpp`:
  - `enable()` calls `storeVaultKey` (sig `(Landroid/content/Context;[B)Z`) and stores **only a non-secret boolean marker** (`security/androidBiometricEnabled`) in `QSettings` — the wrapped key is no longer mirrored into `QSettings`. The transient JNI key byte array is overwritten before release.
  - `isConfigured()` checks the boolean marker.
  - `tryUnlock()` calls `loadVaultKey` (sig `(Landroid/content/Context;)[B`), with JNI null-checks on the returned array/elements.
  - `disable()` removes the marker and clears ESP.
- **Residual / follow-up (documented, not a regression):** unlock still relies on the Keystore 30-second auth-validity window rather than an explicit per-unlock `BiometricPrompt` + `CryptoObject`. Implementing that async prompt flow requires Activity/UI-thread coordination and could not be built/tested here; recommended as the next enhancement. The **High-severity plaintext-key exposure is eliminated**.

### GL-SEC-003 — Android autofill trusts spoofable `webDomain` → browser allowlist (Phase 1)
**Status: Fixed.** `android/src/.../GrimLedgerAutofillService.kt`:
- Resolves the requesting package via `structure.activityComponent?.packageName`.
- New `resolveLookupOrigin()`: a `webDomain` is honored **only** when the requesting package is in a `TRUSTED_BROWSERS` allowlist (Chrome/Firefox/Edge/Brave/Opera/Samsung/DuckDuckGo/Vivaldi/Kiwi/AOSP, etc.); otherwise the lookup is keyed off the **verified package name** (`app://<package>`), and unknown callers fail closed (offer nothing).
- **Tradeoff (documented):** because `OriginMatcher` only matches `http(s)` origins, the `app://<package>` path matches no stored credentials today — i.e. **non-browser native-app autofill is effectively disabled (fail-closed)**, while allowlisted-browser autofill continues to work. This is the safe direction; native-app autofill via Digital Asset Links can be added later.

### GL-SEC-004 — bridge token/socket permissions (Phase 2)
**Status: Fixed.**
- `src/bridge/BridgeAuth.cpp` — the owner-only `setPermissions(ReadOwner|WriteOwner)` on the token and endpoint files is now applied on **all** platforms (was `#if defined(Q_OS_WIN)` only → POSIX 0600).
- `src/bridge/CredentialBridgeServer.cpp` — `m_server->setSocketOptions(QLocalServer::UserAccessOption)` before `listen()`, restricting the local socket to the current user.

### GL-SEC-005 — self-destruct counter persistence (Phase 2)
**Status: Fixed.** `src/utils/AppSettings.cpp` — the failed-unlock counter is now stored in `QSettings` (`security/failedUnlockAttempts`, synced on write) instead of a process-lifetime static, so restarting the app no longer resets it and the opt-in self-destruct cannot be bypassed by relaunching. **Hardening follow-up:** pre-increment before showing the verification result so a kill mid-attempt cannot skip the increment.

### GL-SEC-006 — secret zeroization / misleading `SecureStringUtils` (Phase 2)
**Status: Fixed (best-effort) + documented.** `src/utils/SecureStringUtils.cpp` — `clearQString` now `fill('\0')`s the string's buffer (detaching and overwriting this instance) before clearing, instead of the prior no-op assignment. The header/impl comments document the residual limits (implicit sharing reaches no prior copies; freed pages may persist) and steer sensitive material toward `QByteArray`/`sodium_*` buffers.

### GL-SEC-007 — cleartext attachment/log metadata (Phase 3) — **DEFERRED**
**Status: Deferred (with rationale).** Encrypting `note_attachments.original_name`/`source_url`/`acquisition_note`/`mime_type` (and `security_chronicle.event_type`) requires a **schema/data migration** of existing vaults (re-encrypt-in-place under per-record AAD, with a forward/backward-compatible read path), mirroring `VaultRepository::migrateDomainBoundCrypto`. That is a non-trivial, stateful change that **cannot be safely validated without a build+migration test on real vault data**, which is not possible in this environment. Implementing it blind risks vault corruption — the opposite of the goal. **Interim:** `SECURITY.md` is updated to disclose these specific plaintext columns accurately (see below). Recommended as a scoped follow-up with a dedicated migration + round-trip test.

### GL-SEC-008 — clipboard history/cloud exclusion (Phase 3)
**Status: Fixed.** `src/utils/ClipboardUtils.cpp` — copies now go through a `QMimeData` that also sets the Windows markers `ExcludeClipboardContentFromMonitorProcessing`, `CanIncludeInClipboardHistory`, and `CanUploadToCloudClipboard`, so secrets are kept out of Clipboard History (Win+V) and cloud clipboard, in addition to the existing timed clear. The markers are inert on other platforms.

### GL-SEC-009 — pin libsodium (Phase 3)
**Status: Fixed.** `CMakeLists.txt` — libsodium `GIT_TAG` changed from the mutable `1.0.20-RELEASE` tag to its immutable commit SHA `9511c982fb1d046470a8b42aa36556cdb7da15de` (verified against the existing FetchContent checkout). SQLite was already SHA-256 pinned.

### GL-SEC-010 — TOTP modulus off-by-one (Phase 3)
**Status: Fixed.** `src/utils/TotpGenerator.cpp` — divisor loop corrected from `10^(digits-1)` to `10^digits`, so codes use the full digit space per RFC 4226/6238. (`tests/test_totp_generator.cpp` asserts length 6 + determinism only, so it still passes; RFC Appendix-B vectors are recommended as an added test.)

### GL-SEC-011 — HIBP padding (Phase 3)
**Status: Fixed.** `src/security/BreachCheck.cpp` — adds `Add-Padding: true` request header and ignores padded entries (count 0) when matching, closing the response-size side channel. (The blocking UI-thread event loop is left as-is; recommended to move off-thread later.)

### GL-SEC-012 — JNI robustness (Phase 3)
**Status: Fixed.** `android/cpp/android_jni_bridge.cpp` — null-checks the incoming `jstring` args and the `GetStringUTFChars` return values in `nativeCredentialsForOrigin` and `nativeFillCredential`, releasing partial acquisitions correctly. (The GrimShare KDF-param parse note from the audit is unrelated to the bridge and left for a separate change.)

### GL-SEC-013 — master-password minimum (Phase 3)
**Status: Fixed.** `src/security/PasswordManager.cpp` — `isValidVaultPassword` minimum raised 8 → 12. This gates **vault creation and password change only**; `unlockVault` does not call it, so existing shorter-password vaults still open.

### GL-SEC-014 — POSIX hardlink check (Phase 3)
**Status: Fixed.** `src/utils/PathSafety.h` — `sharesHardLink` now has a POSIX implementation (`stat` + `st_dev`/`st_ino` comparison) instead of returning `false`, matching the Windows behavior so a hardlinked backup target onto the live vault is detected on Linux/macOS.

---

## Fresh security pass (issues noted beyond the baseline audit)

These were observed during remediation. None are introduced by the fixes.

1. **Committed build artifacts / large binaries in VCS (hygiene + supply chain).** The recent Android commits checked the entire `build-android/` tree into git (≈312k inserted lines, including a ~14.5 MB `libGrimLedger_arm64-v8a.so` and many Qt plugin `.so`s), and `dist/` contains committed `.exe`/`.apk` binaries. These bloat the repo and create a supply-chain/trust problem (binaries that don't provably correspond to the source). **Recommendation:** add `build*/` and `dist/` to `.gitignore` and remove the tracked artifacts; publish release binaries via CI artifacts/releases instead. *(Not auto-removed here to avoid a massive, review-unfriendly deletion commit without maintainer sign-off.)*
2. **Bleeding-edge Android toolchain pins.** `android/build.gradle` uses `com.android.tools.build:gradle:9.0.0` and Kotlin `2.3.0`, and `androidx.*` alpha versions. These can cause brittle/unreproducible Gradle builds. **Recommendation:** pin to current stable AGP/Kotlin and non-alpha AndroidX where possible.
3. **`list_matches` username/label enumeration over the bridge** (from the audit, GL-SEC-004 context): returns labels/usernames for any asserted origin without per-request confirmation. The permission/socket hardening reduces who can reach it; consider also minimizing returned fields. Left as a design note.
4. **Android autofill native-app support gap** (introduced tradeoff, see GL-SEC-003): native (non-browser) app autofill is now fail-closed. If desired later, implement Digital Asset Links–verified package→domain mapping rather than trusting `webDomain`.

No instances of secrets being written to logs (`qDebug`/Logcat) were found; the diagnostics added to `mobile/main.cpp` and `GrimVaultController.cpp` log only non-secret status strings.

---

## Verification

**Desktop (performed — PASS):**
- Toolchain: Qt 6.11.1 `mingw_64` + MinGW 13.1.0 + Ninja/CMake (the project's desktop kit). `ANDROID`/MSVC kits not required.
- A **clean from-scratch configure + build** (`build-verify/`, fresh FetchContent) compiled libsodium, SQLite, `grimledger_core`, the full `GrimLedger` desktop app, the native host, and all test targets with no errors. This covers every changed desktop C++ file: `AppSettings`, `BreachCheck`, `BridgeAuth`, `CredentialBridgeServer`, `PasswordManager`, `TotpGenerator`, `SecureStringUtils`, `PathSafety`, `ClipboardUtils`, `LoginWindow`, `MainWindow`, `SettingsWindow`, `App`, and `CryptoManager`.
- The clean fetch also exercised the **GL-SEC-009 libsodium SHA pin** (`GIT_TAG 9511c982...` + `GIT_SHALLOW`): it cloned and built cleanly, confirming the pin does not break fresh builds.
- `ctest` — **9/9 passed (100%)**: `test_origin_matcher`, `test_sqlite_utils`, `test_bridge_auth`, `test_bridge_server`, `test_credential_repository`, `test_note_repository`, `test_vault_restore`, `test_totp_generator`, `test_stage_b`. (The TOTP modulus fix keeps `test_totp_generator` green; the bridge permission/socket changes keep `test_bridge_auth`/`test_bridge_server` green.)
- Note: the pre-existing `build/` tree fails to reconfigure because it was created when the repo lived at `d:/Dev/repos/GrimLedger` (since moved to `grimsec_labs/GrimLedger`); its cached absolute paths are stale. This is unrelated to the changes — a clean build dir works. The maintainer should delete/re-create `build/`.

**Android (blocked — static only):**
- The Kotlin (`GrimLedgerBiometricUnlock.kt`, `GrimLedgerAutofillService.kt`, `GrimLedgerBridge.kt`) and JNI changes were **not** compiled or run here: no NDK build was executed, `adb` is not on PATH, and no device/emulator was confirmed. They are careful, pattern-matched edits but should be compiled (`installer/build-android.ps1`) and exercised on-device before release. See `docs/ANDROID_LAUNCH_FIX.md` for the build/install/verify recipe.

**Tests added/updated:** none added in this pass (time/build constraints). **Recommended test additions:** RFC 6238 vectors for TOTP (GL-SEC-010); a persistence test for the failed-attempt counter (GL-SEC-005); an `AndroidBiometricUnlock`/`QSettings` assertion that no key material is written outside Keystore (GL-SEC-002); an autofill unit test for `resolveLookupOrigin` (GL-SEC-003).

---

## Deferred / not done (explicit)

- **GL-SEC-007** attachment/chronicle metadata encryption — deferred (needs migration + round-trip test on real vault data).
- **GL-SEC-001 real Windows Hello** (KeyCredentialManager/NGC) — only the honest rename (Path B) was done.
- **GL-SEC-002 explicit BiometricPrompt + CryptoObject** — Keystore-only storage done; per-unlock biometric prompt deferred (async UI flow, untestable here).
- **Removal of committed `build-android/`/`dist/` artifacts** — flagged, not executed (large destructive change needs maintainer sign-off).
