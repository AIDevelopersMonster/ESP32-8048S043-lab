$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$test23Prep = Join-Path $repoRoot "controlled_tests\23_LVGL9_ArduinoGFX_PSramBuffers_Isolation\prepare-test23.ps1"
$test23Dir = Join-Path $repoRoot ".controlled-test-work\23_LVGL9_ArduinoGFX_PSramBuffers_Isolation"
$test23File = Join-Path $test23Dir "23_LVGL9_ArduinoGFX_PSramBuffers_Isolation.ino"
$outDir = Join-Path $repoRoot ".controlled-test-work\24_LVGL9_ArduinoGFX_PSramBuffers_Bounce20_Isolation"
$baselineFile = Join-Path $outDir "23_LVGL9_ArduinoGFX_PSramBuffers_VISUAL_FAIL_REFERENCE.txt"
$outFile = Join-Path $outDir "24_LVGL9_ArduinoGFX_PSramBuffers_Bounce20_Isolation.ino"

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

# Rebuild the physically observed Test 23 visual-FAIL reference first.
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
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0;',
    'static constexpr size_t LVGL_BUFFER_BYTES = LVGL_BUFFER_PIXELS * sizeof(uint16_t);',
    'static uint16_t *allocateStrictPsramBuffer(const char *name) {',
    'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT',
    'if (!esp_ptr_external_ram(ptr))',
    'allocateStrictPsramBuffer("LVGL buffer A")',
    'allocateStrictPsramBuffer("LVGL buffer B")',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'LV_DISPLAY_RENDER_MODE_PARTIAL'
)
foreach ($token in $baselineRequired) {
    Require-Token $baseline $token "Test 23 visual-FAIL baseline"
}

Set-Content -Path $baselineFile -Value $baseline -Encoding UTF8

# Test 24 controlled runtime mutation: restore only the 20-line RGB bounce buffer.
# LVGL draw buffers remain strictly in PSRAM exactly as in Test 23.
$dst = $baseline
$dst = Replace-Once $dst `
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0; // Test 22 only runtime change: bounce disabled' `
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 20; // Test 24 only runtime change: bounce restored' `
    "restore 20-line RGB bounce buffer"

# Identification/reporting-only changes.
$dst = $dst.Replace('23_LVGL9_ArduinoGFX_PSramBuffers_Isolation', '24_LVGL9_ArduinoGFX_PSramBuffers_Bounce20_Isolation')
$dst = $dst.Replace('23-LVGL9-GFX-PSRAM1-240901A', '24-LVGL9-GFX-PSRAM-BOUNCE1-240901A')
$dst = $dst.Replace('ESP32-8048S043 Lab / Test 23', 'ESP32-8048S043 Lab / Test 24')
$dst = $dst.Replace('double PSRAM buffers + NO RGB bounce buffer', 'double PSRAM buffers + RGB bounce buffer')
$dst = $dst.Replace('RGB bounce buffer = DISABLED;', 'RGB bounce buffer = 20 display lines;')
$dst = $dst.Replace('0U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)', '20U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)')
$dst = $dst.Replace('[PASS] gfx->begin() with RGB bounce buffer DISABLED', '[PASS] gfx->begin() with RGB bounce buffer requested')
$dst = $dst.Replace('2x PSRAM + NO bounce buffer', '2x PSRAM + RGB bounce buffer')

$test24Required = @(
    '24-LVGL9-GFX-PSRAM-BOUNCE1-240901A',
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 20; // Test 24 only runtime change: bounce restored',
    'static constexpr size_t LVGL_BUFFER_BYTES = LVGL_BUFFER_PIXELS * sizeof(uint16_t);',
    'static uint16_t *allocateStrictPsramBuffer(const char *name) {',
    'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT',
    'if (!esp_ptr_external_ram(ptr))',
    'allocateStrictPsramBuffer("LVGL buffer A")',
    'allocateStrictPsramBuffer("LVGL buffer B")',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'LV_DISPLAY_RENDER_MODE_PARTIAL',
    '20U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)'
)
foreach ($token in $test24Required) {
    Require-Token $dst $token "generated Test 24"
}

if ($dst.Contains('static constexpr uint32_t RGB_BOUNCE_PIXELS = 0;')) {
    throw "Test 24 still contains disabled RGB bounce setting"
}
if ($dst.Contains('MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT')) {
    throw "Test 24 regressed LVGL draw buffers to INTERNAL SRAM"
}
if ($dst.Contains('allocateStrictInternalBuffer(')) {
    throw "Test 24 regressed to internal draw-buffer allocator"
}

Set-Content -Path $outFile -Value $dst -Encoding UTF8

# Arduino compiles every .ino in a sketch directory: keep exactly one.
$inoFiles = @(Get-ChildItem -Path $outDir -Filter *.ino -File)
if ($inoFiles.Count -ne 1 -or $inoFiles[0].FullName -ne $outFile) {
    throw "Generated Test 24 directory must contain exactly one .ino: $outFile"
}

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 24 ==="
Write-Host "FAIL baseline     : $baselineFile"
Write-Host "Generated Test 24 : $outFile"
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
Write-Host "[PASS] Test24 only runtime delta:"
Write-Host "       RGB bounce 0 -> 20 display lines"
Write-Host ""
Write-Host "[PASS] Arduino sketch hygiene:"
Write-Host "       exactly one .ino in generated directory"
Write-Host ""
Write-Host "A/B decision:"
Write-Host "       Test23 PSRAM + bounce0  = flicker"
Write-Host "       Test24 PSRAM + bounce20 = physical verdict required"
Write-Host ""
Write-Host "Open this generated sketch in Arduino IDE:"
Write-Host "  $outFile"
