# Build GrimLedger Android APK/AAB (requires Qt Android kit + SDK/NDK).
param(
    [string]$BuildDir = "",
    [string]$QtAndroidDir = "",
    [string]$AndroidAbi = "arm64-v8a"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot

if (-not $BuildDir) { $BuildDir = Join-Path $RepoRoot "build-android" }
if (-not $QtAndroidDir) {
    $cache = Join-Path $RepoRoot "build-android\CMakeCache.txt"
    if (Test-Path $cache) {
        $m = Select-String -Path $cache -Pattern 'Qt6_DIR:PATH=(.+)' | Select-Object -First 1
        if ($m -and $m.Matches.Groups[1].Value -match "android") {
            $QtAndroidDir = Split-Path (Split-Path $m.Matches.Groups[1].Value -Parent) -Parent
        }
    }
}
if (-not $QtAndroidDir) { $QtAndroidDir = "C:\Qt\6.11.1\android_arm64_v8a" }

$Sdk = $env:ANDROID_SDK_ROOT
if (-not $Sdk) { $Sdk = "$env:LOCALAPPDATA\Android\Sdk" }

$Ndk = Join-Path $QtAndroidDir "..\android-ndk-r27b"
if (-not (Test-Path $Ndk)) {
    $sdkNdkRoot = Join-Path $Sdk "ndk"
    if (Test-Path $sdkNdkRoot) {
        $Ndk = (Get-ChildItem $sdkNdkRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1).FullName
    }
}
if (-not (Test-Path $Ndk)) {
    throw "Android NDK not found. Install via Android Studio SDK Manager or: sdkmanager --install `"ndk;27.2.12479018`""
}
Write-Host "Using NDK: $Ndk"
Write-Host "Using SDK: $Sdk"

$env:ANDROID_SDK_ROOT = $Sdk
$env:ANDROID_NDK_ROOT = $Ndk

$Ninja = "C:\Qt\Tools\Ninja\ninja.exe"
if (-not (Test-Path $Ninja)) { throw "Ninja not found at $Ninja" }

Write-Host "Configuring Android build..."
cmake -S $RepoRoot -B $BuildDir -G Ninja `
    -DCMAKE_MAKE_PROGRAM="$Ninja" `
    -DCMAKE_TOOLCHAIN_FILE="$QtAndroidDir/lib/cmake/Qt6/qt.toolchain.cmake" `
    -DQt6_DIR="$QtAndroidDir/lib/cmake/Qt6" `
    -DANDROID_ABI="$AndroidAbi" `
    -DANDROID_SDK_ROOT="$Sdk" `
    -DANDROID_NDK_ROOT="$Ndk" `
    -DCMAKE_BUILD_TYPE=Release

cmake --build $BuildDir --config Release

$DeployQt = Join-Path $QtAndroidDir "bin\androiddeployqt.exe"
if (-not (Test-Path $DeployQt)) {
    $DeployQt = Join-Path (Split-Path $QtAndroidDir -Parent) "mingw_64\bin\androiddeployqt.exe"
}

Write-Host "Packaging APK..."
$DeployInput = Join-Path $BuildDir "mobile\android-GrimLedgerMobile-deployment-settings.json"
$DeployOutput = Join-Path $RepoRoot "dist\android"
New-Item -ItemType Directory -Force -Path $DeployOutput | Out-Null

if (-not (Test-Path $DeployInput)) {
    throw "Deployment settings not found: $DeployInput (CMake APK step may have already built it)"
}

$ApkCandidates = @(
    (Join-Path $BuildDir "mobile\android-build\GrimLedgerMobile.apk"),
    (Join-Path $BuildDir "mobile\android-build\build\outputs\apk\release\android-build-release-unsigned.apk")
)
$ApkFromCmake = $ApkCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($ApkFromCmake) {
    Copy-Item $ApkFromCmake (Join-Path $DeployOutput "GrimLedger-unsigned.apk") -Force
    Write-Host "APK: $DeployOutput\GrimLedger-unsigned.apk"
} elseif (Test-Path $DeployInput) {
    & $DeployQt --input $DeployInput `
        --output $DeployOutput `
        --android-platform android-34 `
        --release
}

Write-Host "Output: $RepoRoot\dist\android"
