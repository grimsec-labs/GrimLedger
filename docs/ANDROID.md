# GrimLedger on Android

Mobile build uses **Qt Quick (QML)** + shared **grimledger_core** library.

## Prerequisites

- Qt 6 Android kit (e.g. `android_arm64_v8a`) via Qt Maintenance Tool
- Android SDK + NDK (Qt installer or Android Studio)
- Set `ANDROID_SDK_ROOT` if not default

## Build APK

```powershell
.\installer\build-android.ps1
```

Output: `dist\android\`

## Features

| Desktop | Android |
|---------|---------|
| Qt Widgets UI | QML mobile UI (`mobile/qml/`) |
| Chrome extension bridge | `GrimLedgerAutofillService` (Autofill Framework) |
| Windows Hello | `GrimLedgerBiometricUnlock` (Keystore + EncryptedSharedPreferences) |
| Inno Setup installer | APK via `androiddeployqt` |

## Autofill setup

1. Install and unlock GrimLedger.
2. Android Settings → Passwords & autofill → GrimLedger → enable.
3. Add vault keys with matching URLs in the app.

## Project layout

```text
core/           Portable crypto + storage (grimledger_core)
mobile/         QML UI + GrimVaultController
android/        Manifest, Kotlin autofill/biometric, JNI bridge
```
