param(
    [string]$WorkRoot = "$HOME\t36c-modern-i2c"
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

    $TouchC = [System.IO.File]::ReadAllText((Join-Path $Upstream 'main\touch_i2c.c'))
    $TouchH = [System.IO.File]::ReadAllText((Join-Path $Upstream 'main\touch_i2c.h'))
    $LcdC = [System.IO.File]::ReadAllText((Join-Path $Upstream 'main\lcd_display.c'))
    $LcdH = [System.IO.File]::ReadAllText((Join-Path $Upstream 'main\lcd_display.h'))
    $Cmake = [System.IO.File]::ReadAllText((Join-Path $Upstream 'main\CMakeLists.txt'))

    if (-not $TouchH.Contains('#include <driver/i2c.h>')) {
        throw 'Expected legacy driver/i2c.h include not found'
    }
    if (-not $TouchC.Contains('i2c_driver_install')) {
        throw 'Expected legacy i2c_driver_install() baseline not found'
    }
    if (-not $LcdC.Contains('(esp_lcd_i2c_bus_handle_t)1')) {
        throw 'Expected legacy integer bus cast not found in lcd_display.c'
    }
    if (-not $LcdC.Contains('#define LVGL_TASK_SLEEP 500')) {
        throw 'Expected Test 36 baseline sleep=500 not found'
    }
    if (-not $LcdH.Contains('#define TOUCH_GPIO_INT GPIO_NUM_NC')) {
        throw 'Expected GT911 interrupt-disabled baseline not found'
    }
    if (-not $TouchC.Contains('#define I2C_MASTER_FREQ_HZ  400000')) {
        throw 'Expected Test 36 I2C 400 kHz baseline not found'
    }
    if (-not $Cmake.Contains('REQUIRES esp_lcd lvgl lvgl__lvgl')) {
        throw 'Expected upstream CMake dependency line not found'
    }

    $Baseline = @"
Test 36C controlled derivative
Upstream: halyssonJr/lvgl-demo-esp32s3
Commit: $Commit

ONLY intended experimental variable:
  GT911 I2C transport backend
  legacy driver/i2c.h + i2c_driver_install + integer bus id
  -> modern driver/i2c_master.h + i2c_new_master_bus + typed bus handle

Preserved:
- LVGL_TASK_SLEEP 500 ms
- GT911 INT disabled
- I2C logical port 1
- SDA19 / SCL20
- I2C device speed explicitly 400 kHz
- coordinate mapping unchanged
- LVGL/esp_lvgl_port unchanged
- display timings/framebuffers/bounce/direct/avoid-tearing unchanged
- XML/generated UI unchanged
"@

    [System.IO.File]::WriteAllText((Join-Path $WorkRoot 'BASELINE.txt'), $Baseline, (New-Object System.Text.UTF8Encoding($false)))

    Write-Host '[PASS] Test 36C exact upstream baseline prepared'
    Write-Host '[PASS] Legacy I2C baseline verified'
    Write-Host '[PASS] sleep=500 and GT911 INT disabled verified'
    Write-Host '[PASS] Test 36 I2C frequency 400 kHz verified'
}
finally {
    Pop-Location
}
