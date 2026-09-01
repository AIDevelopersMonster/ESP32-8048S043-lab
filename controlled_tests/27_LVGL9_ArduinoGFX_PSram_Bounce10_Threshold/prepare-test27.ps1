$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$test23Prep = Join-Path $repoRoot "controlled_tests\23_LVGL9_ArduinoGFX_PSramBuffers_Isolation\prepare-test23.ps1"
$test23Dir = Join-Path $repoRoot ".controlled-test-work\23_LVGL9_ArduinoGFX_PSramBuffers_Isolation"
$test23File = Join-Path $test23Dir "23_LVGL9_ArduinoGFX_PSramBuffers_Isolation.ino"
$outDir = Join-Path $repoRoot ".controlled-test-work\27_LVGL9_ArduinoGFX_PSram_Bounce10_Threshold"
$baselineFile = Join-Path $outDir "23_LVGL9_ArduinoGFX_PSramBuffers_Bounce0_VISUAL_FAIL_REFERENCE.txt"
$outFile = Join-Path $outDir "27_LVGL9_ArduinoGFX_PSram_Bounce10_Threshold.ino"

function Replace-Once([string]$text, [string]$old, [string]$new, [string]$label) {
    $pos = $text.IndexOf($old)
    if ($pos -lt 0) { throw "Missing expected token for ${label}: $old" }
    $next = $text.IndexOf($old, $pos + $old.Length)
    if ($next -ge 0) { throw "Ambiguous repeated token for ${label}: $old" }
    return $text.Substring(0, $pos) + $new + $text.Substring($pos + $old.Length)
}

function Require-Token([string]$text, [string]$token, [string]$label) {
    if (-not $text.Contains($token)) { throw "Postcondition failed for ${label}: $token" }
}

if (-not (Test-Path $test23Prep)) { throw "Test 23 generator not found: $test23Prep" }

& powershell -ExecutionPolicy Bypass -File $test23Prep
if ($LASTEXITCODE -ne 0) { throw "Test 23 baseline generator failed with exit code $LASTEXITCODE" }
if (-not (Test-Path $test23File)) { throw "Generated Test 23 baseline not found: $test23File" }

if (Test-Path $outDir) { Remove-Item $outDir -Recurse -Force }
New-Item -ItemType Directory -Force $outDir | Out-Null

$baseline = (Get-Content $test23File -Raw) -replace "`r`n", "`n"
$baseline = $baseline -replace "`r", "`n"

$baselineRequired = @(
    '23-LVGL9-GFX-PSRAM1-240901A',
    'static constexpr uint32_t LCD_PCLK_HZ = 14000000;',
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0;',
    'static uint16_t *allocateStrictPsramBuffer(const char *name) {',
    'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'LV_DISPLAY_RENDER_MODE_PARTIAL'
)
foreach ($token in $baselineRequired) { Require-Token $baseline $token "Test 23 visual-FAIL baseline" }

Set-Content -Path $baselineFile -Value $baseline -Encoding UTF8

$dst = $baseline
$dst = Replace-Once $dst `
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0; // Test 22 only runtime change: bounce disabled' `
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 10; // Test 27 only runtime delta: bounce = 10 lines' `
    "RGB bounce 0 to 10 lines"

$dst = $dst.Replace('23_LVGL9_ArduinoGFX_PSramBuffers_Isolation', '27_LVGL9_ArduinoGFX_PSram_Bounce10_Threshold')
$dst = $dst.Replace('23-LVGL9-GFX-PSRAM1-240901A', '27-LVGL9-GFX-PSRAM-BOUNCE10-240901A')
$dst = $dst.Replace('ESP32-8048S043 Lab / Test 23', 'ESP32-8048S043 Lab / Test 27')
$dst = $dst.Replace('double PSRAM buffers + NO RGB bounce buffer', 'double PSRAM buffers + RGB bounce 10 lines')
$dst = $dst.Replace('RGB bounce buffer = DISABLED;', 'RGB bounce buffer = 10 display lines;')
$dst = $dst.Replace('0U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)', '10U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)')
$dst = $dst.Replace('[PASS] gfx->begin() with RGB bounce buffer DISABLED', '[PASS] gfx->begin() with RGB bounce buffer requested')
$dst = $dst.Replace('2x PSRAM + NO bounce buffer', '2x PSRAM + RGB bounce 10 lines')

$required = @(
    '27-LVGL9-GFX-PSRAM-BOUNCE10-240901A',
    'static constexpr uint32_t LCD_PCLK_HZ = 14000000;',
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 10; // Test 27 only runtime delta: bounce = 10 lines',
    'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'LV_DISPLAY_RENDER_MODE_PARTIAL',
    '10U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)'
)
foreach ($token in $required) { Require-Token $dst $token "generated Test 27" }

if ($dst.Contains('static constexpr uint32_t RGB_BOUNCE_PIXELS = 0;')) { throw "Test 27 still contains bounce0" }
if ($dst.Contains('MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT')) { throw "Test 27 unexpectedly moved LVGL buffers to INTERNAL SRAM" }

Set-Content -Path $outFile -Value $dst -Encoding UTF8

$inoFiles = @(Get-ChildItem -Path $outDir -Filter *.ino -File)
if ($inoFiles.Count -ne 1 -or $inoFiles[0].FullName -ne $outFile) {
    throw "Generated Test 27 directory must contain exactly one .ino: $outFile"
}

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 27 ==="
Write-Host "FAIL baseline     : $baselineFile"
Write-Host "Generated Test 27 : $outFile"
Write-Host ""
Write-Host "[PASS] Test23 visual-FAIL controls retained:"
Write-Host "       LVGL 9.1 / explicit RGB565"
Write-Host "       2 x 20-line draw buffers / 32000 bytes each"
Write-Host "       LVGL draw buffers = PSRAM"
Write-Host "       PARTIAL render mode"
Write-Host "       Arduino_GFX partial-area flush"
Write-Host "       PCLK 14 MHz"
Write-Host "       HSYNC 8/4/8"
Write-Host "       VSYNC 8/4/8"
Write-Host "       pclk_active_neg = 1"
Write-Host "       BSP GT911"
Write-Host "       event-driven UI"
Write-Host ""
Write-Host "[PASS] Test27 only runtime delta:"
Write-Host "       RGB bounce 0 -> 10 display lines"
Write-Host ""
Write-Host "Threshold matrix:"
Write-Host "       bounce0  = FAIL / touch flicker"
Write-Host "       bounce10 = physical verdict required"
Write-Host "       bounce20 = PASS"
Write-Host ""
Write-Host "Open this generated sketch in Arduino IDE:"
Write-Host "  $outFile"
