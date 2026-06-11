# Sign and install GrimLedger APK on a connected Android device via adb.
param(
    [string]$ApkPath = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$DistDir = Join-Path $RepoRoot "dist\android"
$SignedApk = Join-Path $DistDir "GrimLedger-debug.apk"

function Ensure-AndroidDevPath {
    $Sdk = $env:ANDROID_SDK_ROOT
    if (-not $Sdk) { $Sdk = "$env:LOCALAPPDATA\Android\Sdk" }
    $env:ANDROID_SDK_ROOT = $Sdk

    $PlatformTools = Join-Path $Sdk "platform-tools"
    if (Test-Path $PlatformTools) {
        if ($env:Path -notlike "*$PlatformTools*") {
            $env:Path = "$PlatformTools;$env:Path"
        }
    }

    if (-not $env:JAVA_HOME) {
        $JavaCandidates = @(
            "$env:ProgramFiles\Android\Android Studio\jbr",
            "$env:ProgramFiles\Android\Android Studio\jre"
        )
        foreach ($candidate in $JavaCandidates) {
            if (Test-Path (Join-Path $candidate "bin\java.exe")) {
                $env:JAVA_HOME = $candidate
                break
            }
        }
    }
}

Ensure-AndroidDevPath

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot "build-android.ps1")
}

if (-not $ApkPath) {
    $Candidates = @(
        (Join-Path $DistDir "GrimLedger-unsigned.apk"),
        (Join-Path $RepoRoot "build-android\mobile\android-build\GrimLedgerMobile.apk"),
        (Join-Path $RepoRoot "build-android\mobile\android-build\build\outputs\apk\release\android-build-release-unsigned.apk")
    )
    $ApkPath = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}

if (-not $ApkPath -or -not (Test-Path $ApkPath)) {
    throw "No APK found. Run installer\build-android.ps1 first."
}

$Sdk = $env:ANDROID_SDK_ROOT
$BuildTools = Get-ChildItem (Join-Path $Sdk "build-tools") -Directory | Sort-Object Name -Descending | Select-Object -First 1
if (-not $BuildTools) { throw "Android build-tools not found in SDK." }

$Zipalign = Join-Path $BuildTools.FullName "zipalign.exe"
$ApkSigner = Join-Path $BuildTools.FullName "apksigner.bat"
$Keystore = Join-Path $env:USERPROFILE ".android\debug.keystore"

if (-not (Test-Path $Keystore)) {
    $Keytool = Join-Path $env:JAVA_HOME "bin\keytool.exe"
    New-Item -ItemType Directory -Force -Path (Split-Path $Keystore -Parent) | Out-Null
    & $Keytool -genkeypair -v -keystore $Keystore -storepass android -alias androiddebugkey `
        -keypass android -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US"
}

New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
$Aligned = Join-Path $DistDir "GrimLedger-aligned.apk"
& $Zipalign -f -p 4 $ApkPath $Aligned
& $ApkSigner sign --ks $Keystore --ks-pass pass:android --key-pass pass:android --out $SignedApk $Aligned

$Adb = Join-Path $Sdk "platform-tools\adb.exe"
& $Adb devices
& $Adb install -r $SignedApk

Write-Host ""
Write-Host "Installed: $SignedApk"
Write-Host "Launch: adb shell am start -n org.grimseclabs.grimledger/org.qtproject.qt.android.bindings.QtActivity"
