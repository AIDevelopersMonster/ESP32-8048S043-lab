param(
    [string]$WorkRoot = "$HOME\t36-lvgl-editor"
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

    # Preserve expensive ignored build/dependency caches between runs.
    # The runner itself decides whether an existing build directory is compatible.
    git clean -ffd -e build/ -e managed_components/
    if ($LASTEXITCODE -ne 0) { throw 'git clean failed' }

    git checkout --detach $Commit
    if ($LASTEXITCODE -ne 0) { throw 'checkout of pinned upstream commit failed' }

    # Restore every tracked file exactly as stored in the pinned commit. This is
    # especially important for sdkconfig: idf.py set-target must never replace it.
    git checkout -- .

    $Head = (git rev-parse HEAD).Trim()
    if ($Head -ne $Commit) {
        throw "Pinned commit mismatch: $Head"
    }

    $Status = (git status --porcelain --untracked-files=no | Out-String).Trim()
    if ($Status) {
        throw "Tracked upstream tree is not clean after checkout: $Status"
    }

    $Required = @(
        'main\lcd_display.c',
        'main\lcd_display.h',
        'main\touch_i2c.c',
        'main\examples\project.xml',
        'main\examples\components\deck_btn.xml',
        'main\examples\screens\stream_deck_main.xml',
        'dependencies.lock',
        'sdkconfig',
        'partitions.csv'
    )

    foreach ($Rel in $Required) {
        if (-not (Test-Path (Join-Path $Upstream $Rel))) {
            throw "Required upstream file missing: $Rel"
        }
    }

    $SdkConfigPath = Join-Path $Upstream 'sdkconfig'
    $SdkConfig = [System.IO.File]::ReadAllText($SdkConfigPath)
    $SdkChecks = @(
        @{ Name = 'ESP32-S3 target'; Token = 'CONFIG_IDF_TARGET="esp32s3"' },
        @{ Name = 'LVGL object names'; Token = 'CONFIG_LV_USE_OBJ_NAME=y' },
        @{ Name = 'custom partition table'; Token = 'CONFIG_PARTITION_TABLE_CUSTOM=y' },
        @{ Name = 'custom partition filename'; Token = 'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"' },
        @{ Name = '16 MB flash'; Token = 'CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y' },
        @{ Name = 'Octal PSRAM'; Token = 'CONFIG_SPIRAM_MODE_OCT=y' },
        @{ Name = '80 MHz PSRAM'; Token = 'CONFIG_SPIRAM_SPEED_80M=y' }
    )

    foreach ($Check in $SdkChecks) {
        if (-not $SdkConfig.Contains($Check.Token)) {
            throw "Pinned sdkconfig verification failed: $($Check.Name) / missing '$($Check.Token)'"
        }
    }

    $Partitions = [System.IO.File]::ReadAllText((Join-Path $Upstream 'partitions.csv'))
    if ($Partitions -notmatch '(?m)^factory\s*,\s*app\s*,\s*factory\s*,\s*0x10000\s*,\s*2M\s*,') {
        throw 'Pinned partitions.csv verification failed: expected 2M factory partition'
    }
    if ($Partitions -notmatch '(?m)^storage\s*,\s*data\s*,\s*spiffs\s*,\s*0x210000\s*,\s*1M\s*,') {
        throw 'Pinned partitions.csv verification failed: expected 1M SPIFFS storage partition'
    }

    $Baseline = @"
Test 36 upstream baseline
Repository: halyssonJr/lvgl-demo-esp32s3
Pinned commit: $Commit
Work tree: $Upstream
Prepared: $(Get-Date -Format o)

Verified tracked configuration:
- target: esp32s3
- CONFIG_LV_USE_OBJ_NAME=y
- custom partitions.csv enabled
- flash: 16 MB
- PSRAM: Octal 80 MHz
- factory partition: 2 MB
- SPIFFS storage: 1 MB at 0x210000

IMPORTANT:
- Do NOT run idf.py set-target for this baseline.
- The pinned sdkconfig is part of the application baseline.
"@
    [System.IO.File]::WriteAllText(
        (Join-Path $WorkRoot 'BASELINE.txt'),
        $Baseline,
        (New-Object System.Text.UTF8Encoding($false))
    )

    Write-Host "[PASS] Test 36 upstream prepared"
    Write-Host "[PASS] HEAD: $Head"
    Write-Host "[PASS] Tracked source tree clean"
    Write-Host "[PASS] Upstream sdkconfig preserved and verified"
    Write-Host "[PASS] LVGL object naming enabled"
    Write-Host "[PASS] Original 2M factory + 1M SPIFFS partition map verified"
    Write-Host "Work tree: $Upstream"
}
finally {
    Pop-Location
}
