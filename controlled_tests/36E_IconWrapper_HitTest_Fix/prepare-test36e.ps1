param(
    [string]$WorkRoot = "$HOME\t36e-icon-wrapper-hit-test"
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

    $Deck = [System.IO.File]::ReadAllText((Join-Path $Upstream 'main\examples\components\deck_btn_gen.c'))
    $LcdC = [System.IO.File]::ReadAllText((Join-Path $Upstream 'main\lcd_display.c'))
    $LcdH = [System.IO.File]::ReadAllText((Join-Path $Upstream 'main\lcd_display.h'))

    if (-not $Deck.Contains('lv_obj_t * lv_obj_0 = lv_obj_create(lv_button_0);')) {
        throw 'Expected icon-wrapper creation baseline not found'
    }
    if ($Deck.Contains('lv_obj_remove_flag(lv_obj_0, LV_OBJ_FLAG_CLICKABLE);')) {
        throw 'Icon-wrapper clickable fix already present in upstream baseline'
    }
    if (-not $LcdC.Contains('#define LVGL_TASK_SLEEP 500')) {
        throw 'Expected Test 36 baseline sleep=500 not found'
    }
    if (-not $LcdH.Contains('#define TOUCH_GPIO_INT GPIO_NUM_NC')) {
        throw 'Expected GT911 interrupt-disabled baseline not found'
    }

    Write-Host '[PASS] Test 36E exact upstream baseline prepared'
    Write-Host '[PASS] Clickable 64x64 icon-wrapper baseline verified'
    Write-Host '[PASS] sleep=500 and GT911 INT disabled verified'
}
finally {
    Pop-Location
}
