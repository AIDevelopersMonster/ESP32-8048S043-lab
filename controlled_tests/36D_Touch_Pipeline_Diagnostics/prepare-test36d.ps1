param(
    [string]$WorkRoot = "$HOME\t36d-touch-pipeline"
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

    $Lcd = [System.IO.File]::ReadAllText((Join-Path $Upstream 'main\lcd_display.c'))
    if (-not $Lcd.Contains('#define LVGL_TASK_SLEEP 500')) { throw 'sleep=500 baseline missing' }
    if (-not $Lcd.Contains('(esp_lcd_i2c_bus_handle_t)1')) { throw 'legacy I2C baseline call missing' }

    Write-Host '[PASS] Test 36D pinned upstream baseline prepared'
    Write-Host '[PASS] Baseline sleep=500 verified'
}
finally {
    Pop-Location
}
