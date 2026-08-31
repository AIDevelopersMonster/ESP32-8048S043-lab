param(
    [switch]$Upload
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$projectDir = Join-Path $repoRoot ".external-test-work\21_LVGL9_ArduinoGFX_EEZ_clumsyCoder00\upstream\Sunton-ESP32-8048S043"
$pio = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"

if (-not (Test-Path $projectDir)) {
    throw "Test 21 project not found: $projectDir"
}
if (-not (Test-Path $pio)) {
    throw "PlatformIO CLI not found: $pio"
}

$driveLetter = $null
foreach ($letter in @("T", "U", "V", "W", "X", "Y", "Z")) {
    if (-not (Test-Path "${letter}:\")) {
        $driveLetter = $letter
        break
    }
}
if (-not $driveLetter) {
    throw "No free drive letter found in T: through Z: for temporary short-path mapping"
}

$drive = "${driveLetter}:"
$shortProject = "${drive}\"

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 21 short-path runner ==="
Write-Host "Physical project : $projectDir"
Write-Host "Temporary path   : $shortProject"
Write-Host "Action           : $(if ($Upload) { 'upload' } else { 'build' })"
Write-Host ""

try {
    & subst $drive $projectDir
    if ($LASTEXITCODE -ne 0) {
        throw "subst failed for $drive -> $projectDir"
    }

    if (-not (Test-Path (Join-Path $shortProject "platformio.ini"))) {
        throw "Short-path mapping does not expose platformio.ini"
    }
    Write-Host "[PASS] Short path mapped"

    $lvInternal = Join-Path $shortProject "lib\lvgl\src\lv_conf_internal.h"
    if (-not (Test-Path $lvInternal)) {
        throw "LVGL internal config header is missing: $lvInternal"
    }
    Write-Host "[PASS] LVGL src\lv_conf_internal.h visible through short path"

    $pioDir = Join-Path $shortProject ".pio"
    if (Test-Path $pioDir) {
        Write-Host "[INFO] Removing prior .pio state before short-path build..."
        Remove-Item $pioDir -Recurse -Force
    }

    Push-Location $shortProject
    try {
        & $pio --version
        if ($Upload) {
            & $pio run --target upload
        }
        else {
            & $pio run
        }
        if ($LASTEXITCODE -ne 0) {
            throw "PlatformIO returned exit code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}
finally {
    & subst $drive /D 2>$null
    Write-Host ""
    Write-Host "[INFO] Temporary drive $drive removed"
}
