$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$sourceDir = Join-Path $repoRoot "libraries\ESP32_8048S043\examples\19_LVGL9_ArduinoGFX_BounceBufferUI"
$sourceFile = Join-Path $sourceDir "19_LVGL9_ArduinoGFX_BounceBufferUI.ino"
$outDir = Join-Path $repoRoot ".controlled-test-work\22_LVGL9_ArduinoGFX_NoBounce_Isolation"
$outFile = Join-Path $outDir "22_LVGL9_ArduinoGFX_NoBounce_Isolation.ino"

if (-not (Test-Path $sourceFile)) {
    throw "Frozen Test 19 source not found: $sourceFile"
}

if (Test-Path $outDir) {
    Remove-Item $outDir -Recurse -Force
}
New-Item -ItemType Directory -Force $outDir | Out-Null

$src = Get-Content $sourceFile -Raw

$required = @(
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 20;',
    '19-LVGL9-GFX-BOUNCE1-240830A',
    'RGB bounce buffer = 20 display lines'
)
foreach ($needle in $required) {
    if (-not $src.Contains($needle)) {
        throw "Frozen Test 19 source does not match expected baseline token: $needle"
    }
}

# Controlled mutation: only disable the RGB bounce buffer.
$dst = $src.Replace(
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 20;',
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0; // Test 22 only change: bounce disabled'
)

# Identification/reporting changes only; no runtime architecture change.
$dst = $dst.Replace('19_LVGL9_ArduinoGFX_BounceBufferUI', '22_LVGL9_ArduinoGFX_NoBounce_Isolation')
$dst = $dst.Replace('19-LVGL9-GFX-BOUNCE1-240830A', '22-LVGL9-GFX-NOBOUNCE1-240831A')
$dst = $dst.Replace('ESP32-8048S043 Lab / Test 19', 'ESP32-8048S043 Lab / Test 22')
$dst = $dst.Replace('LVGL9 + double internal SRAM buffers + RGB bounce buffer', 'LVGL9 + double internal SRAM buffers + NO RGB bounce buffer')
$dst = $dst.Replace('RGB bounce buffer = 20 display lines;', 'RGB bounce buffer = DISABLED;')
$dst = $dst.Replace('20U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)', '0U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)')
$dst = $dst.Replace('[PASS] gfx->begin() with RGB bounce buffer requested', '[PASS] gfx->begin() with RGB bounce buffer DISABLED')
$dst = $dst.Replace('LVGL9 + 2x internal SRAM + RGB bounce buffer', 'LVGL9 + 2x internal SRAM + NO bounce buffer')

Set-Content -Path $outFile -Value $dst -Encoding UTF8

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 22 ==="
Write-Host "Source baseline : $sourceFile"
Write-Host "Generated sketch: $outFile"
Write-Host ""
Write-Host "Controlled runtime change:"
Write-Host "  RGB_BOUNCE_PIXELS: LCD_WIDTH * 20 -> 0"
Write-Host ""
Write-Host "All other intended runtime controls remain Test 19 baseline:"
Write-Host "  LVGL 9.x"
Write-Host "  2 x 20-line INTERNAL SRAM buffers"
Write-Host "  PARTIAL render mode"
Write-Host "  Arduino_GFX partial-area flush"
Write-Host "  PCLK 14 MHz"
Write-Host "  HSYNC 8/4/8"
Write-Host "  VSYNC 8/4/8"
Write-Host "  pclk_active_neg = 1"
Write-Host "  BSP GT911"
Write-Host "  event-driven UI"
Write-Host ""
Write-Host "Open this generated sketch in Arduino IDE:"
Write-Host "  $outFile"
