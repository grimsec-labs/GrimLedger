param(
    [string]$HostExe = "",
    [string]$ExtensionId = "",
    [ValidateSet("Chrome", "Edge", "Both")]
    [string]$Browser = "Both"
)

$ErrorActionPreference = "Stop"

if (-not $HostExe) {
    $releaseHost = Join-Path $PSScriptRoot "..\build\Release\grimledger_host.exe"
    $flatHost = Join-Path $PSScriptRoot "..\build\grimledger_host.exe"
    if (Test-Path $releaseHost) {
        $HostExe = $releaseHost
    } else {
        $HostExe = $flatHost
    }
}

if (-not (Test-Path $HostExe)) {
    Write-Host "Native host not found: $HostExe"
    Write-Host "Build GrimLedger first (grimledger_host.exe must exist)."
    exit 1
}
$HostExe = (Resolve-Path $HostExe).Path

if (-not $ExtensionId) {
    $idCandidates = @(
        (Join-Path $PSScriptRoot "extension-id.txt"),
        (Join-Path (Split-Path $PSScriptRoot -Parent) "installer\extension-id.txt")
    )
    foreach ($idFile in $idCandidates) {
        if (Test-Path $idFile) {
            $ExtensionId = (Get-Content $idFile -Raw).Trim()
            break
        }
    }
}

if (-not $ExtensionId -or $ExtensionId -notmatch '^[a-p]{32}$') {
    Write-Host "A valid 32-character Chrome extension ID is required."
    Write-Host "Load browser-extension/ as unpacked in Chrome, then copy the ID from chrome://extensions"
    Write-Host "Usage: .\install-windows.ps1 -ExtensionId <your-extension-id> [-Browser Chrome|Edge|Both]"
    exit 1
}

$installDir = Join-Path $env:LOCALAPPDATA "GrimLedger\native-host"
New-Item -ItemType Directory -Path $installDir -Force | Out-Null

$installedHost = Join-Path $installDir "grimledger_host.exe"
Copy-Item -Path $HostExe -Destination $installedHost -Force

$manifestTemplate = Join-Path $PSScriptRoot "com.grimledger.bridge.json"
$manifestOut = Join-Path $installDir "com.grimledger.bridge.json"
$origin = "chrome-extension://$ExtensionId/"

$manifest = Get-Content $manifestTemplate -Raw
$manifest = $manifest.Replace("@GRIMLEDGER_HOST_PATH@", $installedHost.Replace("\", "\\"))
$manifest = $manifest.Replace("@CHROME_EXTENSION_ORIGIN@", $origin)
[System.IO.File]::WriteAllText($manifestOut, $manifest, (New-Object System.Text.UTF8Encoding($false)))

if ($Browser -eq "Chrome" -or $Browser -eq "Both") {
    $regPath = "HKCU:\Software\Google\Chrome\NativeMessagingHosts\com.grimledger.bridge"
    New-Item -Path $regPath -Force | Out-Null
    Set-ItemProperty -Path $regPath -Name "(default)" -Value $manifestOut
}

if ($Browser -eq "Edge" -or $Browser -eq "Both") {
    $edgePath = "HKCU:\Software\Microsoft\Edge\NativeMessagingHosts\com.grimledger.bridge"
    New-Item -Path $edgePath -Force | Out-Null
    Set-ItemProperty -Path $edgePath -Name "(default)" -Value $manifestOut
}

Write-Host "Installed native messaging host."
Write-Host "  Host: $installedHost"
Write-Host "  Manifest: $manifestOut"
Write-Host "  Extension origin: $origin"
Write-Host "  Browser registration: $Browser"
