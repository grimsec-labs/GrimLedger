param(
    [string]$HostExe = "",
    [string]$ExtensionId = ""
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
$HostExe = (Resolve-Path $HostExe).Path

if (-not $ExtensionId) {
    Write-Host "Extension ID is required."
    Write-Host "Load browser-extension/ as unpacked in Chrome, then copy the ID from chrome://extensions"
    Write-Host "Usage: .\install-windows.ps1 -ExtensionId <your-extension-id>"
    exit 1
}

$manifestTemplate = Join-Path $PSScriptRoot "com.grimledger.bridge.json"
$manifestOut = Join-Path $env:TEMP "com.grimledger.bridge.json"
$origin = "chrome-extension://$ExtensionId/"

(Get-Content $manifestTemplate -Raw)
    .Replace("@GRIMLEDGER_HOST_PATH@", $HostExe.Replace("\", "\\"))
    .Replace("@CHROME_EXTENSION_ORIGIN@", $origin) |
    Set-Content -Path $manifestOut -Encoding UTF8

$regPath = "HKCU:\Software\Google\Chrome\NativeMessagingHosts\com.grimledger.bridge"
New-Item -Path $regPath -Force | Out-Null
Set-ItemProperty -Path $regPath -Name "(default)" -Value $manifestOut

$edgePath = "HKCU:\Software\Microsoft\Edge\NativeMessagingHosts\com.grimledger.bridge"
New-Item -Path $edgePath -Force | Out-Null
Set-ItemProperty -Path $edgePath -Name "(default)" -Value $manifestOut

Write-Host "Installed native messaging host."
Write-Host "  Host: $HostExe"
Write-Host "  Manifest: $manifestOut"
Write-Host "  Extension origin: $origin"
