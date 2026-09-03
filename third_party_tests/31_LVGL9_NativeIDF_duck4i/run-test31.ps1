param(
    [switch]$Flash,
    [string]$Port = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$workRoot = Join-Path $repoRoot ".third-party-work\31_LVGL9_NativeIDF_duck4i"
$upstreamDir = Join-Path $workRoot "upstream"
$projectDir = Join-Path $upstreamDir "st7262"
$prep = Join-Path $scriptDir "prepare-test31.ps1"

if (-not (Test-Path $upstreamDir)) {
    & powershell -ExecutionPolicy Bypass -File $prep
    if ($LASTEXITCODE -ne 0) { throw "prepare-test31.ps1 failed" }
}

function Resolve-IdfPy {
    $cmd = Get-Command idf.py -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $candidates = @(
        (Join-Path $env:USERPROFILE "esp\esp-idf\tools\idf.py"),
        (Join-Path $env:USERPROFILE "esp\v5.3.2\esp-idf\tools\idf.py"),
        (Join-Path $env:USERPROFILE "esp\esp-idf-v5.3.2\tools\idf.py")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) { return (Resolve-Path $candidate).Path }
    }

    return $null
}

$idfPy = Resolve-IdfPy
if (-not $idfPy) {
    Write-Host "ESP-IDF idf.py was not found in PATH or common Windows locations."
    Write-Host ""
    Write-Host "Open an ESP-IDF 5.3.2 PowerShell/Command Prompt, then rerun this script."
    Write-Host "If ESP-IDF is installed through Espressif IDF Tools, launch the v5.3.2 environment first."
    throw "ESP-IDF environment not active"
}

Write-Host "[PASS] idf.py: $idfPy"

Push-Location $projectDir
try {
    $versionOutput = & python $idfPy --version 2>&1
    if ($LASTEXITCODE -ne 0) { throw "idf.py failed to start" }
    $versionText = ($versionOutput | Out-String).Trim()
    Write-Host "ESP-IDF reported: $versionText"

    if ($versionText -notmatch "v?5\.3\.2") {
        throw "Test 31 requires ESP-IDF 5.3.2. Active environment reports: $versionText"
    }

    Write-Host ""
    Write-Host "=== Test 31 build ==="
    Write-Host "Upstream commit : 578966c969577309b37cf9afb698852e2e81491b"
    Write-Host "ESP-IDF         : 5.3.2"
    Write-Host "LVGL            : 9.2.2"
    Write-Host "Architecture    : native esp_lcd RGB / PSRAM FB / INTERNAL partial LVGL / bounce disabled"
    Write-Host ""

    & python $idfPy set-target esp32s3
    if ($LASTEXITCODE -ne 0) { throw "idf.py set-target failed" }

    & python $idfPy build
    if ($LASTEXITCODE -ne 0) { throw "Test 31 build failed" }

    if ($Flash) {
        Write-Host ""
        Write-Host "=== Test 31 flash ==="
        if ($Port) {
            & python $idfPy -p $Port flash
        }
        else {
            & python $idfPy flash
        }
        if ($LASTEXITCODE -ne 0) { throw "Test 31 flash failed" }
    }
    else {
        Write-Host ""
        Write-Host "Build complete. To flash:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\31_LVGL9_NativeIDF_duck4i\run-test31.ps1 -Flash"
        Write-Host ""
        Write-Host "If a COM port is required explicitly:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\31_LVGL9_NativeIDF_duck4i\run-test31.ps1 -Flash -Port COM7"
    }
}
finally {
    Pop-Location
}
