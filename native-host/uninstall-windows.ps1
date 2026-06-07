param(
    [ValidateSet("Chrome", "Edge", "Both")]
    [string]$Browser = "Both"
)

$ErrorActionPreference = "Stop"

if ($Browser -eq "Chrome" -or $Browser -eq "Both") {
    Remove-Item -Path "HKCU:\Software\Google\Chrome\NativeMessagingHosts\com.grimledger.bridge" -Recurse -ErrorAction SilentlyContinue
}

if ($Browser -eq "Edge" -or $Browser -eq "Both") {
    Remove-Item -Path "HKCU:\Software\Microsoft\Edge\NativeMessagingHosts\com.grimledger.bridge" -Recurse -ErrorAction SilentlyContinue
}

Write-Host "Removed native messaging host registration ($Browser)."
