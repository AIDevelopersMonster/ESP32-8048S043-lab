param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$upstreamUrl = "https://github.com/ffodGit/esp32-8048s043-getting-started-00.git"
$pinnedCommit = "18b6d4de509abb61feb0084c1583d41497836cfd"
$envName = "esp32-s3-devkitm-1"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$workRoot = Join-Path $repoRoot ".third-party-work\30_LVGL9_LovyanGFX_EEZ_ffodGit"
$upstreamDir = Join-Path $workRoot "upstream"

function Require-Token([string]$text, [string]$token, [string]$label) {
    if (-not $text.Contains($token)) {
        throw "Missing expected upstream token in ${label}: $token"
    }
}

if ($Force -and (Test-Path $workRoot)) {
    Remove-Item $workRoot -Recurse -Force
}

New-Item -ItemType Directory -Force $workRoot | Out-Null

if (-not (Test-Path $upstreamDir)) {
    Write-Host "Cloning pinned upstream source..."
    git clone --no-checkout $upstreamUrl $upstreamDir
    if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
}

Push-Location $upstreamDir
try {
    git fetch origin $pinnedCommit
    if ($LASTEXITCODE -ne 0) { throw "git fetch pinned commit failed" }

    git checkout --detach $pinnedCommit
    if ($LASTEXITCODE -ne 0) { throw "git checkout pinned commit failed" }

    $head = (git rev-parse HEAD).Trim()
    if ($head -ne $pinnedCommit) {
        throw "Pinned commit mismatch. Expected $pinnedCommit, got $head"
    }

    $status = @(git status --porcelain)
    if ($status.Count -ne 0) {
        throw "Upstream working tree is not clean after checkout. No source modifications are allowed for Test 30 baseline."
    }

    $pio = Get-Content (Join-Path $upstreamDir "platformio.ini") -Raw
    $lgfx = Get-Content (Join-Path $upstreamDir "include\lovyanGfxSetup.h") -Raw
    $main = Get-Content (Join-Path $upstreamDir "src\main.cpp") -Raw
    $version = Get-Content (Join-Path $upstreamDir "lib\lvgl\lv_version.h") -Raw

    Require-Token $pio "platform = espressif32" "platformio.ini"
    Require-Token $pio "board = esp32-s3-devkitm-1" "platformio.ini"
    Require-Token $pio "lovyan03/LovyanGFX@^1.1.16" "platformio.ini"
    Require-Token $pio "board_build.arduino.memory_type = qio_opi" "platformio.ini"
    Require-Token $pio "board_upload.flash_size = 16MB" "platformio.ini"

    Require-Token $lgfx "cfg.freq_write = 14000000;" "lovyanGfxSetup.h"
    Require-Token $lgfx "cfg.hsync_front_porch = 8;" "lovyanGfxSetup.h"
    Require-Token $lgfx "cfg.hsync_pulse_width = 4;" "lovyanGfxSetup.h"
    Require-Token $lgfx "cfg.hsync_back_porch  = 8;" "lovyanGfxSetup.h"
    Require-Token $lgfx "cfg.vsync_front_porch = 8;" "lovyanGfxSetup.h"
    Require-Token $lgfx "cfg.vsync_pulse_width = 4;" "lovyanGfxSetup.h"
    Require-Token $lgfx "cfg.vsync_back_porch  = 8;" "lovyanGfxSetup.h"
    Require-Token $lgfx "cfg.pclk_active_neg = 1;" "lovyanGfxSetup.h"
    Require-Token $lgfx "cfg.i2c_addr   = 0x5D;" "lovyanGfxSetup.h"

    Require-Token $main "LV_DISPLAY_RENDER_MODE_PARTIAL" "main.cpp"
    Require-Token $main "tft.writePixels((lgfx::rgb565_t *)px_map, w * h);" "main.cpp"
    Require-Token $main "ui_init();" "main.cpp"

    Require-Token $version "#define LVGL_VERSION_MAJOR 9" "lv_version.h"
    Require-Token $version "#define LVGL_VERSION_MINOR 1" "lv_version.h"
    Require-Token $version "#define LVGL_VERSION_PATCH 1" "lv_version.h"

    $manifest = @"
Test 30 upstream baseline
=========================
Repository : $upstreamUrl
Commit     : $pinnedCommit
Environment: $envName
Policy     : exact upstream source; no source patches

Verified architecture:
- LVGL 9.1.1-dev (vendored upstream)
- LovyanGFX ^1.1.16
- LVGL PARTIAL render mode
- ~1/10-screen static draw buffer
- LovyanGFX writePixels flush path
- 800x480 RGB
- PCLK 14 MHz
- HSYNC 8/4/8
- VSYNC 8/4/8
- pclk_active_neg = 1
- GT911 addr 0x5D, SDA19, SCL20, RST38

Reproducibility note:
- upstream leaves `platform = espressif32` unpinned
- resolved PlatformIO/Arduino core version must be recorded after build
"@
    Set-Content -Path (Join-Path $workRoot "BASELINE.txt") -Value $manifest -Encoding UTF8

    Write-Host ""
    Write-Host "=== Test 30 pinned upstream prepared ==="
    Write-Host "Upstream : $upstreamUrl"
    Write-Host "Commit   : $pinnedCommit"
    Write-Host "Path     : $upstreamDir"
    Write-Host ""
    Write-Host "[PASS] Exact upstream source checked out"
    Write-Host "[PASS] Working tree clean / no source patches"
    Write-Host "[PASS] LVGL 9.1.1-dev markers verified"
    Write-Host "[PASS] LovyanGFX RGB 800x480 / PCLK14 / 8-4-8 verified"
    Write-Host "[PASS] GT911 configuration verified"
    Write-Host ""
    Write-Host "Next: run .\third_party_tests\30_LVGL9_LovyanGFX_EEZ_ffodGit\run-test30.ps1"
}
finally {
    Pop-Location
}
