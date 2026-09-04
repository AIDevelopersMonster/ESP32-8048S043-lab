param(
    [switch]$Upload,
    [string]$UploadPort = "",
    [string]$TargetWifiSsid = "TEST-NETWORK",
    [string]$WorkRoot = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$prep = Join-Path $scriptDir "prepare-test35.ps1"

if ([string]::IsNullOrWhiteSpace($WorkRoot)) {
    $WorkRoot = Join-Path $env:USERPROFILE "t35-robotcore"
}

$upstreamDir = Join-Path $WorkRoot "upstream"
$upstreamCommit = "1d95120dc43e7663dfb5888a4da079aae1929153"
$platformCommit = "816219db19399d376cfeac3bab6edd14b781701c"
$pioFile = Join-Path $upstreamDir "platformio.ini"
$secretsFile = Join-Path $upstreamDir "display\secrets.h"

if (-not (Test-Path $upstreamDir)) {
    & powershell -ExecutionPolicy Bypass -File $prep -WorkRoot $WorkRoot
    if ($LASTEXITCODE -ne 0) { throw "prepare-test35.ps1 failed" }
}

# Always restore exact upstream before applying temporary reproducibility pins.
& git -C $upstreamDir reset --hard $upstreamCommit
if ($LASTEXITCODE -ne 0) { throw "Could not reset upstream" }
& git -C $upstreamDir clean -fdx
if ($LASTEXITCODE -ne 0) { throw "Could not clean upstream" }

$head = (& git -C $upstreamDir rev-parse HEAD | Out-String).Trim()
if ($head -ne $upstreamCommit) { throw "Unexpected upstream HEAD: $head" }

if (-not (Test-Path $pioFile)) { throw "platformio.ini missing" }

$originalPio = [System.IO.File]::ReadAllText($pioFile)
$patchedPio = $originalPio

# Pin moving platform URL to the latest known commit before the app commit.
$patchedPio = $patchedPio.Replace(
    "platform = https://github.com/Jason2866/platform-espressif32.git",
    "platform = https://github.com/Jason2866/platform-espressif32.git#$platformCommit"
)

# Pin caret dependencies to the versions documented by upstream.
$patchedPio = $patchedPio.Replace("lvgl/lvgl @ ^9.1.0", "lvgl/lvgl @ 9.1.0")
$patchedPio = $patchedPio.Replace("tamctec/TAMC_GT911 @ ^1.0.2", "tamctec/TAMC_GT911 @ 1.0.2")

if ($patchedPio -eq $originalPio) {
    throw "Reproducibility pinning made no changes; upstream manifest may have changed"
}

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($pioFile, $patchedPio, $utf8NoBom)

$escapedSsid = $TargetWifiSsid.Replace("\", "\\").Replace('"', '\"')
$secretsText = @"
#pragma once
#define TARGET_WIFI_SSID "$escapedSsid"
"@
[System.IO.File]::WriteAllText($secretsFile, $secretsText, $utf8NoBom)

# Use the existing PlatformIO command if available. Prefer pio, then python -m platformio.
$pioCmd = Get-Command pio -ErrorAction SilentlyContinue
$pythonCmd = Get-Command python -ErrorAction SilentlyContinue

function Invoke-Pio {
    param([string[]]$Args)

    if ($pioCmd) {
        & $pioCmd.Source @Args
        return $LASTEXITCODE
    }

    if ($pythonCmd) {
        & $pythonCmd.Source -m platformio @Args
        return $LASTEXITCODE
    }

    throw "PlatformIO not found. Install PlatformIO Core or ensure 'pio' is in PATH."
}

Push-Location $upstreamDir
try {
    Write-Host ""
    Write-Host "=== Test 35 historical build ==="
    Write-Host "Upstream app     : $upstreamCommit"
    Write-Host "PIO platform pin : $platformCommit"
    Write-Host "LVGL             : 9.1.0 exact"
    Write-Host "TAMC_GT911       : 1.0.2 exact"
    Write-Host "GUI              : cards / modal / sliders / footer / 3 application pages"
    Write-Host "Display          : Arduino_GFX / RGB / bounce20 / PCLK16"
    Write-Host "Touch            : exact upstream custom calibration retained"
    Write-Host ""

    $buildArgs = @("run", "-e", "ESP32S3-8048S043")
    $rc = Invoke-Pio -Args $buildArgs
    if ($rc -ne 0) { throw "Test 35 build failed" }

    if ($Upload) {
        Write-Host ""
        Write-Host "=== Test 35 upload ==="
        $uploadArgs = @("run", "-e", "ESP32S3-8048S043", "-t", "upload")
        if ($UploadPort) {
            $uploadArgs += @("--upload-port", $UploadPort)
        }
        $rc = Invoke-Pio -Args $uploadArgs
        if ($rc -ne 0) { throw "Test 35 upload failed" }
    }
    else {
        Write-Host ""
        Write-Host "Build complete. To upload:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\35_LVGL9_RobotCoreDisplay\run-test35.ps1 -Upload"
        Write-Host ""
        Write-Host "Explicit port example:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\35_LVGL9_RobotCoreDisplay\run-test35.ps1 -Upload -UploadPort COM7"
    }
}
finally {
    Pop-Location

    # Restore exact public upstream source and remove all generated files.
    & git -C $upstreamDir reset --hard $upstreamCommit | Out-Null
    & git -C $upstreamDir clean -fdx | Out-Null

    $status = (& git -C $upstreamDir status --porcelain --untracked-files=all | Out-String).Trim()
    if ($status) {
        Write-Host "[FAIL] Upstream source tree changed after Test 35:"
        Write-Host $status
        throw "Exact upstream source was not restored"
    }

    Write-Host "[PASS] Exact upstream Robot-Core-Display source restored"
    Write-Host "[PASS] Temporary platform/dependency pins removed"
    Write-Host "[PASS] Temporary secrets.h removed"
}
