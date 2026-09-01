$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$test23Prep = Join-Path $repoRoot "controlled_tests\23_LVGL9_ArduinoGFX_PSramBuffers_Isolation\prepare-test23.ps1"
$test23Dir = Join-Path $repoRoot ".controlled-test-work\23_LVGL9_ArduinoGFX_PSramBuffers_Isolation"
$test23File = Join-Path $test23Dir "23_LVGL9_ArduinoGFX_PSramBuffers_Isolation.ino"
$outDir = Join-Path $repoRoot ".controlled-test-work\26_LVGL9_ArduinoGFX_PSram_Bounce0_PCLK12_Isolation"
$baselineFile = Join-Path $outDir "23_LVGL9_ArduinoGFX_PSramBuffers_PCLK14_VISUAL_FAIL_REFERENCE.txt"
$outFile = Join-Path $outDir "26_LVGL9_ArduinoGFX_PSram_Bounce0_PCLK12_Isolation.ino"

function Replace-Once([string]$text, [string]$old, [string]$new, [string]$label) {
    $pos = $text.IndexOf($old)
    if ($pos -lt 0) {
        throw "Missing expected token for ${label}: $old"
    }
    $next = $text.IndexOf($old, $pos + $old.Length)
    if ($next -ge 0) {
        throw "Ambiguous repeated token for ${label}: $old"
    }
    return $text.Substring(0, $pos) + $new + $text.Substring($pos + $old.Length)
}

function Require-Token([string]$text, [string]$token, [string]$label) {
    if (-not $text.Contains($token)) {
        throw "Postcondition failed for ${label}: $token"
    }
}

if (-not (Test-Path $test23Prep)) {
    throw "Test 23 generator not found: $test23Prep"
}

# Rebuild the physically observed Test 23 visual-FAIL baseline first.
& powershell -ExecutionPolicy Bypass -File $test23Prep
if ($LASTEXITCODE -ne 0) {
    throw "Test 23 baseline generator failed with exit code $LASTEXITCODE"
}
if (-not (Test-Path $test23File)) {
    throw "Generated Test 23 baseline not found: $test23File"
}

if (Test-Path $outDir) {
    Remove-Item $outDir -Recurse -Force
}
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
foreach ($token in $baselineRequired) {
    Require-Token $baseline $token "Test 23 PCLK14 visual-FAIL baseline"
}

Set-Content -Path $baselineFile -Value $baseline -Encoding UTF8

# Test 26 controlled runtime mutation: reduce only RGB pixel clock 14 MHz -> 12 MHz.
$dst = $baseline
$dst = Replace-Once $dst `
    'static constexpr uint32_t LCD_PCLK_HZ = 14000000;' `
    'static constexpr uint32_t LCD_PCLK_HZ = 12000000; // Test 26 only runtime delta: 14 MHz -> 12 MHz' `
    "PCLK 14 MHz to 12 MHz"

# Identification/reporting-only changes.
$dst = $dst.Replace('23_LVGL9_ArduinoGFX_PSramBuffers_Isolation', '26_LVGL9_ArduinoGFX_PSram_Bounce0_PCLK12_Isolation')
$dst = $dst.Replace('23-LVGL9-GFX-PSRAM1-240901A', '26-LVGL9-GFX-PSRAM-PCLK12-240901A')
$dst = $dst.Replace('ESP32-8048S043 Lab / Test 23', 'ESP32-8048S043 Lab / Test 26')

$test26Required = @(
    '26-LVGL9-GFX-PSRAM-PCLK12-240901A',
    'static constexpr uint32_t LCD_PCLK_HZ = 12000000; // Test 26 only runtime delta: 14 MHz -> 12 MHz',
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0;',
    'static uint16_t *allocateStrictPsramBuffer(const char *name) {',
    'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'LV_DISPLAY_RENDER_MODE_PARTIAL'
)
foreach ($token in $test26Required) {
    Require-Token $dst $token "generated Test 26"
}

if ($dst.Contains('static constexpr uint32_t LCD_PCLK_HZ = 14000000;')) {
    throw "Test 26 still contains the 14 MHz PCLK setting"
}
if ($dst.Contains('static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 20;')) {
    throw "Test 26 unexpectedly enabled RGB bounce buffering"
}
if ($dst.Contains('MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT')) {
    throw "Test 26 unexpectedly moved LVGL draw buffers to INTERNAL SRAM"
}

Set-Content -Path $outFile -Value $dst -Encoding UTF8

$inoFiles = @(Get-ChildItem -Path $outDir -Filter *.ino -File)
if ($inoFiles.Count -ne 1 -or $inoFiles[0].FullName -ne $outFile) {
    throw "Generated Test 26 directory must contain exactly one .ino: $outFile"
}

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 26 ==="
Write-Host "FAIL baseline     : $baselineFile"
Write-Host "Generated Test 26 : $outFile"
Write-Host ""
Write-Host "[PASS] Test23 visual-FAIL controls retained:"
Write-Host "       LVGL 9.1 / explicit RGB565"
Write-Host "       2 x 20-line draw buffers / 32000 bytes each"
Write-Host "       LVGL draw buffers = PSRAM"
Write-Host "       PARTIAL render mode"
Write-Host "       Arduino_GFX partial-area flush"
Write-Host "       RGB bounce = 0"
Write-Host "       HSYNC 8/4/8"
Write-Host "       VSYNC 8/4/8"
Write-Host "       pclk_active_neg = 1"
Write-Host "       BSP GT911"
Write-Host "       event-driven UI"
Write-Host ""
Write-Host "[PASS] Test26 only runtime delta:"
Write-Host "       PCLK 14 MHz -> 12 MHz"
Write-Host ""
Write-Host "[PASS] Arduino sketch hygiene:"
Write-Host "       exactly one .ino in generated directory"
Write-Host ""
Write-Host "Decision:"
Write-Host "       Test23 PSRAM + bounce0 + PCLK14 = touch flicker"
Write-Host "       Test26 PSRAM + bounce0 + PCLK12 = physical verdict required"
Write-Host ""
Write-Host "Open this generated sketch in Arduino IDE:"
Write-Host "  $outFile"
