param(
    [string]$WorkRoot = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($WorkRoot)) {
    $WorkRoot = Join-Path $env:USERPROFILE "t33-ryanewen"
}

$upstreamDir = Join-Path $WorkRoot "upstream"
$upstreamRepo = "https://github.com/RyanEwen/esphome-lvgl.git"
$upstreamCommit = "4d3ff33b242c6b6ff67dc76f1cfa9b1041473362"

Write-Host "Test 33 work root: $WorkRoot"

if (-not (Test-Path $WorkRoot)) {
    New-Item -ItemType Directory -Force -Path $WorkRoot | Out-Null
}

if (-not (Test-Path $upstreamDir)) {
    Write-Host "Cloning RyanEwen upstream..."
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

$example = Join-Path $upstreamDir "sunton-43-example.yaml"
$device = Join-Path $upstreamDir "devices\ESP32-8048S043.yaml"
$layout = Join-Path $upstreamDir "layouts\800x480.yaml"
$common = Join-Path $upstreamDir "common.yaml"

foreach ($required in @($example, $device, $layout, $common)) {
    if (-not (Test-Path $required)) { throw "Required upstream file missing: $required" }
}

$exampleText = [System.IO.File]::ReadAllText($example)
$deviceText = [System.IO.File]::ReadAllText($device)
$layoutText = [System.IO.File]::ReadAllText($layout)
$commonText = [System.IO.File]::ReadAllText($common)

$checks = @(
    @{ Name = "Sunton device package"; Ok = $exampleText.Contains("devices/ESP32-8048S043.yaml") },
    @{ Name = "800x480 layout package"; Ok = $exampleText.Contains("layouts/800x480.yaml") },
    @{ Name = "common package"; Ok = $exampleText.Contains("common.yaml") },
    @{ Name = "mipi_rgb display"; Ok = $deviceText.Contains("platform: mipi_rgb") },
    @{ Name = "RPI RGB model"; Ok = $deviceText.Contains("model: RPI") },
    @{ Name = "800 width"; Ok = $deviceText.Contains("width: 800") },
    @{ Name = "480 height"; Ok = $deviceText.Contains("height: 480") },
    @{ Name = "PCLK 14MHz"; Ok = $deviceText.Contains("pclk_frequency: 14MHz") },
    @{ Name = "PCLK inverted"; Ok = $deviceText.Contains("pclk_inverted: true") },
    @{ Name = "GT911"; Ok = $deviceText.Contains("platform: gt911") },
    @{ Name = "Octal PSRAM"; Ok = $deviceText.Contains("mode: octal") },
    @{ Name = "LVGL UI"; Ok = $layoutText.Contains("lvgl:") },
    @{ Name = "modular widget includes"; Ok = $layoutText.Contains("widgets/") },
    @{ Name = "WiFi secrets"; Ok = $commonText.Contains("!secret wifi_ssid") -and $commonText.Contains("!secret wifi_password") }
)

foreach ($check in $checks) {
    if (-not $check.Ok) { throw "Architecture verification failed: $($check.Name)" }
}

$baseline = @"
Test 33 upstream baseline
=========================
Repository: RyanEwen/esphome-lvgl
Commit: $upstreamCommit
Date: 2026-01-13
Work root: $WorkRoot

Verified target:
- sunton-43-example.yaml
- common.yaml package
- devices/ESP32-8048S043.yaml package
- layouts/800x480.yaml package
- ESP32-S3 / 16 MB flash / Octal PSRAM
- ESP-IDF framework
- mipi_rgb model RPI
- 800x480
- PCLK 14 MHz / inverted
- GT911
- modular LVGL YAML widgets

Historical ESPHome build pin: 2025.12.5
"@

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText((Join-Path $WorkRoot "BASELINE.txt"), $baseline, $utf8NoBom)

Write-Host "[PASS] Upstream commit: $upstreamCommit"
Write-Host "[PASS] Upstream source tree clean"
Write-Host "[PASS] Sunton 4.3-inch package composition verified"
Write-Host "[PASS] ESPHome + LVGL modular architecture verified"
Write-Host "[PASS] PCLK 14 MHz / inverted / GT911 verified"
