param(
    [switch]$Upload,
    [string]$UploadPort = "",
    [string]$PlatformIOPath = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$workRoot = Join-Path $repoRoot ".third-party-work\32_LVGL9_ArduinoGFX_EEZ_DevAnyKR"
$upstreamDir = Join-Path $workRoot "upstream"
$prep = Join-Path $scriptDir "prepare-test32.ps1"
$platformioIni = Join-Path $upstreamDir "platformio.ini"
$overlayStamp = Join-Path $workRoot "HISTORICAL_ENV.txt"

$upstreamCommit = "bb056490f0738911618f60f98e164f36dde0f84d"
$platformSpec = "https://github.com/pioarduino/platform-espressif32/releases/download/53.03.11/platform-espressif32.zip"
$lvglVersion = "9.1.0"
$gfxVersion = "1.5.2"
$timeVersion = "1.6.1"
$eezCommit = "5bb6c8692d440e599469d5c52b6c3f2094dbf910"
$gt911Commit = "b3f175e65a799368be9c544e255204e1e74ad2ed"

if (-not (Test-Path $upstreamDir)) {
    & powershell -ExecutionPolicy Bypass -File $prep
    if ($LASTEXITCODE -ne 0) { throw "prepare-test32.ps1 failed" }
}

function Resolve-PlatformIO {
    param([string]$Explicit)

    if ($Explicit) {
        if (Test-Path $Explicit) { return (Resolve-Path $Explicit).Path }
        throw "Specified PlatformIO executable not found: $Explicit"
    }

    foreach ($name in @("pio", "platformio")) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) { return $cmd.Source }
    }

    $candidates = @(
        (Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"),
        (Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python313\Scripts\platformio.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python312\Scripts\platformio.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python311\Scripts\platformio.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) { return (Resolve-Path $candidate).Path }
    }

    return $null
}

$pio = Resolve-PlatformIO -Explicit $PlatformIOPath
if (-not $pio) { throw "PlatformIO was not found" }

Write-Host "[PASS] PlatformIO: $pio"

# Always restore original upstream config before inspecting or overlaying it.
& git -C $upstreamDir checkout -- platformio.ini
if ($LASTEXITCODE -ne 0) { throw "Could not restore upstream platformio.ini" }

$original = [System.IO.File]::ReadAllText($platformioIni)
if (-not $original.Contains("platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip")) {
    throw "Unexpected upstream platform declaration"
}
if (-not $original.Contains("lvgl/lvgl@^9.1.0")) { throw "Unexpected upstream LVGL dependency" }
if (-not $original.Contains("moononournation/GFX Library for Arduino@^1.5.2")) { throw "Unexpected upstream Arduino_GFX dependency" }

$overlay = $original
$overlay = $overlay.Replace(
    "platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip",
    "platform = $platformSpec"
)
$overlay = $overlay.Replace("lvgl/lvgl@^9.1.0", "lvgl/lvgl@$lvglVersion")
$overlay = $overlay.Replace("paulstoffregen/Time@^1.6.1", "paulstoffregen/Time@$timeVersion")
$overlay = $overlay.Replace("https://github.com/eez-open/eez-framework.git", "https://github.com/eez-open/eez-framework.git#$eezCommit")
$overlay = $overlay.Replace("https://github.com/TAMCTec/gt911-arduino.git", "https://github.com/TAMCTec/gt911-arduino.git#$gt911Commit")
$overlay = $overlay.Replace("moononournation/GFX Library for Arduino@^1.5.2", "moononournation/GFX Library for Arduino@$gfxVersion")

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($platformioIni, $overlay, $utf8NoBom)

$bytes = [System.IO.File]::ReadAllBytes($platformioIni)
if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
    throw "Temporary platformio.ini was written with a UTF-8 BOM"
}

$envText = @"
Test 32 historical environment overlay
======================================
Upstream commit: $upstreamCommit
Upstream date: 2025-01-23

Temporary build-only dependency reconstruction:
platform-espressif32: 53.03.11
Arduino core:          3.1.1
ESP-IDF line:          5.3.2.241224
LVGL:                  $lvglVersion
Arduino_GFX:           $gfxVersion
Time:                   $timeVersion
EEZ framework:          $eezCommit
GT911 Arduino:          $gt911Commit

Source display/touch/UI code is not modified.
platformio.ini is restored after the command finishes.
"@
[System.IO.File]::WriteAllText($overlayStamp, $envText, $utf8NoBom)

# Avoid stale packages/objects from a previous dependency overlay.
$pioDir = Join-Path $upstreamDir ".pio"
if (Test-Path $pioDir) {
    Remove-Item -Recurse -Force $pioDir
}

Push-Location $upstreamDir
try {
    Write-Host ""
    Write-Host "=== Test 32 historical build ==="
    Write-Host "Upstream commit : $upstreamCommit"
    Write-Host "Platform         : pioarduino 53.03.11 / Arduino 3.1.1 / IDF 5.3.2 line"
    Write-Host "LVGL             : $lvglVersion"
    Write-Host "Arduino_GFX      : $gfxVersion"
    Write-Host "Architecture     : Arduino_GFX RGB / EEZ / LVGL PARTIAL / PCLK15 / pclk edge 0"
    Write-Host ""

    & $pio run -e ESP32_8048S043C
    if ($LASTEXITCODE -ne 0) { throw "Test 32 build failed" }

    if ($Upload) {
        Write-Host ""
        Write-Host "=== Test 32 upload ==="
        if ($UploadPort) {
            & $pio run -e ESP32_8048S043C -t upload --upload-port $UploadPort
        }
        else {
            & $pio run -e ESP32_8048S043C -t upload
        }
        if ($LASTEXITCODE -ne 0) { throw "Test 32 upload failed" }
    }
    else {
        Write-Host ""
        Write-Host "Build complete. To upload:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\32_LVGL9_ArduinoGFX_EEZ_DevAnyKR\run-test32.ps1 -Upload"
        Write-Host ""
        Write-Host "Explicit port example:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\32_LVGL9_ArduinoGFX_EEZ_DevAnyKR\run-test32.ps1 -Upload -UploadPort COM7"
    }
}
finally {
    Pop-Location
    & git -C $upstreamDir checkout -- platformio.ini

    $status = (& git -C $upstreamDir status --porcelain | Out-String).Trim()
    if ($status) {
        Write-Host "[FAIL] Upstream tracked tree changed after build:"
        Write-Host $status
        throw "Exact upstream source was not restored"
    }
    Write-Host "[PASS] Exact tracked upstream source restored after build"
}
