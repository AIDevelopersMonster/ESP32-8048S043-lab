param(
    [string]$WorkRoot = "$HOME\t36b-touch-sleep16"
)

$ErrorActionPreference = 'Stop'

$RepoUrl = 'https://github.com/halyssonJr/lvgl-demo-esp32s3.git'
$Commit = '79e862ca332525ba8721c4691f450fb44ec08738'
$Upstream = Join-Path $WorkRoot 'upstream'

New-Item -ItemType Directory -Path $WorkRoot -Force | Out-Null

if (-not (Test-Path $Upstream)) {
    git clone $RepoUrl $Upstream
    if ($LASTEXITCODE -ne 0) { throw 'git clone failed' }
}

Push-Location $Upstream
try {
    git fetch --all --tags --prune
    if ($LASTEXITCODE -ne 0) { throw 'git fetch failed' }
    git reset --hard
    if ($LASTEXITCODE -ne 0) { throw 'git reset failed' }
    git clean -ffd -e build/ -e managed_components/
    if ($LASTEXITCODE -ne 0) { throw 'git clean failed' }
    git checkout --detach $Commit
    if ($LASTEXITCODE -ne 0) { throw 'checkout failed' }
    git checkout -- .

    $Head = (git rev-parse HEAD).Trim()
    if ($Head -ne $Commit) { throw "Pinned commit mismatch: $Head" }

    $LcdSourcePath = Join-Path $Upstream 'main\lcd_display.c'
    $LcdHeaderPath = Join-Path $Upstream 'main\lcd_display.h'

    $SourceText = [System.IO.File]::ReadAllText($LcdSourcePath)
    $HeaderText = [System.IO.File]::ReadAllText($LcdHeaderPath)

    if (-not $SourceText.Contains('#define LVGL_TASK_SLEEP 500')) {
        throw 'Expected Test36 baseline LVGL_TASK_SLEEP 500 not found in main/lcd_display.c'
    }
    if (-not $HeaderText.Contains('#define TOUCH_GPIO_INT GPIO_NUM_NC')) {
        throw 'Expected GT911 interrupt-disabled baseline not found in main/lcd_display.h'
    }

    $Baseline = @"
Test 36B controlled derivative
Upstream: halyssonJr/lvgl-demo-esp32s3
Commit: $Commit
ONLY intended runtime source delta:
  #define LVGL_TASK_SLEEP 500
  ->
  #define LVGL_TASK_SLEEP 16

Everything else remains baseline Test 36, including:
- GT911 INT disabled in main/lcd_display.h
- I2C 400 kHz
- same coordinate mapping
- same LVGL/esp_lvgl_port versions
- same display timings
- same PSRAM framebuffers
- same bounce10
- same DIRECT/avoid_tearing
- same generated UI
"@
    [System.IO.File]::WriteAllText((Join-Path $WorkRoot 'BASELINE.txt'), $Baseline, (New-Object System.Text.UTF8Encoding($false)))

    Write-Host '[PASS] Test 36B baseline prepared'
    Write-Host '[PASS] Test 36 baseline sleep=500 verified in lcd_display.c'
    Write-Host '[PASS] GT911 interrupt-disabled baseline verified in lcd_display.h'
}
finally {
    Pop-Location
}
