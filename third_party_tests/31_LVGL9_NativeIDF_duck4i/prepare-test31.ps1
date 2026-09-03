param()

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$workRoot = Join-Path $repoRoot ".third-party-work\31_LVGL9_NativeIDF_duck4i"
$upstreamDir = Join-Path $workRoot "upstream"
$repoUrl = "https://github.com/duck4i/esp32_8048S043-ST7262_GT911.git"
$pinnedCommit = "578966c969577309b37cf9afb698852e2e81491b"

New-Item -ItemType Directory -Force -Path $workRoot | Out-Null

if (-not (Test-Path $upstreamDir)) {
    Write-Host "Cloning duck4i upstream..."
    git clone $repoUrl $upstreamDir
    if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
}

Push-Location $upstreamDir
try {
    git fetch origin
    if ($LASTEXITCODE -ne 0) { throw "git fetch failed" }

    git checkout --detach $pinnedCommit
    if ($LASTEXITCODE -ne 0) { throw "git checkout pinned commit failed" }

    git reset --hard $pinnedCommit
    if ($LASTEXITCODE -ne 0) { throw "git reset failed" }

    git clean -fd
    if ($LASTEXITCODE -ne 0) { throw "git clean failed" }

    $head = (git rev-parse HEAD).Trim()
    if ($head -ne $pinnedCommit) {
        throw "Unexpected upstream HEAD: $head"
    }

    $trackedChanges = @(git status --porcelain --untracked-files=no)
    if ($trackedChanges.Count -ne 0) {
        throw "Pinned upstream checkout is not clean"
    }

    $lock = Get-Content (Join-Path $upstreamDir "st7262\dependencies.lock") -Raw
    foreach ($token in @("version: 5.3.2", "version: 9.2.2", "version: 2.5.0", "target: esp32s3")) {
        if (-not $lock.Contains($token)) {
            throw "dependencies.lock verification failed: missing '$token'"
        }
    }

    $main = Get-Content (Join-Path $upstreamDir "st7262\main\main.c") -Raw
    foreach ($token in @("MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT", "LV_DISP_RENDER_MODE_PARTIAL", "esp_lcd_panel_st7262_draw_bitmap", "lv_demo_widgets()")) {
        if (-not $main.Contains($token)) {
            throw "main.c architecture verification failed: missing '$token'"
        }
    }

    $driver = Get-Content (Join-Path $upstreamDir "st7262\components\esp_lcd_st7262\esp_lcd_st7262.c") -Raw
    foreach ($token in @(".fb_in_psram = true", ".double_fb = false", "esp_lcd_new_rgb_panel")) {
        if (-not $driver.Contains($token)) {
            throw "ST7262 driver verification failed: missing '$token'"
        }
    }

    Write-Host "[PASS] Upstream commit: $head"
    Write-Host "[PASS] Upstream tracked tree clean"
    Write-Host "[PASS] IDF 5.3.2 / LVGL 9.2.2 / esp_lvgl_port 2.5.0 lock verified"
    Write-Host "[PASS] Native RGB + PSRAM framebuffer + INTERNAL partial LVGL buffer architecture verified"
}
finally {
    Pop-Location
}

$baseline = @"
TEST=31_LVGL9_NativeIDF_duck4i
UPSTREAM=$repoUrl
COMMIT=$pinnedCommit
PROJECT_SUBDIR=st7262
EXPECTED_IDF=5.3.2
EXPECTED_LVGL=9.2.2
EXPECTED_ESP_LVGL_PORT=2.5.0
"@

Set-Content -Path (Join-Path $workRoot "BASELINE.txt") -Value $baseline -Encoding ASCII -NoNewline
Write-Host "[PASS] Baseline recorded outside upstream source"
