# GrimLedger — Android Launch Crash: Root Cause & Fix

- **Date:** 2026-06-08
- **Branch:** `security-remediation-android-fix`
- **Analysis type:** Static (root-caused from source + build artifacts). Runtime reproduction on-device was **not possible in this environment** — see "Verification status / blockers" below.

---

## Symptom

The Android app builds and installs, but **fatally errors and cannot launch** — the process dies during startup before (or as) the QML UI comes up.

## Root cause (primary) — `UnsatisfiedLinkError` from a wrong `System.loadLibrary`

`android/src/org/grimseclabs/grimledger/GrimLedgerBridge.kt` contained:

```kotlin
object GrimLedgerBridge {
    init {
        System.loadLibrary("GrimLedger")
    }
    @JvmStatic external fun nativeSetVaultController(ptr: Long)
    ...
}
```

The application's main Qt library is **not** `libGrimLedger.so`. The executable target is `GrimLedgerMobile` with `OUTPUT_NAME "GrimLedger"` (`mobile/CMakeLists.txt:23-26`), so `androiddeployqt` packages it as **`libGrimLedger_<abi>.so`** (e.g. `libGrimLedger_arm64-v8a.so` — confirmed present at `build-android/mobile/libGrimLedger_arm64-v8a.so`) and Qt's own loader (`QtLoader`/`QtNative`) loads it by its full, ABI-suffixed name during bootstrap.

`System.loadLibrary("GrimLedger")` asks the JVM for `System.mapLibraryName("GrimLedger")` = **`libGrimLedger.so`** (no ABI suffix). That file does not exist in the APK, so the call throws:

```
java.lang.UnsatisfiedLinkError: dlopen failed: library "libGrimLedger.so" not found
```

**When it fires:** The `init {}` block runs the first time the `GrimLedgerBridge` Kotlin object is touched. That happens inside `AndroidJniBridge_registerController()` (`android/cpp/android_jni_bridge.cpp:50-57`), which calls `QJniObject::callStaticMethod(... "registerNativeController" ...)`. That function is called unconditionally from the **`GrimVaultController` constructor** (`mobile/GrimVaultController.cpp:35-37`), which runs in `main()` at `GrimVaultController vault;` — **before** `engine.load()`. The `UnsatisfiedLinkError` propagates out of the JNI call at startup and aborts the process. Net effect: instant fatal launch failure.

**Why the manual load is unnecessary anyway:** The JNI symbols (`Java_org_grimseclabs_grimledger_GrimLedgerBridge_*`) are compiled *into the main app library* (`android_jni_bridge.cpp` is part of the `GrimLedgerMobile` target — `mobile/CMakeLists.txt:5`). Qt has already loaded that library (it contains `main()` itself) and associated it with the app's class loader, so name-based JNI binding resolves the symbols **without** any `System.loadLibrary` in user code. The explicit load was both wrong-named and redundant.

## Fix applied

`android/src/.../GrimLedgerBridge.kt` — **removed the `init { System.loadLibrary("GrimLedger") }` block** (replaced with an explanatory comment). The native methods resolve via the Qt-loaded application library.

```kotlin
object GrimLedgerBridge {
    // No System.loadLibrary(): the JNI symbols live in the Qt-loaded
    // libGrimLedger_<abi>.so already. A bare loadLibrary("GrimLedger") looks for
    // libGrimLedger.so (no ABI suffix), which doesn't exist -> UnsatisfiedLinkError.
    @JvmStatic external fun nativeSetVaultController(ptr: Long)
    ...
}
```

## Startup hardening applied (defense-in-depth, prompt §2D)

These do not change the root cause but make any *future* startup failure diagnosable instead of a silent death, and remove secondary SIGSEGV risk:

| Change | File | Why |
|--------|------|-----|
| Log + abort message on `sodium_init()` failure | `mobile/main.cpp` | Previously a silent `return 1` |
| Connect `QQmlApplicationEngine::objectCreationFailed` and log the failing URL; log on empty `rootObjects()` | `mobile/main.cpp` | Surfaces missing-QML-module/import errors in Logcat instead of a blank exit |
| Check `Database::open()` return value and `qWarning` on failure | `mobile/GrimVaultController.cpp:33` | The return was ignored; a closed `sqlite3*` would later be dereferenced (SIGSEGV) |
| Replace `font.family: "Consolas"` with `"monospace"` | `mobile/qml/{main,LoginScreen,CredentialsScreen}.qml` | Consolas does not exist on Android (cosmetic fallback, not the crash) |

These diagnostics go to Logcat via Qt's default message handler (tag `Qt`/category) and contain **no secrets**.

## Things ruled out

- **Missing AndroidX biometric/security-crypto classes** (a candidate `NoClassDefFoundError` when the login screen evaluates `vault.biometricSupported`): ruled out — `androidx.biometric:biometric` and `androidx.security:security-crypto` are declared in `android/build.gradle:25-26`.
- **Missing QtQuick.Controls plugins** (candidate empty-rootObjects on `engine.load`): the controls/style plugin `.so`s are present in the build output (added in a prior "Android APK updates & fixes" commit), so the QML imports resolve.
- **`androidContext()` rewrite** to `QtNative.activity()` (`src/security/AndroidBiometricUnlock.cpp`): only reached when biometric is queried, which is *after* the crash point; not the launch blocker. (It is exercised by the `biometricSupported` binding once the bridge crash is fixed; the activity is valid at QML-load time.)

## Interaction with the security work

The launch fix and the GL-SEC-002 biometric fix both touch the Android bridge/biometric layer. They were made together and do **not** conflict: removing the bad `loadLibrary` is independent of the biometric storage change, and the biometric rewrite keeps the same JNI class/method-resolution path (now via the Qt-loaded library).

## Build / install / verify steps (for a maintainer with the full toolchain)

```powershell
# Requires: Android SDK + NDK, a JDK (Android Studio JBR works), Qt 6.11 android_arm64_v8a kit.
$env:ANDROID_SDK_ROOT = "$env:LOCALAPPDATA\Android\Sdk"
$env:JAVA_HOME        = "C:\Program Files\Android\Android Studio\jbr"
.\installer\build-android.ps1            # adjust -QtAndroidDir if needed

# Install + launch on an arm64 device/emulator:
$adb = "$env:LOCALAPPDATA\Android\Sdk\platform-tools\adb.exe"
& $adb install -r dist\android\GrimLedger-debug.apk
& $adb logcat -c
& $adb shell am start -n org.grimseclabs.grimledger/org.qtproject.qt.android.bindings.QtActivity
& $adb logcat -d *:E AndroidRuntime:E libc:F DEBUG:F Qt*:W > android-crash.log
```

**Expected after fix:** no `UnsatisfiedLinkError`/`FATAL` at launch; app reaches `LoginScreen.qml`; create-vault + unlock smoke test works; second launch is clean.

## Verification status / blockers

- **Static root cause: high confidence.** The lib-name mismatch, the call-site ordering (constructor → JNI → class init → `loadLibrary`), and the packaged library name are all confirmed from source and build artifacts.
- **Runtime reproduction: BLOCKED in this environment.** `adb` is not on PATH, no device/emulator was confirmed connected, `ANDROID_SDK_ROOT`/NDK env vars were unset, and there is no JDK on PATH (only Android Studio's bundled JBR exists). A full `androiddeployqt` + Gradle build + on-device launch could not be executed here. The maintainer should run the steps above to confirm the app reaches the login screen and capture a clean `android-crash.log`.

## Remaining Android notes (not launch blockers)

- `android/build.gradle` pins very new toolchain versions (`com.android.tools.build:gradle:9.0.0`, Kotlin `2.3.0`); if the Gradle build itself fails, check AGP/Kotlin/Gradle-wrapper compatibility — that is a *build-time* concern distinct from the runtime crash fixed here.
- The entire `build-android/` tree (including a ~14.5 MB `.so` and many Qt plugin `.so`s) is committed to git. This is a hygiene/supply-chain issue (large binaries in VCS) and should be `.gitignore`d, but it does not affect launch.
