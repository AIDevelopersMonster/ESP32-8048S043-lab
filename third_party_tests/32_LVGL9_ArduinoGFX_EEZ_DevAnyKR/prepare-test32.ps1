param(
    [string]$WorkRoot = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($WorkRoot)) {
    # Keep the temporary PlatformIO/LVGL tree deliberately short on Windows.
    # LVGL include chains contain many ../ segments and can hit Windows/GCC path
    # handling limits even when the target file exists.
    $WorkRoot = Join-Path $env:USERPROFILE "t32-devany"
}

$workRoot = $WorkRoot
$upstreamDir = Join-Path $workRoot "upstream"
$upstreamRepo = "https://github.com/DevAnyKR/ESP32_8048S043C.git"
$upstreamCommit = "bb056490f0738911618f60f98e164f36dde0f84d"

Write-Host "Test 32 short work root: $workRoot"

if (-not (Test-Path $workRoot)) {
    New-Item -ItemType Directory -Force -Path $workRoot | Out-Null
}

if (-not (Test-Path $upstreamDir)) {
    Write-Host "Cloning DevAnyKR upstream..."
    & git clone $upstreamRepo $upstreamDir
    if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
}

& git -C $upstreamDir fetch --all --tags --prune
if ($LASTEXITCODE -ne 0) { throw "git fetch failed" }

& git -C $upstreamDir reset --hard
& git -C $upstreamDir clean -fdx
& git -C $upstreamDir checkout --detach $upstreamCommit
if ($LASTEXITCODE -ne 0) { throw "checkout of pinned upstream commit failed" }

$head = (& git -C $upstreamDir rev-parse HEAD | Out-String).Trim()
if ($head -ne $upstreamCommit) { throw "Unexpected upstream HEAD: $head" }

$status = (& git -C $upstreamDir status --porcelain --untracked-files=all | Out-String).Trim()
if ($status) { throw "Pinned upstream tree is not clean: $status" }

$platformio = Join-Path $upstreamDir "platformio.ini"
$mainCpp = Join-Path $upstreamDir "EEZ_DEMO\main.cpp"
$touchH = Join-Path $upstreamDir "include\touch.h"
$boardJson = Join-Path $upstreamDir "boards\ESP32-8048S043C.json"
$lvConf = Join-Path $upstreamDir "include\lv_conf.h"

foreach ($required in @($platformio, $mainCpp, $touchH, $boardJson, $lvConf)) {
    if (-not (Test-Path $required)) { throw "Required upstream file missing: $required" }
}

$iniText = [System.IO.File]::ReadAllText($platformio)
$mainText = [System.IO.File]::ReadAllText($mainCpp)
$touchText = [System.IO.File]::ReadAllText($touchH)
$lvConfText = [System.IO.File]::ReadAllText($lvConf)

$checks = @(
    @{ Name = "EEZ_DEMO source directory"; Ok = $iniText.Contains("src_dir = EEZ_DEMO") },
    @{ Name = "Arduino framework"; Ok = $iniText.Contains("framework = arduino") },
    @{ Name = "LVGL dependency"; Ok = $iniText.Contains("lvgl/lvgl@^9.1.0") },
    @{ Name = "LVGL config v9.2.2"; Ok = $lvConfText.Contains("Configuration file for v9.2.2") },
    @{ Name = "Arduino_GFX dependency"; Ok = $iniText.Contains("GFX Library for Arduino@^1.5.2") },
    @{ Name = "PCLK 15 MHz"; Ok = $mainText.Contains("15 * 1000000L") },
    @{ Name = "pclk_active_neg = 0"; Ok = $mainText.Contains("0 /* pclk_active_neg */") },
    @{ Name = "PARTIAL render mode"; Ok = $mainText.Contains("LV_DISPLAY_RENDER_MODE_PARTIAL") },
    @{ Name = "draw16bitRGBBitmap flush"; Ok = $mainText.Contains("draw16bitRGBBitmap") },
    @{ Name = "MALLOC_CAP_8BIT draw buffer"; Ok = $mainText.Contains("heap_caps_malloc(bufSize, MALLOC_CAP_8BIT)") },
    @{ Name = "GT911 enabled"; Ok = $touchText.Contains("#define TOUCH_GT911") }
)

foreach ($check in $checks) {
    if (-not $check.Ok) { throw "Architecture verification failed: $($check.Name)" }
}

$baseline = @"
Test 32 upstream baseline
=========================
Repository: DevAnyKR/ESP32_8048S043C
Commit: $upstreamCommit
Date: 2025-01-23
Short Windows work root: $workRoot

Architecture verified from pinned source:
- PlatformIO / Arduino
- LVGL dependency ^9.1.0 with checked-in lv_conf.h for v9.2.2
- Arduino_GFX RGB
- EEZ_DEMO / EEZ Studio generated UI
- PARTIAL render mode
- one MALLOC_CAP_8BIT draw buffer
- PCLK 15 MHz
- pclk_active_neg 0
- GT911 touch

The upstream source tree is preserved unchanged.
The short work path is intentional to avoid Windows/GCC failures on LVGL's deep relative include chains.
"@

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText((Join-Path $workRoot "BASELINE.txt"), $baseline, $utf8NoBom)

Write-Host "[PASS] Upstream commit: $upstreamCommit"
Write-Host "[PASS] Upstream tracked tree clean"
Write-Host "[PASS] Arduino_GFX + LVGL PARTIAL + EEZ architecture verified"
Write-Host "[PASS] LVGL checked-in config targets v9.2.2"
Write-Host "[PASS] PCLK 15 MHz / pclk_active_neg 0 / GT911 verified"
Write-Host "[PASS] Short-path baseline recorded outside upstream source"
