$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$workRoot = Join-Path $repoRoot ".external-test-work\21_LVGL9_ArduinoGFX_EEZ_clumsyCoder00"
$upstreamDir = Join-Path $workRoot "upstream"
$upstreamUrl = "https://github.com/clumsyCoder00/Sunton-ESP32-8048S043.git"
$pinnedCommit = "e89a5cd6f50c54f0d0c49dcdfde4f7dfd909c5c7"
$projectDir = Join-Path $upstreamDir "Sunton-ESP32-8048S043"

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 21 ==="
Write-Host "External project : clumsyCoder00/Sunton-ESP32-8048S043"
Write-Host "Pinned commit    : $pinnedCommit"
Write-Host "Work directory   : $upstreamDir"
Write-Host ""

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git is not available in PATH"
}

New-Item -ItemType Directory -Force $workRoot | Out-Null

if (-not (Test-Path (Join-Path $upstreamDir ".git"))) {
    Write-Host "[INFO] Cloning upstream..."
    git clone $upstreamUrl $upstreamDir
}
else {
    Write-Host "[INFO] Existing upstream clone found. Fetching..."
    git -C $upstreamDir fetch --all --tags --prune
}

Write-Host "[INFO] Checking out pinned revision..."
git -C $upstreamDir checkout --detach $pinnedCommit

$actualCommit = (git -C $upstreamDir rev-parse HEAD).Trim()
if ($actualCommit -ne $pinnedCommit) {
    throw "Pinned revision mismatch: expected $pinnedCommit, got $actualCommit"
}

Write-Host ""
Write-Host "=== UPSTREAM SNAPSHOT ==="
git -C $upstreamDir log -1 --format="Commit : %H%nDate   : %aI%nAuthor : %an <%ae>%nTitle  : %s"

Write-Host ""
Write-Host "=== LICENSE ==="
if (Test-Path (Join-Path $upstreamDir "LICENSE")) {
    Get-Content (Join-Path $upstreamDir "LICENSE") -TotalCount 3
}

Write-Host ""
Write-Host "=== PLATFORMIO SETTINGS ==="
Get-Content (Join-Path $projectDir "platformio.ini")

Write-Host ""
Write-Host "=== PUBLISHED LIBRARY VERSIONS ==="
$rootReadme = Get-Content (Join-Path $upstreamDir "README.md")
$rootReadme | Select-String -Pattern "Arduino GFX|eez-framework|GT911|LVGL"

Write-Host ""
Write-Host "=== KEY DISPLAY/LVGL SETTINGS ==="
$mainCpp = Get-Content (Join-Path $projectDir "src\main.cpp")
$mainCpp | Select-String -Pattern "DIRECT_MODE|RGB_PANEL|Arduino_ESP32RGBPanel|Arduino_RGB_Display|draw16bitRGBBitmap|lv_display_set_buffers"

Write-Host ""
Write-Host "=== KEY TOUCH SETTINGS ==="
$touchHeader = Get-Content (Join-Path $projectDir "src\touch.h")
$touchHeader | Select-String -Pattern "TOUCH_GT911|TOUCH_MAP_X|TOUCH_MAP_Y|TOUCH_GT911_ROTATION"

Write-Host ""
Write-Host "=== PLATFORMIO CLI ==="
$pio = Get-Command pio -ErrorAction SilentlyContinue
$platformio = Get-Command platformio -ErrorAction SilentlyContinue

if ($pio) {
    Write-Host "[PASS] pio found: $($pio.Source)"
    pio --version
    Write-Host ""
    Write-Host "Next commands:"
    Write-Host "  cd `"$projectDir`""
    Write-Host "  pio run"
}
elseif ($platformio) {
    Write-Host "[PASS] platformio found: $($platformio.Source)"
    platformio --version
    Write-Host ""
    Write-Host "Next commands:"
    Write-Host "  cd `"$projectDir`""
    Write-Host "  platformio run"
}
else {
    Write-Host "[NOTE] PlatformIO CLI is not currently available in PATH."
    Write-Host "Install or activate PlatformIO before building Test 21."
}
