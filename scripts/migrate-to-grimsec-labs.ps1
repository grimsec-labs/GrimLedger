# Migrate GrimLedger remote from miditool to grimsec-labs/GrimLedger
# Prerequisites:
#   1. Log in as grimsec-labs:  gh auth login
#   2. Create empty repo:       gh repo create grimsec-labs/GrimLedger --public
param(
    [string]$GrimsecEmail = "",
    [switch]$UseSsh,
    [switch]$LocalOnly
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

if (-not $LocalOnly) {
    Write-Host "Checking grimsec-labs/GrimLedger exists..."
    gh repo view grimsec-labs/GrimLedger 2>$null | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Creating grimsec-labs/GrimLedger (requires gh auth as grimsec-labs org owner)..."
        gh repo create grimsec-labs/GrimLedger --public --description "Local-first encrypted markdown vault and password manager"
        if ($LASTEXITCODE -ne 0) {
            throw @"
Could not create grimsec-labs/GrimLedger.
Log in with the grimsec-labs account:
  gh auth login
Then create the repo manually at https://github.com/new (owner: grimsec-labs)
"@
        }
    }
} else {
    Write-Host "LocalOnly: skipping GitHub repo create/view checks."
}

if ($GrimsecEmail) {
    git config user.email $GrimsecEmail
    git config user.name "grimsec-labs"
    Write-Host "Set local git identity for this repo."
}

$remotes = @(git remote)
if ($remotes -contains "origin") {
    $url = git remote get-url origin
    if ($url -match "miditool") {
        git remote rename origin miditool-backup
        Write-Host "Renamed origin -> miditool-backup ($url)"
        $remotes = @(git remote)
    }
}

$originUrl = if ($UseSsh) {
    "git@github-grimsec:grimsec-labs/GrimLedger.git"
} else {
    "https://github.com/grimsec-labs/GrimLedger.git"
}

if ($remotes -contains "origin") {
    git remote set-url origin $originUrl
} else {
    git remote add origin $originUrl
}

Write-Host "Pushing main to grimsec-labs/GrimLedger..."
if ($LocalOnly) {
    Write-Host "LocalOnly: skipping push. Create the repo on GitHub, then run without -LocalOnly."
} else {
    git push -u origin main
    if ($LASTEXITCODE -ne 0) {
        throw "Push failed. Authenticate as grimsec-labs (gh auth login) and ensure the repo exists."
    }
}

Write-Host ""
Write-Host "Done. Remotes:"
git remote -v
Write-Host ""
Write-Host "Optional - archive miditool/GrimLedger (run while logged in as miditool):"
Write-Host "  gh auth switch"
Write-Host "  gh repo edit miditool/GrimLedger --description 'Moved to https://github.com/grimsec-labs/GrimLedger'"
Write-Host "  gh repo archive miditool/GrimLedger --yes"
