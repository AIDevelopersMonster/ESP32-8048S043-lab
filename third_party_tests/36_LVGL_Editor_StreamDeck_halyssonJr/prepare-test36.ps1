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
}

Push-Location $Upstream
try {
    git fetch --all --tags --prune
    git reset --hard
    git clean -ffd
    git checkout --detach $Commit

    $Head = (git rev-parse HEAD).Trim()
    if ($Head -ne $Commit) {
        throw "Pinned commit mismatch: $Head"
    }

    $Status = git status --porcelain
    if ($Status) {
        throw "Upstream tree is not clean after checkout"
    }

    $Required = @(
        'main\lcd_display.c',
        'main\lcd_display.h',
        'main\touch_i2c.c',
        'main\examples\project.xml',
        'main\examples\components\deck_btn.xml',
        'main\examples\screens\stream_deck_main.xml',
        'dependencies.lock',
        'partitions.csv'
    )

    foreach ($Rel in $Required) {
        if (-not (Test-Path (Join-Path $Upstream $Rel))) {
            throw "Required upstream file missing: $Rel"
        }
    }

    $Baseline = @"
Test 36 upstream baseline
Repository: halyssonJr/lvgl-demo-esp32s3
Pinned commit: $Commit
Work tree: $Upstream
Prepared: $(Get-Date -Format o)
"@
    [System.IO.File]::WriteAllText(
        (Join-Path $WorkRoot 'BASELINE.txt'),
        $Baseline,
        (New-Object System.Text.UTF8Encoding($false))
    )

    Write-Host "[PASS] Test 36 upstream prepared"
    Write-Host "[PASS] HEAD: $Head"
    Write-Host "[PASS] Clean source tree"
    Write-Host "Work tree: $Upstream"
}
finally {
    Pop-Location
}
