# Package browser-extension/ as a ZIP for Chrome Web Store upload.
param(
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$ExtDir = Join-Path $RepoRoot "browser-extension"
$ManifestPath = Join-Path $ExtDir "manifest.json"

if (-not (Test-Path $ManifestPath)) {
    Write-Error "manifest.json not found at $ManifestPath"
}

$manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
$version = $manifest.version

if (-not $OutDir) {
    $OutDir = Join-Path $RepoRoot "dist"
}
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null

$zipName = "grimledger-bridge-$version.zip"
$zipPath = Join-Path $OutDir $zipName

$staging = Join-Path $env:TEMP "grimledger-ext-stage-$([Guid]::NewGuid().ToString('N'))"
New-Item -ItemType Directory -Path $staging -Force | Out-Null
try {
    Get-ChildItem -Path $ExtDir -File | Where-Object {
        $_.Name -notmatch '\.(md)$' -and $_.Name -notlike '.extension-*'
    } | ForEach-Object {
        Copy-Item $_.FullName (Join-Path $staging $_.Name)
    }
    if (Test-Path $zipPath) {
        Remove-Item $zipPath -Force
    }
    Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $zipPath
} finally {
    Remove-Item -Recurse -Force $staging -ErrorAction SilentlyContinue
}

Write-Host "Packaged extension for Chrome Web Store upload:"
Write-Host "  $zipPath"
Write-Host ""

$idFile = Join-Path $PSScriptRoot "extension-id.txt"
if (Test-Path $idFile) {
    Write-Host "Extension ID:"
    Write-Host "  $((Get-Content $idFile -Raw).Trim())"
}
