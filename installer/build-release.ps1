# Stage a Windows release folder: GrimLedger + Qt deps + browser extension + native-host tools.
param(
    [string]$BuildDir = "",
    [string]$Config = "Release",
    [string]$OutDir = "",
    [switch]$SetupExe,
    [switch]$StageOnly
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot

function EnsureInstallerIcon {
    param([string]$RepoRoot)
    $IcoPath = Join-Path $RepoRoot "resources\icon.ico"
    if (Test-Path $IcoPath) { return }

    $PngPath = Join-Path $RepoRoot "resources\icon.png"
    if (-not (Test-Path $PngPath)) {
        Write-Host "Note: resources\icon.ico not found — Setup.exe will use the default Inno icon."
        return
    }

    try {
        Add-Type -AssemblyName System.Drawing
        $bitmap = [System.Drawing.Bitmap]::FromFile($PngPath)
        $iconHandle = $bitmap.GetHicon()
        $icon = [System.Drawing.Icon]::FromHandle($iconHandle)
        $fs = [System.IO.File]::Create($IcoPath)
        $icon.Save($fs)
        $fs.Close()
        $icon.Dispose()
        $bitmap.Dispose()
        Write-Host "Generated resources\icon.ico from icon.png"
    } catch {
        Write-Host "Note: could not generate icon.ico — $($_.Exception.Message)"
    }
}

if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot "build"
}
if (-not $OutDir) {
    $OutDir = Join-Path $RepoRoot "dist\GrimLedger"
}

$ExeDir = Join-Path $BuildDir $Config
$AppExe = Join-Path $ExeDir "GrimLedger.exe"
$HostExeSrc = Join-Path $ExeDir "grimledger_host.exe"
if (-not (Test-Path $AppExe)) {
    $AppExe = Join-Path $BuildDir "GrimLedger.exe"
    $HostExeSrc = Join-Path $BuildDir "grimledger_host.exe"
    if (Test-Path $AppExe) { $ExeDir = $BuildDir }
}
if (-not (Test-Path $AppExe)) {
    Write-Host "Building GrimLedger ($Config)..."
    cmake -S $RepoRoot -B $BuildDir -DCMAKE_BUILD_TYPE=$Config
    cmake --build $BuildDir --config $Config
    $AppExe = Join-Path $ExeDir "GrimLedger.exe"
    $HostExeSrc = Join-Path $ExeDir "grimledger_host.exe"
    if (-not (Test-Path $AppExe)) {
        $AppExe = Join-Path $BuildDir "GrimLedger.exe"
        $HostExeSrc = Join-Path $BuildDir "grimledger_host.exe"
        if (Test-Path $AppExe) { $ExeDir = $BuildDir }
    }
}
if (-not (Test-Path $AppExe)) {
    Write-Error "GrimLedger.exe not found after build. Expected under $BuildDir"
}

$WinDeployQt = Get-Command windeployqt -ErrorAction SilentlyContinue
if (-not $WinDeployQt) {
    $QtCandidates = @(
        $env:QT_BIN,
        "C:\Qt\6.11.1\mingw_64\bin",
        "C:\Qt\6.8.0\msvc2019_64\bin"
    ) | Where-Object { $_ }
    foreach ($bin in $QtCandidates) {
        $candidate = Join-Path $bin "windeployqt.exe"
        if (Test-Path $candidate) {
            $WinDeployQt = $candidate
            break
        }
    }
    if (-not $WinDeployQt) {
        $cacheFile = Join-Path $BuildDir "CMakeCache.txt"
        if (Test-Path $cacheFile) {
            $match = Select-String -Path $cacheFile -Pattern 'WINDEPLOYQT_EXECUTABLE:FILEPATH=(.+)' | Select-Object -First 1
            if ($match -and (Test-Path $match.Matches.Groups[1].Value)) {
                $WinDeployQt = $match.Matches.Groups[1].Value
            }
        }
    }
}
if (-not $WinDeployQt) {
    Write-Error "windeployqt not found. Add Qt bin to PATH or set `$env:QT_BIN."
}

if (Test-Path $OutDir) {
    Remove-Item -Recurse -Force $OutDir
}
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null

Copy-Item $AppExe $OutDir
Copy-Item $HostExeSrc (Join-Path $OutDir "grimledger_host.exe")

Write-Host "Running windeployqt..."
$WinDeployQtArgs = @(
    (Join-Path $OutDir "GrimLedger.exe"),
    "--no-translations"
)
if ($WinDeployQt -match "msvc") {
    $WinDeployQtArgs += "--no-compiler-runtime"
}
& $WinDeployQt @WinDeployQtArgs

$ExtSrc = Join-Path $RepoRoot "browser-extension"
$ExtDst = Join-Path $OutDir "browser-extension"
Copy-Item -Recurse $ExtSrc $ExtDst

$NativeDst = Join-Path $OutDir "native-host"
New-Item -ItemType Directory -Path $NativeDst -Force | Out-Null
Copy-Item (Join-Path $RepoRoot "native-host\*") $NativeDst -Recurse

Copy-Item (Join-Path $PSScriptRoot "install.ps1") $OutDir
Copy-Item (Join-Path $PSScriptRoot "README.md") $OutDir

$BuildSetup = $SetupExe -or (-not $StageOnly)
if ($BuildSetup) {
    EnsureInstallerIcon -RepoRoot $RepoRoot
}

Write-Host ""
Write-Host "Release staged at: $OutDir"

if (-not $BuildSetup) {
    Write-Host "Run as the installing user:"
    Write-Host "  cd `"$OutDir`""
    Write-Host "  .\install.ps1"
}

if ($BuildSetup) {
    $Iscc = Get-Command iscc -ErrorAction SilentlyContinue
    if (-not $Iscc) {
        $IsccPath = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
        if (Test-Path $IsccPath) { $Iscc = $IsccPath }
    }
    if (-not $Iscc) {
        Write-Warning "Inno Setup (iscc) not found. Install from https://jrsoftware.org/isinfo.php then run:"
        Write-Warning "  iscc installer\grimledger.iss"
    } else {
        Write-Host "Building Setup.exe..."
        & $Iscc (Join-Path $PSScriptRoot "grimledger.iss")
        $DistDir = Join-Path $RepoRoot "dist"
        $SetupPath = Join-Path $DistDir "GrimLedger-Setup.exe"
        Copy-Item (Join-Path $PSScriptRoot "read-before-install.txt") (Join-Path $DistDir "READ-BEFORE-INSTALL.txt") -Force
        Copy-Item (Join-Path $PSScriptRoot "post-install.txt") (Join-Path $DistDir "AFTER-INSTALL.txt") -Force
        Write-Host "Installer: $SetupPath"
        Write-Host "Instructions: $DistDir\READ-BEFORE-INSTALL.txt"
        Write-Host "              $DistDir\AFTER-INSTALL.txt"
    }
}
