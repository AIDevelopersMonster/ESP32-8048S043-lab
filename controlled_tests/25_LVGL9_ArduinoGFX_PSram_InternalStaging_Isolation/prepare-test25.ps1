$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$test23Prep = Join-Path $repoRoot "controlled_tests\23_LVGL9_ArduinoGFX_PSramBuffers_Isolation\prepare-test23.ps1"
$test23Dir = Join-Path $repoRoot ".controlled-test-work\23_LVGL9_ArduinoGFX_PSramBuffers_Isolation"
$test23File = Join-Path $test23Dir "23_LVGL9_ArduinoGFX_PSramBuffers_Isolation.ino"
$outDir = Join-Path $repoRoot ".controlled-test-work\25_LVGL9_ArduinoGFX_PSram_InternalStaging_Isolation"
$baselineFile = Join-Path $outDir "23_LVGL9_ArduinoGFX_PSramBuffers_VISUAL_FAIL_REFERENCE.txt"
$outFile = Join-Path $outDir "25_LVGL9_ArduinoGFX_PSram_InternalStaging_Isolation.ino"

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
    'static uint16_t *lvBufA = nullptr;',
    'static uint16_t *lvBufB = nullptr;',
    'static uint16_t *allocateStrictPsramBuffer(const char *name) {',
    'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT',
    'if (!esp_ptr_external_ram(ptr))',
    'reinterpret_cast<uint16_t *>(pxMap)',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'LV_DISPLAY_RENDER_MODE_PARTIAL'
)
foreach ($token in $baselineRequired) {
    Require-Token $baseline $token "Test 23 visual-FAIL baseline"
}

Set-Content -Path $baselineFile -Value $baseline -Encoding UTF8

$dst = $baseline

# Test 25 controlled runtime mutation:
# keep LVGL draw buffers in PSRAM and RGB bounce disabled, but insert exactly one
# INTERNAL SRAM transfer buffer between the LVGL PSRAM chunk and Arduino_GFX.
$dst = Replace-Once $dst `
    "static uint16_t *lvBufB = nullptr;" `
    "static uint16_t *lvBufB = nullptr;`nstatic uint16_t *flushStage = nullptr;" `
    "add INTERNAL flush staging pointer"

$psramAllocatorEnd = @'
  Serial.printf("[PASS] %s PSRAM: %u bytes at %p\n",
                name, static_cast<unsigned>(LVGL_BUFFER_BYTES), ptr);
  return ptr;
}
'@
$stageAllocator = @'
  Serial.printf("[PASS] %s PSRAM: %u bytes at %p\n",
                name, static_cast<unsigned>(LVGL_BUFFER_BYTES), ptr);
  return ptr;
}

static uint16_t *allocateStrictInternalStageBuffer(const char *name) {
  uint16_t *ptr = static_cast<uint16_t *>(
    heap_caps_malloc(LVGL_BUFFER_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
  );

  if (!ptr) {
    Serial.printf("[FAIL] %s INTERNAL staging allocation failed: %u bytes\n",
                  name, static_cast<unsigned>(LVGL_BUFFER_BYTES));
    return nullptr;
  }

  if (!esp_ptr_internal(ptr)) {
    Serial.printf("[FAIL] %s staging allocation is not INTERNAL SRAM: %p\n", name, ptr);
    heap_caps_free(ptr);
    return nullptr;
  }

  Serial.printf("[PASS] %s INTERNAL staging: %u bytes at %p\n",
                name, static_cast<unsigned>(LVGL_BUFFER_BYTES), ptr);
  return ptr;
}
'@
$dst = Replace-Once $dst $psramAllocatorEnd $stageAllocator "add strict INTERNAL staging allocator"

$oldInitBlock = @'
  lvBufA = allocateStrictPsramBuffer("LVGL buffer A");
  lvBufB = allocateStrictPsramBuffer("LVGL buffer B");

  if (!lvBufA || !lvBufB) {
    Serial.println("[STOP] Strict double-buffer PSRAM condition not met");
    return false;
  }

  display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
'@
$newInitBlock = @'
  lvBufA = allocateStrictPsramBuffer("LVGL buffer A");
  lvBufB = allocateStrictPsramBuffer("LVGL buffer B");

  if (!lvBufA || !lvBufB) {
    Serial.println("[STOP] Strict double-buffer PSRAM condition not met");
    return false;
  }

  flushStage = allocateStrictInternalStageBuffer("LVGL flush stage");
  if (!flushStage) {
    Serial.println("[STOP] Strict INTERNAL flush-staging condition not met");
    return false;
  }

  display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
'@
$dst = Replace-Once $dst $oldInitBlock $newInitBlock "allocate INTERNAL staging during LVGL init"

$oldFlush = @'
static void lvglFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *pxMap) {
  flushCount++;

  if (gfx) {
    const int32_t w = area->x2 - area->x1 + 1;
    const int32_t h = area->y2 - area->y1 + 1;

    gfx->draw16bitRGBBitmap(
      area->x1,
      area->y1,
      reinterpret_cast<uint16_t *>(pxMap),
      w,
      h
    );
  }

  lv_display_flush_ready(disp);
}
'@
$newFlush = @'
static void lvglFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *pxMap) {
  flushCount++;

  if (gfx) {
    const int32_t w = area->x2 - area->x1 + 1;
    const int32_t h = area->y2 - area->y1 + 1;
    const size_t pixelCount = static_cast<size_t>(w) * static_cast<size_t>(h);
    const size_t copyBytes = pixelCount * sizeof(uint16_t);

    if (!flushStage || pixelCount > LVGL_BUFFER_PIXELS) {
      Serial.printf("[FAIL] flush staging bounds: %ldx%ld = %lu px, capacity=%lu px\n",
                    static_cast<long>(w), static_cast<long>(h),
                    static_cast<unsigned long>(pixelCount),
                    static_cast<unsigned long>(LVGL_BUFFER_PIXELS));
      lv_display_flush_ready(disp);
      return;
    }

    memcpy(flushStage, pxMap, copyBytes);

    gfx->draw16bitRGBBitmap(
      area->x1,
      area->y1,
      flushStage,
      w,
      h
    );
  }

  lv_display_flush_ready(disp);
}
'@
$dst = Replace-Once $dst $oldFlush $newFlush "stage PSRAM flush chunk through INTERNAL SRAM"

# Identification/reporting-only changes.
$dst = $dst.Replace('23_LVGL9_ArduinoGFX_PSramBuffers_Isolation', '25_LVGL9_ArduinoGFX_PSram_InternalStaging_Isolation')
$dst = $dst.Replace('23-LVGL9-GFX-PSRAM1-240901A', '25-LVGL9-GFX-PSRAM-STAGE1-240901A')
$dst = $dst.Replace('ESP32-8048S043 Lab / Test 23', 'ESP32-8048S043 Lab / Test 25')
$dst = $dst.Replace('double PSRAM buffers + NO RGB bounce buffer', 'double PSRAM buffers + INTERNAL flush staging + NO RGB bounce buffer')
$dst = $dst.Replace('2x PSRAM + NO bounce buffer', '2x PSRAM + INTERNAL staging + NO bounce buffer')

# Add one diagnostic line without changing runtime behavior.
$diagAnchor = '  Serial.printf("%-28s: PSRAM required\n", "LVGL buffer policy");'
$diagReplacement = "  Serial.printf(`"%-28s: PSRAM required\n`", `"LVGL buffer policy`");`n  Serial.printf(`"%-28s: INTERNAL SRAM / %lu bytes\n`", `"Flush staging`",`n                static_cast<unsigned long>(LVGL_BUFFER_BYTES));"
$dst = Replace-Once $dst $diagAnchor $diagReplacement "flush staging diagnostic"

$test25Required = @(
    '25-LVGL9-GFX-PSRAM-STAGE1-240901A',
    'static constexpr uint32_t RGB_BOUNCE_PIXELS = 0;',
    'static uint16_t *allocateStrictPsramBuffer(const char *name) {',
    'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT',
    'static uint16_t *flushStage = nullptr;',
    'static uint16_t *allocateStrictInternalStageBuffer(const char *name) {',
    'MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT',
    'if (!esp_ptr_internal(ptr))',
    'flushStage = allocateStrictInternalStageBuffer("LVGL flush stage");',
    'memcpy(flushStage, pxMap, copyBytes);',
    'gfx->draw16bitRGBBitmap(',
    '      flushStage,',
    'lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);',
    'LV_DISPLAY_RENDER_MODE_PARTIAL'
)
foreach ($token in $test25Required) {
    Require-Token $dst $token "generated Test 25"
}

if ($dst.Contains('static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 20;')) {
    throw "Test 25 unexpectedly enabled RGB bounce buffering"
}
if ($dst.Contains('reinterpret_cast<uint16_t *>(pxMap),')) {
    throw "Test 25 still passes LVGL PSRAM pxMap directly to Arduino_GFX"
}

Set-Content -Path $outFile -Value $dst -Encoding UTF8

# Arduino compiles every .ino in a sketch directory: keep exactly one.
$inoFiles = @(Get-ChildItem -Path $outDir -Filter *.ino -File)
if ($inoFiles.Count -ne 1 -or $inoFiles[0].FullName -ne $outFile) {
    throw "Generated Test 25 directory must contain exactly one .ino: $outFile"
}

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 25 ==="
Write-Host "FAIL baseline     : $baselineFile"
Write-Host "Generated Test 25 : $outFile"
Write-Host ""
Write-Host "[PASS] Test23 visual-FAIL controls retained:"
Write-Host "       LVGL 9.1 / explicit RGB565"
Write-Host "       2 x 20-line draw buffers / 32000 bytes each"
Write-Host "       LVGL draw buffers = PSRAM"
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
Write-Host "[PASS] Test25 only runtime delta:"
Write-Host "       one 32000-byte INTERNAL staging buffer"
Write-Host "       PSRAM pxMap -> memcpy -> INTERNAL -> draw16bitRGBBitmap()"
Write-Host ""
Write-Host "[PASS] Arduino sketch hygiene:"
Write-Host "       exactly one .ino in generated directory"
Write-Host ""
Write-Host "Decision:"
Write-Host "       Test23 PSRAM + bounce0 + direct pxMap = flicker"
Write-Host "       Test25 PSRAM + bounce0 + INTERNAL staging = physical verdict required"
Write-Host ""
Write-Host "Open this generated sketch in Arduino IDE:"
Write-Host "  $outFile"
