param(
    [string]$WorkRoot = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($WorkRoot)) {
    $WorkRoot = Join-Path $env:USERPROFILE "t35-robotcore"
}

$upstreamDir = Join-Path $WorkRoot "upstream"
$upstreamRepo = "https://github.com/Albert-Benavent-Cabrera/Robot-Core-Display.git"
$upstreamCommit = "1d95120dc43e7663dfb5888a4da079aae1929153"
$historicalPlatformCommit = "816219db19399d376cfeac3bab6edd14b781701c"

Write-Host "Test 35 work root: $WorkRoot"

if (-not (Test-Path $WorkRoot)) {
    New-Item -ItemType Directory -Force -Path $WorkRoot | Out-Null
}

if (-not (Test-Path $upstreamDir)) {
    Write-Host "Cloning Robot-Core-Display upstream..."
    & git clone $upstreamRepo $upstreamDir
    if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
}

& git -C $upstreamDir fetch --all --tags --prune
if ($LASTEXITCODE -ne 0) { throw "git fetch failed" }

& git -C $upstreamDir reset --hard
if ($LASTEXITCODE -ne 0) { throw "git reset failed" }
& git -C $upstreamDir clean -fdx
if ($LASTEXITCODE -ne 0) { throw "git clean failed" }
& git -C $upstreamDir checkout --detach $upstreamCommit
if ($LASTEXITCODE -ne 0) { throw "checkout of pinned upstream commit failed" }

$head = (& git -C $upstreamDir rev-parse HEAD | Out-String).Trim()
if ($head -ne $upstreamCommit) { throw "Unexpected upstream HEAD: $head" }

$status = (& git -C $upstreamDir status --porcelain --untracked-files=all | Out-String).Trim()
if ($status) { throw "Pinned upstream tree is not clean: $status" }

$required = @(
    (Join-Path $upstreamDir "README.md"),
    (Join-Path $upstreamDir "LICENSE"),
    (Join-Path $upstreamDir "platformio.ini"),
    (Join-Path $upstreamDir "display\display.ino"),
    (Join-Path $upstreamDir "display\lv_conf.h"),
    (Join-Path $upstreamDir "display\secrets_example.h"),
    (Join-Path $upstreamDir "display\src\core\Config.hpp"),
    (Join-Path $upstreamDir "display\src\ui\pages\page_cocktails.cpp"),
    (Join-Path $upstreamDir "display\src\ui\pages\page_config.cpp"),
    (Join-Path $upstreamDir "display\src\ui\pages\page_pumps.cpp"),
    (Join-Path $upstreamDir "lib\GFX_Library_for_Arduino")
)

foreach ($path in $required) {
    if (-not (Test-Path $path)) { throw "Required upstream path missing: $path" }
}

$readmeText = [System.IO.File]::ReadAllText((Join-Path $upstreamDir "README.md"))
$pioText = [System.IO.File]::ReadAllText((Join-Path $upstreamDir "platformio.ini"))
$configText = [System.IO.File]::ReadAllText((Join-Path $upstreamDir "display\src\core\Config.hpp"))

$checks = @(
    @{ Name = "PlatformIO src_dir display"; Ok = $pioText.Contains("src_dir = display") },
    @{ Name = "ESP32S3 environment"; Ok = $pioText.Contains("[env:ESP32S3-8048S043]") },
    @{ Name = "Jason2866 moving platform URL"; Ok = $pioText.Contains("https://github.com/Jason2866/platform-espressif32.git") },
    @{ Name = "LVGL 9.1 declaration"; Ok = $pioText.Contains("lvgl/lvgl @ ^9.1.0") },
    @{ Name = "TAMC GT911 declaration"; Ok = $pioText.Contains("tamctec/TAMC_GT911 @ ^1.0.2") },
    @{ Name = "800x480"; Ok = $configText.Contains("#define SCREEN_WIDTH  800") -and $configText.Contains("#define SCREEN_HEIGHT 480") },
    @{ Name = "PCLK 16MHz"; Ok = $configText.Contains("#define PCLK_SPEED    16000000L") },
    @{ Name = "PCLK inverted"; Ok = $configText.Contains("#define PCLK_INV      1") },
    @{ Name = "RGB bounce 20 lines"; Ok = $configText.Contains("#define BOUNCE_BUFFER_SIZE (SCREEN_WIDTH * 20)") },
    @{ Name = "LVGL draw 100 lines"; Ok = $configText.Contains("#define LVGL_DRAW_LINES 100") },
    @{ Name = "LVGL double buffer"; Ok = $configText.Contains("#define DOUBLE_BUFFER_ENABLED 1") },
    @{ Name = "Custom touch calibration enabled"; Ok = $configText.Contains("#define USE_CUSTOM_CALIBRATION") },
    @{ Name = "Touch X2 330"; Ok = $configText.Contains("#define TOUCH_MAP_X2 330") },
    @{ Name = "Touch Y2 220"; Ok = $configText.Contains("#define TOUCH_MAP_Y2 220") },
    @{ Name = "Cocktail gallery documented"; Ok = $readmeText.Contains("Drink Selection") },
    @{ Name = "Recipe configuration documented"; Ok = $readmeText.Contains("Recipe Config") },
    @{ Name = "Pump configuration documented"; Ok = $readmeText.Contains("Pump Config") },
    @{ Name = "ESP-NOW documented"; Ok = $readmeText.Contains("ESP-NOW") }
)

foreach ($check in $checks) {
    if (-not $check.Ok) { throw "Architecture verification failed: $($check.Name)" }
}

$baseline = @"
Test 35 upstream baseline
=========================
Repository: Albert-Benavent-Cabrera/Robot-Core-Display
Commit: $upstreamCommit
Date: 2026-01-26
License: GPL-3.0
Work root: $WorkRoot

GUI architecture:
- native LVGL C++
- cocktail gallery / cards
- recipe configuration modal
- reusable sliders
- pump calibration page
- reusable footer/navigation
- ESP-NOW remote application state
- mock/offline data available

Display baseline:
- ESP32-S3 / 16 MB flash / OPI PSRAM
- Arduino_GFX vendored upstream
- 800x480 RGB
- PCLK 16 MHz / inverted
- H 8/4/20
- V 8/4/8
- RGB bounce: 20 lines
- LVGL draw lines: 100
- LVGL double buffer enabled

Touch baseline:
- GT911 SDA19/SCL20
- INT18 / RST38
- custom calibration enabled
- TOUCH_MAP_X2 330
- TOUCH_MAP_Y2 220

Historical build pins used by Test 35 runner:
- Jason2866/platform-espressif32: $historicalPlatformCommit
- platform version: 2025.01.50
- framework package: 1901-1318-5.5
- ESP-IDF package: v5.5.2.260104
- LVGL: 9.1.0 exact
- TAMC_GT911: 1.0.2 exact
"@

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText((Join-Path $WorkRoot "BASELINE.txt"), $baseline, $utf8NoBom)

Write-Host "[PASS] Upstream commit: $upstreamCommit"
Write-Host "[PASS] Upstream source tree clean"
Write-Host "[PASS] Applied GUI architecture verified"
Write-Host "[PASS] RGB bounce20 / LVGL draw100 / PCLK16 verified"
Write-Host "[PASS] Custom upstream touch calibration recorded"
Write-Host "[PASS] Historical platform pin selected: $historicalPlatformCommit"
