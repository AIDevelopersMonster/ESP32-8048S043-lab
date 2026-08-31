$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$sourceDir = Join-Path $repoRoot "libraries\ESP32_8048S043\examples\19_LVGL9_ArduinoGFX_BounceBufferUI"
$sourceFile = Join-Path $sourceDir "19_LVGL9_ArduinoGFX_BounceBufferUI.ino"
$outDir = Join-Path $repoRoot ".controlled-test-work\22_LVGL9_ArduinoGFX_NoBounce_Isolation"
$baselineFile = Join-Path $outDir "19_LVGL9_ArduinoGFX_BounceBufferUI_PHYSICAL_PASS_RECONSTRUCTED.ino"
$outFile = Join-Path $outDir "22_LVGL9_ArduinoGFX_NoBounce_Isolation.ino"

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

if (-not (Test-Path $sourceFile)) {
    throw "Repository Test 19 source not found: $sourceFile"
}

if (Test-Path $outDir) {
    Remove-Item $outDir -Recurse -Force
}
New-Item -ItemType Directory -Force $outDir | Out-Null

# The repository Test 19 file predates the exact physical-PASS corrections used on Sample A.
# Normalize the input, reconstruct that verified RGB565 baseline first, then change only
# the RGB bounce-buffer setting for Test 22.
$src = (Get-Content $sourceFile -Raw) -replace "`r`n", "`n"
$src = $src -replace "`r", "`n"

$baseline = $src

# Replace only the preprocessor condition line. This intentionally avoids a multiline
# literal match so Git autocrlf / Windows CRLF cannot affect the reconstruction.
$newGuardPrefix = "#ifndef LVGL_VERSION_MAJOR`n#error `"LVGL version macros not found. Check which lvgl.h Arduino IDE is actually using.`"`n#elif LVGL_VERSION_MAJOR != 9"
$baseline = Replace-Once $baseline `
    '#if LV_VERSION_MAJOR != 9' `
    $newGuardPrefix `
    "LVGL 9 version guard"

$baseline = Replace-Once $baseline `
    'static constexpr size_t LVGL_BUFFER_BYTES = LVGL_BUFFER_PIXELS * sizeof(lv_color_t);' `
    'static constexpr size_t LVGL_BUFFER_BYTES = LVGL_BUFFER_PIXELS * sizeof(uint16_t);' `
    "RGB565 draw-buffer byte count"
$baseline = Replace-Once $baseline `
    'static lv_color_t *lvBufA = nullptr;' `
    'static uint16_t *lvBufA = nullptr;' `
    "LVGL buffer A type"
$baseline = Replace-Once $baseline `
    'static lv_color_t *lvBufB = nullptr;' `
    'static uint16_t *lvBufB = nullptr;' `
    "LVGL buffer B type"
$baseline = Replace-Once $baseline `
    'static lv_color_t *allocateStrictInternalBuffer(const char *name) {' `
    'static uint16_t *allocateStrictInternalBuffer(const char *name) {' `
    "strict internal allocator return type"
$baseline = Replace-Once $baseline `
    '  lv_color_t *ptr = static_cast<lv_color_t *>(' `
    '  uint16_t *ptr = static_cast<uint16_t *>(' `
    "strict internal allocator pointer type"
$baseline = Replace-Once $baseline `
    '  lv_display_set_flush_cb(display, lvglFlush);' `
    "  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);`n  lv_display_set_flush_cb(display, lvglFlush);" `
    "explicit LVGL RGB565 display format"

$oldDiag = '  Serial.printf("%-28s: %d\n", "LV_COLOR_DEPTH", LV_COLOR_DEPTH);'
$newDiag = "  Serial.printf(`"%-28s: %d bytes\n`", `"sizeof(lv_color_t)`",`n                static_cast<int>(sizeof(lv_color_t)));`n  Serial.printf(`"%-28s: %d\n`", `"LV_COLOR_DEPTH`", LV_COLOR_DEPTH);`n  Serial.printf(`"%-28s: RGB565 / 2 bytes px\n`", `"LVGL display format`");"
$baseline = Replace-Once $baseline $oldDiag $newDiag "physical-PASS LVGL diagnostics"

$baselineRequired = @(
    '#ifndef LVGL_VERSION_MAJOR',
    'LVGL_VERSION_MAJOR != 9',
    'static constexpr size_t LVGL_BUFFER_BYTES = LVGL_BUFFER_PIXELS * sizeof(uint16_t);',
    'static uint16_t *lvBufA = nullptr;',
    'static uint16_t *lvBufB = nullptr;',
    'static uint16_t *allocateStrictInternalBuffer(const char *name) {',
    'uint16_t *ptr = static_cast<uint16_t *>(',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 20;'
)
foreach ($token in $baselineRequired) {
    Require-Token $baseline $token "reconstructed Test 19 physical-PASS baseline"
}

if ($baseline.Contains('#if LV_VERSION_MAJOR != 9')) {
    throw "Stale LV_VERSION_MAJOR guard remains in reconstructed Test 19 baseline"
}
if ($baseline.Contains('LVGL_BUFFER_PIXELS * sizeof(lv_color_t)')) {
    throw "Stale sizeof(lv_color_t) draw-buffer sizing remains in reconstructed Test 19 baseline"
}

Set-Content -Path $baselineFile -Value $baseline -Encoding UTF8

# Test 22 controlled runtime mutation: disable RGB bounce buffering and nothing else
# that affects rendering behavior. Identification and diagnostic text are updated separately.
$dst = Replace-Once $baseline `
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 20;' `
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0; // Test 22 only runtime change: bounce disabled' `
    "disable RGB bounce buffer"

# Identification/reporting-only changes.
$dst = $dst.Replace('19_LVGL9_ArduinoGFX_BounceBufferUI', '22_LVGL9_ArduinoGFX_NoBounce_Isolation')
$dst = $dst.Replace('19-LVGL9-GFX-BOUNCE1-240830A', '22-LVGL9-GFX-NOBOUNCE1-240901A')
$dst = $dst.Replace('ESP32-8048S043 Lab / Test 19', 'ESP32-8048S043 Lab / Test 22')
$dst = $dst.Replace('LVGL9 + double internal SRAM buffers + RGB bounce buffer', 'LVGL9 + double internal SRAM buffers + NO RGB bounce buffer')
$dst = $dst.Replace('RGB bounce buffer = 20 display lines;', 'RGB bounce buffer = DISABLED;')
$dst = $dst.Replace('20U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)', '0U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS)')
$dst = $dst.Replace('[PASS] gfx->begin() with RGB bounce buffer requested', '[PASS] gfx->begin() with RGB bounce buffer DISABLED')
$dst = $dst.Replace('LVGL9 + 2x internal SRAM + RGB bounce buffer', 'LVGL9 + 2x internal SRAM + NO bounce buffer')

$test22Required = @(
    '#ifndef LVGL_VERSION_MAJOR',
    'LVGL_VERSION_MAJOR != 9',
    'static constexpr size_t LVGL_BUFFER_BYTES = LVGL_BUFFER_PIXELS * sizeof(uint16_t);',
    'static uint16_t *lvBufA = nullptr;',
    'static uint16_t *lvBufB = nullptr;',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0; // Test 22 only runtime change: bounce disabled',
    '22-LVGL9-GFX-NOBOUNCE1-240901A'
)
foreach ($token in $test22Required) {
    Require-Token $dst $token "generated Test 22"
}

if ($dst.Contains('static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 20;')) {
    throw "Test 22 still contains the enabled 20-line RGB bounce setting"
}
if ($dst.Contains('LVGL_BUFFER_PIXELS * sizeof(lv_color_t)')) {
    throw "Test 22 regressed to sizeof(lv_color_t) buffer sizing"
}

Set-Content -Path $outFile -Value $dst -Encoding UTF8

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 22 ==="
Write-Host "Repository source : $sourceFile"
Write-Host "PASS reference    : $baselineFile"
Write-Host "Generated Test 22 : $outFile"
Write-Host ""
Write-Host "[PASS] Reconstructed physical-PASS Test19 baseline:"
Write-Host "       LVGL_VERSION_MAJOR guard"
Write-Host "       explicit LV_COLOR_FORMAT_RGB565"
Write-Host "       uint16_t RGB565 draw buffers"
Write-Host "       2 x 20 lines / 32000 bytes each"
Write-Host "       INTERNAL SRAM required"
Write-Host ""
Write-Host "[PASS] Test22 only runtime delta:"
Write-Host "       RGB bounce 20 lines -> 0"
Write-Host ""
Write-Host "Retained controls:"
Write-Host "       PARTIAL render mode"
Write-Host "       Arduino_GFX partial-area flush"
Write-Host "       PCLK 14 MHz"
Write-Host "       HSYNC 8/4/8"
Write-Host "       VSYNC 8/4/8"
Write-Host "       pclk_active_neg = 1"
Write-Host "       BSP GT911"
Write-Host "       event-driven UI"
Write-Host ""
Write-Host "Open this generated sketch in Arduino IDE:"
Write-Host "  $outFile"
