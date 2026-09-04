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

function Show-LatestIdfFlashLogs {
    param([string]$BuildDir)

    $LogDir = Join-Path $BuildDir 'log'
    if (-not (Test-Path $LogDir)) {
        Write-Warning "ESP-IDF log directory not found: $LogDir"
        return
    }

    $Files = @(
        Get-ChildItem -Path $LogDir -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -like 'idf_py_stderr_output_*' -or $_.Name -like 'idf_py_stdout_output_*' } |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 4
    )

    if (-not $Files) {
        Write-Warning "No idf_py stdout/stderr logs found in $LogDir"
        return
    }

    Write-Host ''
    Write-Host '========== ESP-IDF / esptool diagnostic tail =========='
    foreach ($File in $Files) {
        Write-Host "--- $($File.FullName) ---"
        Get-Content -Path $File.FullName -Tail 120 -ErrorAction SilentlyContinue
    }
    Write-Host '========================================================'
    Write-Host ''
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

        $Ports = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
        Write-Host "Detected serial ports: $($Ports -join ', ')"
        if ($Ports -notcontains $UploadPort) {
            throw "Requested upload port $UploadPort is not currently present. Available: $($Ports -join ', ')"
        }

        Write-Host "Flashing Test 36B to $UploadPort ..."
        idf.py -p $UploadPort flash
        if ($LASTEXITCODE -ne 0) {
            Show-LatestIdfFlashLogs -BuildDir (Join-Path $Upstream 'build')
            throw "idf.py flash failed on $UploadPort. See diagnostic tail above. If it says access denied / port busy, close Serial Monitor, Arduino IDE monitor, PuTTY, or any other program holding the port and rerun."
        }
        Write-Host "[PASS] Test 36B flashed to $UploadPort"
    }
}
finally {
    # ESP-IDF 5.5.5 may rewrite tracked sdkconfig/dependencies.lock when reproducing
    # the upstream 5.5.0 project. Restore the ENTIRE tracked upstream tree, not only
    # the one controlled source delta. Keep ignored build/ and managed_components/.
    git checkout -- . 2>$null
    $Status = (git status --porcelain --untracked-files=no | Out-String).Trim()
    if ($Status) {
        Write-Warning "Tracked upstream tree is not clean after full restore: $Status"
    } else {
        Write-Host '[PASS] Upstream tracked source fully restored after Test 36B'
    }
    Pop-Location
}
