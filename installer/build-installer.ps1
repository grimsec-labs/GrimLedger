# One-command Windows release: stage app + build GrimLedger-Setup.exe
# Requires: Qt windeployqt, Inno Setup 6 (https://jrsoftware.org/isinfo.php)
param(
    [string]$BuildDir = "",
    [string]$Config = "Release"
)

& (Join-Path $PSScriptRoot "build-release.ps1") -BuildDir $BuildDir -Config $Config -SetupExe
