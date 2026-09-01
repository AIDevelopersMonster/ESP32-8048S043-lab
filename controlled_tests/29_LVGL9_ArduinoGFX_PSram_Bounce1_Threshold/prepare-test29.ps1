$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$test23Prep = Join-Path $repoRoot "controlled_tests\23_LVGL9_ArduinoGFX_PSramBuffers_Isolation\prepare-test23.ps1"
$test23Dir = Join-Path $repoRoot ".controlled-test-work\23_LVGL9_ArduinoGFX_PSramBuffers_Isolation"
$test23File = Join-Path $test23Dir "23_LVGL9_ArduinoGFX_PSramBuffers_Isolation.ino"
$outDir = Join-Path $repoRoot ".controlled-test-work\29_LVGL9_ArduinoGFX_PSram_Bounce1_Threshold"
$baselineFile = Join-Path $outDir "23_LVGL9_ArduinoGFX_PSramBuffers_Bounce0_VISUAL_FAIL_REFERENCE.txt"
$outFile = Join-Path $outDir "29_LVGL9_ArduinoGFX_PSram_Bounce1_Threshold.ino"

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

# Rebuild the physically observed Test 23 visual-FAIL baseline.
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

# Test 29: change exactly one runtime parameter: RGB bounce 0 -> 1 display line.
$dst = $baseline
$dst = Replace-Once $dst `
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0; // Test 22 only runtime change: bounce disabled' `
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 1; // Test 29 only runtime delta: bounce = 1 line' `
    "RGB bounce 0 to 1 line"

# Identification/reporting-only changes.
$dst = $dst.Replace('23_LVGL9_ArduinoGFX_PSramBuffers_Isolation', '29_LVGL9_ArduinoGFX_PSram_Bounce1_Threshold')
$dst = $dst.Replace('23-LVGL9-GFX-PSRAM1-240901A', '29-LVGL9-GFX-PSRAM-BOUNCE1-240901A')
$dst = $dst.Replace('ESP32-8048S043 Lab / Test 23', 'ESP32-8048S043 Lab / Test 29')
$dst = $dst.Replace('double PSRAM buffers + NO RGB bounce buffer', 'double PSRAM buffers + RGB bounce 1 line')
$dst = $dst.Replace('RGB bounce buffer = DISABLED;', 'RGB bounce buffer = 1 display line;')
$dst = $dst.Replace('0U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)', '1U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)')
$dst = $dst.Replace('[PASS] gfx->begin() with RGB bounce buffer DISABLED', '[PASS] gfx->begin() with RGB bounce buffer requested')
$dst = $dst.Replace('2x PSRAM + NO bounce buffer', '2x PSRAM + RGB bounce 1 line')

$required = @(
    '29-LVGL9-GFX-PSRAM-BOUNCE1-240901A',
    'static constexpr uint32_t LCD_PCLK_HZ = 14000000;',
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 1; // Test 29 only runtime delta: bounce = 1 line',
    'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'LV_DISPLAY_RENDER_MODE_PARTIAL',
    '1U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)'
)
foreach ($token in $required) { Require-Token $dst $token "generated Test 29" }

if ($dst.Contains('static constexpr uint32_t RGB_BOUNCE_PIXELS = 0;')) { throw "Test 29 still contains bounce0" }
if ($dst.Contains('MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT')) { throw "Test 29 unexpectedly moved LVGL buffers to INTERNAL SRAM" }

Set-Content -Path $outFile -Value $dst -Encoding UTF8

$inoFiles = @(Get-ChildItem -Path $outDir -Filter *.ino -File)
if ($inoFiles.Count -ne 1 -or $inoFiles[0].FullName -ne $outFile) {
    throw "Generated Test 29 directory must contain exactly one .ino: $outFile"
}

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 29 ==="
Write-Host "FAIL baseline     : $baselineFile"
Write-Host "Generated Test 29 : $outFile"
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
Write-Host "[PASS] Test29 only runtime delta:"
Write-Host "       RGB bounce 0 -> 1 display line"
Write-Host ""
Write-Host "Scope note:"
Write-Host "       this is a lower-bound characterization of this exact firmware/board path"
Write-Host "       it is not a universal ESP32-S3 or LVGL rule"
Write-Host ""
Write-Host "Open this generated sketch in Arduino IDE:"
Write-Host "  $outFile"
