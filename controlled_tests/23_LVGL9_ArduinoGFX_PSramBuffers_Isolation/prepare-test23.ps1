$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$test22Prep = Join-Path $repoRoot "controlled_tests\22_LVGL9_ArduinoGFX_NoBounce_Isolation\prepare-test22.ps1"
$test22Dir = Join-Path $repoRoot ".controlled-test-work\22_LVGL9_ArduinoGFX_NoBounce_Isolation"
$test22File = Join-Path $test22Dir "22_LVGL9_ArduinoGFX_NoBounce_Isolation.ino"
$outDir = Join-Path $repoRoot ".controlled-test-work\23_LVGL9_ArduinoGFX_PSramBuffers_Isolation"
$baselineFile = Join-Path $outDir "22_LVGL9_ArduinoGFX_NoBounce_Isolation_PHYSICAL_PASS_REFERENCE.txt"
$outFile = Join-Path $outDir "23_LVGL9_ArduinoGFX_PSramBuffers_Isolation.ino"

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

if (-not (Test-Path $test22Prep)) {
    throw "Test 22 generator not found: $test22Prep"
}

# Rebuild the already physically validated Test 22 baseline first.
& powershell -ExecutionPolicy Bypass -File $test22Prep
if ($LASTEXITCODE -ne 0) {
    throw "Test 22 baseline generator failed with exit code $LASTEXITCODE"
}
if (-not (Test-Path $test22File)) {
    throw "Generated Test 22 baseline not found: $test22File"
}

if (Test-Path $outDir) {
    Remove-Item $outDir -Recurse -Force
}
New-Item -ItemType Directory -Force $outDir | Out-Null

$baseline = (Get-Content $test22File -Raw) -replace "`r`n", "`n"
$baseline = $baseline -replace "`r", "`n"

$baselineRequired = @(
    '22-LVGL9-GFX-NOBOUNCE1-240901A',
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0;',
    'static constexpr size_t LVGL_BUFFER_BYTES = LVGL_BUFFER_PIXELS * sizeof(uint16_t);',
    'static uint16_t *allocateStrictInternalBuffer(const char *name) {',
    'MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT',
    'if (!esp_ptr_internal(ptr))',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'LV_DISPLAY_RENDER_MODE_PARTIAL'
)
foreach ($token in $baselineRequired) {
    Require-Token $baseline $token "Test 22 physical-PASS baseline"
}

Set-Content -Path $baselineFile -Value $baseline -Encoding UTF8

# Test 23 controlled runtime mutation: move both LVGL draw buffers from INTERNAL SRAM to PSRAM.
$dst = $baseline
$dst = Replace-Once $dst `
    'static uint16_t *allocateStrictInternalBuffer(const char *name) {' `
    'static uint16_t *allocateStrictPsramBuffer(const char *name) {' `
    "allocator function name"
$dst = Replace-Once $dst `
    'heap_caps_malloc(LVGL_BUFFER_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)' `
    'heap_caps_malloc(LVGL_BUFFER_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)' `
    "allocation capability INTERNAL to SPIRAM"
$dst = Replace-Once $dst `
    '    Serial.printf("[FAIL] %s internal SRAM allocation failed: %u bytes\n",' `
    '    Serial.printf("[FAIL] %s PSRAM allocation failed: %u bytes\n",' `
    "allocation failure diagnostic"
$dst = Replace-Once $dst `
    '  if (!esp_ptr_internal(ptr)) {' `
    '  if (!esp_ptr_external_ram(ptr)) {' `
    "placement verification"
$dst = Replace-Once $dst `
    '    Serial.printf("[FAIL] %s allocation is not internal SRAM: %p\n", name, ptr);' `
    '    Serial.printf("[FAIL] %s allocation is not PSRAM: %p\n", name, ptr);' `
    "placement failure diagnostic"
$dst = Replace-Once $dst `
    '  Serial.printf("[PASS] %s INTERNAL SRAM: %u bytes at %p\n",' `
    '  Serial.printf("[PASS] %s PSRAM: %u bytes at %p\n",' `
    "placement success diagnostic"
$dst = $dst.Replace('allocateStrictInternalBuffer("LVGL buffer A")', 'allocateStrictPsramBuffer("LVGL buffer A")')
$dst = $dst.Replace('allocateStrictInternalBuffer("LVGL buffer B")', 'allocateStrictPsramBuffer("LVGL buffer B")')
$dst = $dst.Replace('[STOP] Strict double-buffer internal-SRAM condition not met', '[STOP] Strict double-buffer PSRAM condition not met')
$dst = $dst.Replace('[PASS] LVGL9 partial display registered with 2 SRAM buffers', '[PASS] LVGL9 partial display registered with 2 PSRAM buffers')

# Identification/reporting-only changes.
$dst = $dst.Replace('22_LVGL9_ArduinoGFX_NoBounce_Isolation', '23_LVGL9_ArduinoGFX_PSramBuffers_Isolation')
$dst = $dst.Replace('22-LVGL9-GFX-NOBOUNCE1-240901A', '23-LVGL9-GFX-PSRAM1-240901A')
$dst = $dst.Replace('ESP32-8048S043 Lab / Test 22', 'ESP32-8048S043 Lab / Test 23')
$dst = $dst.Replace('double internal SRAM buffers + NO RGB bounce buffer', 'double PSRAM buffers + NO RGB bounce buffer')
$dst = $dst.Replace('LVGL buffer policy          : INTERNAL SRAM required', 'LVGL buffer policy          : PSRAM required')
$dst = $dst.Replace('LVGL buffer policy", "INTERNAL SRAM required', 'LVGL buffer policy", "PSRAM required')
$dst = $dst.Replace('2x internal SRAM + NO bounce buffer', '2x PSRAM + NO bounce buffer')

$test23Required = @(
    '23-LVGL9-GFX-PSRAM1-240901A',
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0;',
    'static uint16_t *allocateStrictPsramBuffer(const char *name) {',
    'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT',
    'if (!esp_ptr_external_ram(ptr))',
    'allocateStrictPsramBuffer("LVGL buffer A")',
    'allocateStrictPsramBuffer("LVGL buffer B")',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'LV_DISPLAY_RENDER_MODE_PARTIAL'
)
foreach ($token in $test23Required) {
    Require-Token $dst $token "generated Test 23"
}

if ($dst.Contains('MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT')) {
    throw "Test 23 still contains INTERNAL draw-buffer allocation capability"
}
if ($dst.Contains('allocateStrictInternalBuffer(')) {
    throw "Test 23 still contains the internal draw-buffer allocator"
}

Set-Content -Path $outFile -Value $dst -Encoding UTF8

$inoFiles = @(Get-ChildItem -Path $outDir -Filter *.ino -File)
if ($inoFiles.Count -ne 1 -or $inoFiles[0].FullName -ne $outFile) {
    throw "Generated Test 23 directory must contain exactly one .ino: $outFile"
}

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 23 ==="
Write-Host "PASS baseline     : $baselineFile"
Write-Host "Generated Test 23 : $outFile"
Write-Host ""
Write-Host "[PASS] Test22 physical-PASS controls retained:"
Write-Host "       LVGL 9.1 / explicit RGB565"
Write-Host "       2 x 20-line draw buffers / 32000 bytes each"
Write-Host "       PARTIAL render mode"
Write-Host "       Arduino_GFX partial-area flush"
Write-Host "       RGB bounce = 0"
Write-Host "       PCLK 14 MHz"
Write-Host "       HSYNC 8/4/8"
Write-Host "       VSYNC 8/4/8"
Write-Host "       pclk_active_neg = 1"
Write-Host "       BSP GT911"
Write-Host "       event-driven UI"
Write-Host ""
Write-Host "[PASS] Test23 only runtime delta:"
Write-Host "       LVGL draw buffers INTERNAL SRAM -> PSRAM"
Write-Host ""
Write-Host "[PASS] Arduino sketch hygiene:"
Write-Host "       exactly one .ino in generated directory"
Write-Host ""
Write-Host "Open this generated sketch in Arduino IDE:"
Write-Host "  $outFile"
