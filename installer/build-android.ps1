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
    $cache = Join-Path $RepoRoot "build\CMakeCache.txt"
    if (Test-Path $cache) {
        $m = Select-String -Path $cache -Pattern 'Qt6_DIR:PATH=(.+)' | Select-Object -First 1
        if ($m) { $QtAndroidDir = Split-Path (Split-Path $m.Matches.Groups[1].Value -Parent) -Parent }
    }
}
if (-not $QtAndroidDir) { $QtAndroidDir = "C:\Qt\6.11.1\android_arm64_v8a" }

$Ndk = Join-Path $QtAndroidDir "..\android-ndk-r27b"
$Sdk = $env:ANDROID_SDK_ROOT
if (-not $Sdk) { $Sdk = "$env:LOCALAPPDATA\Android\Sdk" }

Write-Host "Configuring Android build..."
cmake -S $RepoRoot -B $BuildDir `
    -DCMAKE_TOOLCHAIN_FILE="$QtAndroidDir\lib\cmake\Qt6\qt.toolchain.cmake" `
    -DANDROID_ABI=$AndroidAbi `
    -DANDROID_SDK_ROOT=$Sdk `
    -DANDROID_NDK_ROOT=$Ndk `
    -DCMAKE_BUILD_TYPE=Release

cmake --build $BuildDir --config Release

$DeployQt = Join-Path $QtAndroidDir "bin\androiddeployqt.exe"
if (-not (Test-Path $DeployQt)) {
    $DeployQt = Join-Path (Split-Path $QtAndroidDir -Parent) "mingw_64\bin\androiddeployqt.exe"
}

Write-Host "Packaging APK..."
& $DeployQt --input "$BuildDir\android-GrimLedgerMobile-deployment-settings.json" `
    --output "$RepoRoot\dist\android" `
    --android-platform android-34 `
    --release

Write-Host "Output: $RepoRoot\dist\android"
