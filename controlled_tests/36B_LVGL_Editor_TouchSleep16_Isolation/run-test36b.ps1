param(
    [switch]$Upload,
    [string]$UploadPort = "",
    [string]$WorkRoot = "$HOME\t36b-touch-sleep16",
    [string]$IdfExport = ""
)

$ErrorActionPreference = 'Stop'

$Commit = '79e862ca332525ba8721c4691f450fb44ec08738'
$Upstream = Join-Path $WorkRoot 'upstream'
$Prepare = Join-Path $PSScriptRoot 'prepare-test36b.ps1'

& powershell -ExecutionPolicy Bypass -File $Prepare -WorkRoot $WorkRoot

function Import-IdfEnvironment {
    param([string]$ExplicitExport)
    if ($ExplicitExport) {
        . $ExplicitExport
        return
    }
    if (Get-Command idf.py -ErrorAction SilentlyContinue) { return }
    $Candidates = @(
        "$HOME\esp\v5.5.5\esp-idf\export.ps1",
        "C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1"
    )
    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            . $Candidate
            return
        }
    }
    throw 'idf.py not available; pass -IdfExport'
}

Import-IdfEnvironment -ExplicitExport $IdfExport

Push-Location $Upstream
try {
    $Head = (git rev-parse HEAD).Trim()
    if ($Head -ne $Commit) { throw "Pinned commit mismatch: $Head" }

    $LcdPath = Join-Path $Upstream 'main\lcd_display.c'
    $Original = [System.IO.File]::ReadAllText($LcdPath)
    if (-not $Original.Contains('#define LVGL_TASK_SLEEP 500')) {
        throw 'Baseline sleep=500 not found before patch'
    }

    $Patched = $Original.Replace('#define LVGL_TASK_SLEEP 500', '#define LVGL_TASK_SLEEP 16')
    if ($Patched -eq $Original) { throw 'Patch produced no change' }
    [System.IO.File]::WriteAllText($LcdPath, $Patched, (New-Object System.Text.UTF8Encoding($false)))

    $Diff = (git diff -- main/lcd_display.c | Out-String)
    if ($Diff -notmatch 'LVGL_TASK_SLEEP 500' -or $Diff -notmatch 'LVGL_TASK_SLEEP 16') {
        throw 'Unexpected source diff'
    }

    Write-Host '[PASS] Applied ONLY Test 36B delta: LVGL_TASK_SLEEP 500 -> 16 ms'

    idf.py build
    if ($LASTEXITCODE -ne 0) { throw 'idf.py build failed' }
    Write-Host '[PASS] Test 36B build complete'

    if ($Upload) {
        if (-not $UploadPort) { throw '-Upload requires -UploadPort' }
        idf.py -p $UploadPort flash
        if ($LASTEXITCODE -ne 0) { throw 'idf.py flash failed' }
        Write-Host "[PASS] Test 36B flashed to $UploadPort"
    }
}
finally {
    git checkout -- main/lcd_display.c 2>$null
    $Status = (git status --porcelain --untracked-files=no | Out-String).Trim()
    if ($Status) {
        Write-Warning "Tracked upstream tree is not clean after restore: $Status"
    } else {
        Write-Host '[PASS] Upstream tracked source restored after Test 36B'
    }
    Pop-Location
}
