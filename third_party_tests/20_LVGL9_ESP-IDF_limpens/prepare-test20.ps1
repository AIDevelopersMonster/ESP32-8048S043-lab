$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$workRoot = Join-Path $repoRoot '.external-test-work\20_LVGL9_ESP-IDF_limpens'
$upstreamDir = Join-Path $workRoot 'upstream'
$upstreamUrl = 'https://github.com/limpens/esp32-8048S043-lvgl9.git'
$pinnedCommit = 'eb1b8cff63e5a703631ab1638ff76eb7ba7e7a51'

Write-Host ''
Write-Host '=== ESP32-8048S043 Lab / Test 20 ==='
Write-Host 'External project : limpens/esp32-8048S043-lvgl9'
Write-Host "Pinned commit    : $pinnedCommit"
Write-Host "Work directory   : $upstreamDir"
Write-Host ''

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw 'git is not available in PATH.'
}

New-Item -ItemType Directory -Force -Path $workRoot | Out-Null

if (Test-Path $upstreamDir) {
    Write-Host '[INFO] Existing working copy found.'
    Push-Location $upstreamDir
    try {
        git remote get-url origin
        git fetch --all --tags --prune
    }
    finally {
        Pop-Location
    }
}
else {
    Write-Host '[INFO] Cloning upstream...'
    git clone $upstreamUrl $upstreamDir
}

Push-Location $upstreamDir
try {
    Write-Host '[INFO] Checking out pinned revision...'
    git checkout --detach $pinnedCommit

    $actualCommit = (git rev-parse HEAD).Trim()
    if ($actualCommit -ne $pinnedCommit) {
        throw "Pinned revision mismatch: $actualCommit"
    }

    Write-Host ''
    Write-Host '=== UPSTREAM SNAPSHOT ==='
    Write-Host "Commit : $actualCommit"
    git log -1 --format='Date   : %cI%nAuthor : %an <%ae>%nTitle  : %s'

    Write-Host ''
    Write-Host '=== DECLARED COMPONENTS ==='
    Get-Content '.\main\idf_component.yml'

    Write-Host ''
    Write-Host '=== KEY DISPLAY SETTINGS ==='
    Select-String '.\main\hardware.h' -Pattern 'LCD_H_RES|LCD_V_RES|LCD_PIXEL_CLOCK_HZ|TOUCH_H_RES|TOUCH_V_RES' |
        ForEach-Object { $_.Line.Trim() }

    Select-String '.\main\lvgl9.c' -Pattern 'hsync_pulse_width|hsync_back_porch|hsync_front_porch|vsync_pulse_width|vsync_back_porch|vsync_front_porch|pclk_active_neg|num_fbs|bounce_buffer_size_px|fb_in_psram|double_fb' |
        ForEach-Object { $_.Line.Trim() }

    Write-Host ''
    if (Get-Command idf.py -ErrorAction SilentlyContinue) {
        Write-Host '=== ESP-IDF ENVIRONMENT ==='
        idf.py --version
        Write-Host ''
        Write-Host '[READY] ESP-IDF environment detected.'
        Write-Host 'Next commands:'
        Write-Host "  cd `"$upstreamDir`""
        Write-Host '  idf.py set-target esp32s3'
        Write-Host '  idf.py build'
        Write-Host '  idf.py flash monitor'
    }
    else {
        Write-Host '[NOTE] idf.py is not currently available in PATH.'
        Write-Host 'Activate an ESP-IDF environment, then run:'
        Write-Host "  cd `"$upstreamDir`""
        Write-Host '  idf.py --version'
        Write-Host '  idf.py set-target esp32s3'
        Write-Host '  idf.py build'
    }
}
finally {
    Pop-Location
}
