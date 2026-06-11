# Run adb with Android SDK platform-tools on PATH for this session.
$Sdk = $env:ANDROID_SDK_ROOT
if (-not $Sdk) { $Sdk = "$env:LOCALAPPDATA\Android\Sdk" }

$Adb = Join-Path $Sdk "platform-tools\adb.exe"
if (-not (Test-Path $Adb)) {
    Write-Error "adb not found at $Adb. Install Android SDK Platform-Tools via Android Studio SDK Manager."
}

$PlatformTools = Join-Path $Sdk "platform-tools"
if ($env:Path -notlike "*$PlatformTools*") {
    $env:Path = "$PlatformTools;$env:Path"
}

if ($args.Count -eq 0) {
    & $Adb devices
} else {
    & $Adb @args
}
