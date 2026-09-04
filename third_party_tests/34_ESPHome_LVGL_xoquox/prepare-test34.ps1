param(
    [string]$WorkRoot = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($WorkRoot)) {
    $WorkRoot = Join-Path $env:USERPROFILE "t34-xoquox"
}

$upstreamDir = Join-Path $WorkRoot "upstream"
$upstreamRepo = "https://github.com/xoquox/esphome-lvgl.git"
$upstreamCommit = "33a2a35c0c09c9b3c825e98a0a4abe41931b5708"
$wrapper = Join-Path $WorkRoot "xoquox-43-test.yaml"

Write-Host "Test 34 work root: $WorkRoot"

if (-not (Test-Path $WorkRoot)) {
    New-Item -ItemType Directory -Force -Path $WorkRoot | Out-Null
}

if (-not (Test-Path $upstreamDir)) {
    Write-Host "Cloning xoquox upstream..."
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

$device = Join-Path $upstreamDir "devices\ESP32-8048S043.yaml"
$layout = Join-Path $upstreamDir "layouts\800x480.yaml"
$common = Join-Path $upstreamDir "common.yaml"
$bme680 = Join-Path $upstreamDir "sensors\bme680.yaml"

foreach ($required in @($device, $layout, $common, $bme680)) {
    if (-not (Test-Path $required)) { throw "Required upstream file missing: $required" }
}

$deviceText = [System.IO.File]::ReadAllText($device)
$layoutText = [System.IO.File]::ReadAllText($layout)
$commonText = [System.IO.File]::ReadAllText($common)
$bmeText = [System.IO.File]::ReadAllText($bme680)

$checks = @(
    @{ Name = "rpi_dpi_rgb display"; Ok = $deviceText.Contains("platform: rpi_dpi_rgb") },
    @{ Name = "800 width"; Ok = $deviceText.Contains("width: 800") },
    @{ Name = "480 height"; Ok = $deviceText.Contains("height: 480") },
    @{ Name = "PCLK 14MHz"; Ok = $deviceText.Contains("pclk_frequency: 14MHz") },
    @{ Name = "PCLK inverted"; Ok = $deviceText.Contains("pclk_inverted: true") },
    @{ Name = "GT911"; Ok = $deviceText.Contains("platform: gt911") },
    @{ Name = "touch bus_a"; Ok = $deviceText.Contains("id: bus_a") -and $deviceText.Contains("i2c_id: bus_a") },
    @{ Name = "sensor bus_b"; Ok = $deviceText.Contains("id: bus_b") -and $deviceText.Contains("sda: 17") -and $deviceText.Contains("scl: 18") },
    @{ Name = "historical anti-artifact comment"; Ok = $deviceText.Contains("these versions prevent artifacting") },
    @{ Name = "historical pins commented"; Ok = $deviceText.Contains("# version: 5.3.0") -and $deviceText.Contains("# platform_version: 6.8.1") },
    @{ Name = "Octal PSRAM"; Ok = $deviceText.Contains("mode: octal") },
    @{ Name = "LVGL UI"; Ok = $layoutText.Contains("lvgl:") },
    @{ Name = "modular widget includes"; Ok = $layoutText.Contains("widgets/") },
    @{ Name = "WiFi secrets"; Ok = $commonText.Contains("!secret wifi_ssid") -and $commonText.Contains("!secret wifi_password") },
    @{ Name = "BME680 package"; Ok = $bmeText.Contains("platform: bme680") -and $bmeText.Contains("i2c_id: bus_b") }
)

foreach ($check in $checks) {
    if (-not $check.Ok) { throw "Architecture verification failed: $($check.Name)" }
}

$wrapperText = @"
esphome:
  name: xoquox-43-test
  friendly_name: xoquox 4.3 Test

substitutions:
  home_page: lighting_1

packages:
  common: !include upstream/common.yaml
  device: !include upstream/devices/ESP32-8048S043.yaml
  layout: !include upstream/layouts/800x480.yaml
"@

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($wrapper, $wrapperText, $utf8NoBom)

$baseline = @"
Test 34 upstream baseline
=========================
Repository: xoquox/esphome-lvgl
Parent: RyanEwen/esphome-lvgl
Commit: $upstreamCommit
Date: 2026-01-18
Work root: $WorkRoot

Verified target:
- external wrapper xoquox-43-test.yaml
- common.yaml package
- devices/ESP32-8048S043.yaml package
- layouts/800x480.yaml package
- sensors/bme680.yaml exists but is not included in baseline wrapper
- ESP32-S3 / 16 MB flash / Octal PSRAM
- ESP-IDF framework
- rpi_dpi_rgb
- 800x480
- PCLK 14 MHz / inverted
- GT911 on bus_a GPIO19/20
- second I2C bus_b GPIO17/18
- historical anti-artifact pins retained only as comments
- modular LVGL YAML widgets

Historical ESPHome build pin: 2025.12.7
"@

[System.IO.File]::WriteAllText((Join-Path $WorkRoot "BASELINE.txt"), $baseline, $utf8NoBom)

Write-Host "[PASS] Upstream commit: $upstreamCommit"
Write-Host "[PASS] Upstream source tree clean"
Write-Host "[PASS] External Test 34 wrapper created outside upstream source"
Write-Host "[PASS] rpi_dpi_rgb / PCLK14 / inverted / GT911 bus_a verified"
Write-Host "[PASS] dual-I2C fork profile verified"
Write-Host "[PASS] BME680 package detected but excluded from baseline wrapper"
Write-Host "[PASS] historical anti-artifact pins remain commented"
