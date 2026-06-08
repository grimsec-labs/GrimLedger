# Install GrimLedger (Option 2: bundled unpacked browser extension).
# Run from the staged release folder (same directory as GrimLedger.exe).
param(
    [string]$ExtensionId = "",
    [ValidateSet("Chrome", "Edge", "Both")]
    [string]$Browser = "Both",
    [switch]$SkipNativeHost
)

$ErrorActionPreference = "Stop"

$SourceDir = $PSScriptRoot
$AppExe = Join-Path $SourceDir "GrimLedger.exe"
$HostExe = Join-Path $SourceDir "grimledger_host.exe"
$ExtSrc = Join-Path $SourceDir "browser-extension"

if (-not (Test-Path $AppExe)) {
    Write-Error "GrimLedger.exe not found. Run installer\build-release.ps1 first, then run this script from dist\GrimLedger."
}

$InstallRoot = Join-Path $env:LOCALAPPDATA "GrimLedger"
$AppDir = Join-Path $InstallRoot "app"
$ExtDir = Join-Path $InstallRoot "browser-extension"

Write-Host "Installing GrimLedger to $AppDir ..."
New-Item -ItemType Directory -Path $AppDir -Force | Out-Null
Copy-Item -Path (Join-Path $SourceDir "*") -Destination $AppDir -Recurse -Force -Exclude @("install.ps1", "README.md")

New-Item -ItemType Directory -Path $ExtDir -Force | Out-Null
Copy-Item -Path (Join-Path $ExtSrc "*") -Destination $ExtDir -Recurse -Force

$StartMenu = [Environment]::GetFolderPath("Programs")
$ShortcutDir = Join-Path $StartMenu "GrimLedger"
New-Item -ItemType Directory -Path $ShortcutDir -Force | Out-Null

$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut((Join-Path $ShortcutDir "GrimLedger.lnk"))
$Shortcut.TargetPath = Join-Path $AppDir "GrimLedger.exe"
$Shortcut.WorkingDirectory = $AppDir
$Shortcut.Description = "GrimLedger encrypted vault"
$Shortcut.Save()

Write-Host "Desktop app installed."
Write-Host ""
Write-Host "=== Browser extension (manual load) ==="
Write-Host "1. Open Chrome or Edge and go to: chrome://extensions"
Write-Host "2. Enable Developer mode"
Write-Host "3. Click 'Load unpacked' and select:"
Write-Host "   $ExtDir"
Write-Host "4. Copy the 32-character extension ID from the extensions page"
Write-Host ""

if (-not $SkipNativeHost) {
    if (-not $ExtensionId) {
        $ExtensionId = Read-Host "Paste extension ID (or press Enter to skip native-host setup)"
    }

    if ($ExtensionId -and $ExtensionId -match '^[a-p]{32}$') {
        $NativeScript = Join-Path $AppDir "native-host\install-windows.ps1"
        $InstalledHost = Join-Path $AppDir "grimledger_host.exe"
        & $NativeScript -HostExe $InstalledHost -ExtensionId $ExtensionId -Browser $Browser
    } else {
        Write-Host "Skipping native-host registration. Re-run later:"
        Write-Host "  .\native-host\install-windows.ps1 -ExtensionId YOUR_ID -HostExe `"$AppDir\grimledger_host.exe`""
    }
}

Write-Host ""
Write-Host "5. In GrimLedger: Settings -> Security -> enable browser bridge (off by default)"
Write-Host "6. Unlock GrimLedger and keep it running while using the extension"
Write-Host ""
Write-Host "Launch: $ShortcutDir\GrimLedger.lnk"
