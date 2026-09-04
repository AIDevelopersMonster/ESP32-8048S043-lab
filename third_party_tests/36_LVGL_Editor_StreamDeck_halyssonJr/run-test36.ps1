param(
    [switch]$Upload,
    [string]$UploadPort = "",
    [string]$WorkRoot = "$HOME\t36-lvgl-editor",
    [string]$IdfExport = ""
)

$ErrorActionPreference = 'Stop'

$Commit = '79e862ca332525ba8721c4691f450fb44ec08738'
$Upstream = Join-Path $WorkRoot 'upstream'
$Prepare = Join-Path $PSScriptRoot 'prepare-test36.ps1'

& powershell -ExecutionPolicy Bypass -File $Prepare -WorkRoot $WorkRoot
if ($LASTEXITCODE -ne 0) { throw 'prepare-test36.ps1 failed' }

function Import-IdfEnvironment {
    param([string]$ExplicitExport)

    if ($ExplicitExport) {
        if (-not (Test-Path $ExplicitExport)) {
            throw "ESP-IDF export script not found: $ExplicitExport"
        }
        . $ExplicitExport
        return
    }

    if (Get-Command idf.py -ErrorAction SilentlyContinue) {
        return
    }

    $Candidates = @(
        "$HOME\esp\v5.5.5\esp-idf\export.ps1",
        "$HOME\esp\v5.5\esp-idf\export.ps1",
        "$HOME\esp\esp-idf\export.ps1",
        "C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1",
        "C:\Espressif\frameworks\esp-idf-v5.5\export.ps1"
    )

    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            Write-Host "Using ESP-IDF environment: $Candidate"
            . $Candidate
            return
        }
    }

    throw "idf.py is not available. Open an ESP-IDF 5.5.x PowerShell or pass -IdfExport <path-to-export.ps1>."
}

Import-IdfEnvironment -ExplicitExport $IdfExport

$idfVersionText = (& idf.py --version | Out-String).Trim()
Write-Host "ESP-IDF: $idfVersionText"
if ($idfVersionText -notmatch 'v?5\.5\.') {
    Write-Warning "Upstream lock is ESP-IDF 5.5.0. This runner is intended for the ESP-IDF 5.5.x line."
}

Push-Location $Upstream
try {
    $Head = (git rev-parse HEAD).Trim()
    if ($Head -ne $Commit) {
        throw "Pinned commit mismatch before build: $Head"
    }

    $TrackedStatus = (git status --porcelain --untracked-files=no | Out-String).Trim()
    if ($TrackedStatus) {
        throw "Tracked upstream tree is dirty before build: $TrackedStatus"
    }

    $SdkConfig = [System.IO.File]::ReadAllText((Join-Path $Upstream 'sdkconfig'))
    if (-not $SdkConfig.Contains('CONFIG_IDF_TARGET="esp32s3"')) {
        throw 'Pinned sdkconfig no longer targets esp32s3'
    }
    if (-not $SdkConfig.Contains('CONFIG_LV_USE_OBJ_NAME=y')) {
        throw 'Pinned sdkconfig lost CONFIG_LV_USE_OBJ_NAME=y'
    }
    if (-not $SdkConfig.Contains('CONFIG_PARTITION_TABLE_CUSTOM=y')) {
        throw 'Pinned sdkconfig lost custom partition table selection'
    }

    # The first Test 36 runner incorrectly called `idf.py set-target esp32s3`.
    # That command replaced the tracked sdkconfig, disabling LV_USE_OBJ_NAME and
    # switching the build to the default 1 MB partition table. If that failed
    # build cache is present, discard only build/ and keep managed_components/.
    $BuildConfig = Join-Path $Upstream 'build\config\sdkconfig.h'
    if (Test-Path $BuildConfig) {
        $BuildConfigText = [System.IO.File]::ReadAllText($BuildConfig)
        $BadObjName = -not $BuildConfigText.Contains('#define CONFIG_LV_USE_OBJ_NAME 1')
        $BadCustomPartition = -not $BuildConfigText.Contains('#define CONFIG_PARTITION_TABLE_CUSTOM 1')
        if ($BadObjName -or $BadCustomPartition) {
            Write-Host '[INFO] Removing incompatible build cache created by the previous Test 36 runner'
            Remove-Item -Recurse -Force (Join-Path $Upstream 'build')
        }
    }

    Write-Host '[PASS] Using pinned upstream sdkconfig; idf.py set-target is intentionally NOT called'

    idf.py build
    if ($LASTEXITCODE -ne 0) { throw 'idf.py build failed' }

    # Verify that the actual build consumed the required upstream configuration.
    $BuiltSdkConfig = Join-Path $Upstream 'build\config\sdkconfig.h'
    if (-not (Test-Path $BuiltSdkConfig)) {
        throw 'Build completed but build/config/sdkconfig.h is missing'
    }
    $BuiltConfigText = [System.IO.File]::ReadAllText($BuiltSdkConfig)
    if (-not $BuiltConfigText.Contains('#define CONFIG_LV_USE_OBJ_NAME 1')) {
        throw 'Build did not retain CONFIG_LV_USE_OBJ_NAME=1'
    }
    if (-not $BuiltConfigText.Contains('#define CONFIG_PARTITION_TABLE_CUSTOM 1')) {
        throw 'Build did not retain the custom upstream partition table'
    }

    Write-Host '[PASS] Test 36 build complete'
    Write-Host '[PASS] LVGL object-name support retained in actual build'
    Write-Host '[PASS] Upstream custom partition configuration retained in actual build'

    if ($Upload) {
        if (-not $UploadPort) {
            throw '-Upload requires -UploadPort, for example COM7'
        }
        idf.py -p $UploadPort flash
        if ($LASTEXITCODE -ne 0) { throw 'idf.py flash failed' }
        Write-Host "[PASS] Test 36 flashed to $UploadPort"
        Write-Host "Monitor command: idf.py -p $UploadPort monitor"
    }
}
finally {
    # Restore tracked source/config exactly to the pinned commit. Keep ignored
    # build and managed component directories for faster repeat builds.
    git checkout -- . 2>$null
    $After = (git rev-parse HEAD).Trim()
    $AfterStatus = (git status --porcelain --untracked-files=no | Out-String).Trim()
    if (($After -eq $Commit) -and (-not $AfterStatus)) {
        Write-Host '[PASS] Exact pinned upstream tracked revision retained'
    }
    Pop-Location
}
