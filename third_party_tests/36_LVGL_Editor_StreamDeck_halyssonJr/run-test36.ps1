param(
    [switch]$Upload,
    [string]$UploadPort = "",
    [string]$WorkRoot = "$HOME\t36-lvgl-editor",
    [string]$IdfExport = ""
)

$ErrorActionPreference = 'Stop'

$Commit = '79e862ca332525ba8721c4691f450fb44ec08738'
$Upstream = Join-Path $WorkRoot 'upstream'
$Prepare = Join-Path $PSScriptRoot 'prepare-test36.ps1'

& powershell -ExecutionPolicy Bypass -File $Prepare -WorkRoot $WorkRoot

function Import-IdfEnvironment {
    param([string]$ExplicitExport)

    if ($ExplicitExport) {
        if (-not (Test-Path $ExplicitExport)) {
            throw "ESP-IDF export script not found: $ExplicitExport"
        }
        . $ExplicitExport
        return
    }

    if (Get-Command idf.py -ErrorAction SilentlyContinue) {
        return
    }

    $Candidates = @(
        "$HOME\esp\v5.5.5\esp-idf\export.ps1",
        "$HOME\esp\v5.5\esp-idf\export.ps1",
        "$HOME\esp\esp-idf\export.ps1",
        "C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1",
        "C:\Espressif\frameworks\esp-idf-v5.5\export.ps1"
    )

    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            Write-Host "Using ESP-IDF environment: $Candidate"
            . $Candidate
            return
        }
    }

    throw "idf.py is not available. Open an ESP-IDF 5.5.x PowerShell or pass -IdfExport <path-to-export.ps1>."
}

Import-IdfEnvironment -ExplicitExport $IdfExport

$idfVersionText = (& idf.py --version | Out-String).Trim()
Write-Host "ESP-IDF: $idfVersionText"
if ($idfVersionText -notmatch 'v?5\.5\.') {
    Write-Warning "Upstream lock is ESP-IDF 5.5.0. This runner is intended for the ESP-IDF 5.5.x line."
}

Push-Location $Upstream
try {
    $Head = (git rev-parse HEAD).Trim()
    if ($Head -ne $Commit) {
        throw "Pinned commit mismatch before build: $Head"
    }
    if (git status --porcelain) {
        throw "Upstream tree is dirty before build"
    }

    idf.py set-target esp32s3
    if ($LASTEXITCODE -ne 0) { throw "idf.py set-target failed" }

    idf.py build
    if ($LASTEXITCODE -ne 0) { throw "idf.py build failed" }

    Write-Host "[PASS] Test 36 build complete"

    if ($Upload) {
        if (-not $UploadPort) {
            throw "-Upload requires -UploadPort, for example COM7"
        }
        idf.py -p $UploadPort flash
        if ($LASTEXITCODE -ne 0) { throw "idf.py flash failed" }
        Write-Host "[PASS] Test 36 flashed to $UploadPort"
        Write-Host "Monitor command: idf.py -p $UploadPort monitor"
    }
}
finally {
    # Keep build output and managed components for reproducibility/incremental rebuild,
    # but ensure tracked source remains unchanged.
    git checkout -- . 2>$null
    $After = (git rev-parse HEAD).Trim()
    if ($After -eq $Commit) {
        Write-Host "[PASS] Exact pinned upstream revision retained"
    }
    Pop-Location
}
