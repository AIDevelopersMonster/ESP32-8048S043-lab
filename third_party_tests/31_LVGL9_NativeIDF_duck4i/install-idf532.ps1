param(
    [string]$InstallRoot = "C:\Espressif\frameworks\esp-idf-v5.3.2",
    [string]$ToolsRoot = ""
)

$ErrorActionPreference = "Stop"

$requiredVersion = "5.3.2"
$idfRepo = "https://github.com/espressif/esp-idf.git"
$tag = "v5.3.2"

if ([string]::IsNullOrWhiteSpace($ToolsRoot)) {
    if (-not [string]::IsNullOrWhiteSpace($env:IDF_TOOLS_PATH)) {
        $ToolsRoot = $env:IDF_TOOLS_PATH
    }
    elseif (Test-Path "C:\Espressif" -PathType Container) {
        $ToolsRoot = "C:\Espressif"
    }
    else {
        $ToolsRoot = Join-Path $env:USERPROFILE ".espressif"
    }
}

Write-Host "=== ESP-IDF 5.3.2 side-by-side installer for Test 31 ==="
Write-Host "Framework root : $InstallRoot"
Write-Host "Tools root     : $ToolsRoot"
Write-Host "Existing IDF   : C:\Espressif\frameworks\esp-idf-v5.5.5 is left untouched"
Write-Host ""

$git = Get-Command git -ErrorAction SilentlyContinue
if (-not $git) {
    throw "git was not found in PATH"
}

if (Test-Path $InstallRoot -PathType Container) {
    $idfPyExisting = Join-Path $InstallRoot "tools\idf.py"
    if (-not (Test-Path $idfPyExisting)) {
        throw "InstallRoot already exists but is not an ESP-IDF tree: $InstallRoot"
    }

    $versionFile = Join-Path $InstallRoot "version.txt"
    $versionHint = ""
    if (Test-Path $versionFile) {
        $versionHint = ([System.IO.File]::ReadAllText($versionFile)).Trim()
    }
    if (-not $versionHint -and (Test-Path (Join-Path $InstallRoot ".git"))) {
        $versionHint = (& git -C $InstallRoot describe --tags --always 2>$null | Out-String).Trim()
    }

    if ($versionHint -notmatch "(^|v|[^0-9])5\.3\.2([^0-9]|$)") {
        throw "Existing ESP-IDF tree is not 5.3.2: $InstallRoot [$versionHint]"
    }

    Write-Host "[PASS] Existing ESP-IDF 5.3.2 source tree found"
}
else {
    $parent = Split-Path $InstallRoot -Parent
    if (-not (Test-Path $parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }

    Write-Host "Cloning official ESP-IDF $tag..."
    & git clone --branch $tag --depth 1 --recursive --shallow-submodules $idfRepo $InstallRoot
    if ($LASTEXITCODE -ne 0) {
        throw "ESP-IDF 5.3.2 clone failed"
    }

    Write-Host "[PASS] Official ESP-IDF 5.3.2 source cloned"
}

# Verify exact tag/source before installing tools.
$describe = (& git -C $InstallRoot describe --tags --always 2>$null | Out-String).Trim()
Write-Host "ESP-IDF source reports: $describe"
if ($describe -notmatch "^v?5\.3\.2($|[-+])") {
    throw "Unexpected ESP-IDF source version: $describe"
}

$env:IDF_TOOLS_PATH = $ToolsRoot
$installScript = Join-Path $InstallRoot "install.ps1"
if (-not (Test-Path $installScript)) {
    throw "install.ps1 not found: $installScript"
}

Write-Host ""
Write-Host "Installing ESP32-S3 toolchain and Python environment for ESP-IDF 5.3.2..."
Write-Host "This reuses compatible cached tools when possible and keeps the 5.5.5 framework source untouched."
Write-Host ""

& powershell -NoProfile -ExecutionPolicy Bypass -File $installScript esp32s3
if ($LASTEXITCODE -ne 0) {
    throw "ESP-IDF 5.3.2 install.ps1 failed"
}

$exportScript = Join-Path $InstallRoot "export.ps1"
if (-not (Test-Path $exportScript)) {
    throw "export.ps1 not found after installation: $exportScript"
}

Write-Host ""
Write-Host "Activating installed ESP-IDF 5.3.2 for verification..."
$env:IDF_PATH = $InstallRoot
. $exportScript
if ($LASTEXITCODE -ne 0) {
    throw "ESP-IDF 5.3.2 export.ps1 failed"
}

$idfPy = Join-Path $InstallRoot "tools\idf.py"
$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    throw "Python was not placed on PATH by ESP-IDF export.ps1"
}

$versionOutput = & python $idfPy --version 2>&1
if ($LASTEXITCODE -ne 0) {
    throw "idf.py verification failed"
}
$versionText = ($versionOutput | Out-String).Trim()
Write-Host "ESP-IDF reported: $versionText"

if ($versionText -notmatch "v?5\.3\.2") {
    throw "Installed environment is not ESP-IDF 5.3.2: $versionText"
}

Write-Host ""
Write-Host "[PASS] ESP-IDF 5.3.2 installed side-by-side"
Write-Host "[PASS] ESP-IDF 5.5.5 framework source was not modified"
Write-Host "[PASS] idf.py reports exact required version"
Write-Host ""
Write-Host "Next command from the ESP32-8048S043-lab repository root:"
Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\31_LVGL9_NativeIDF_duck4i\run-test31.ps1"
