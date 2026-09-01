param(
    [switch]$Upload,
    [string]$UploadPort = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$workRoot = Join-Path $repoRoot ".third-party-work\30_LVGL9_LovyanGFX_EEZ_ffodGit"
$upstreamDir = Join-Path $workRoot "upstream"
$prep = Join-Path $scriptDir "prepare-test30.ps1"

if (-not (Test-Path $upstreamDir)) {
    & powershell -ExecutionPolicy Bypass -File $prep
    if ($LASTEXITCODE -ne 0) { throw "prepare-test30.ps1 failed" }
}

$pioCmd = Get-Command pio -ErrorAction SilentlyContinue
if (-not $pioCmd) {
    $pioCmd = Get-Command platformio -ErrorAction SilentlyContinue
}
if (-not $pioCmd) {
    throw "PlatformIO CLI not found. Install PlatformIO Core or use VS Code + PlatformIO, then run this script again."
}

Push-Location $upstreamDir
try {
    Write-Host ""
    Write-Host "=== Test 30 upstream build ==="
    Write-Host "Project : $upstreamDir"
    Write-Host "Source  : exact pinned ffodGit upstream"
    Write-Host ""

    & $pioCmd.Source run -e esp32-s3-devkitm-1
    if ($LASTEXITCODE -ne 0) { throw "PlatformIO build failed" }

    Write-Host ""
    Write-Host "=== PlatformIO resolved environment ==="
    & $pioCmd.Source platform show espressif32
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Could not print resolved platform details; build itself succeeded."
    }

    if ($Upload) {
        $args = @("run", "-e", "esp32-s3-devkitm-1", "-t", "upload")
        if ($UploadPort) {
            $args += @("--upload-port", $UploadPort)
        }

        Write-Host ""
        Write-Host "=== Test 30 upload ==="
        & $pioCmd.Source @args
        if ($LASTEXITCODE -ne 0) { throw "PlatformIO upload failed" }
    }
    else {
        Write-Host ""
        Write-Host "Build complete. To flash the physical board:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\30_LVGL9_LovyanGFX_EEZ_ffodGit\run-test30.ps1 -Upload"
        Write-Host ""
        Write-Host "If multiple COM ports exist, add for example:"
        Write-Host "  -UploadPort COM7"
    }
}
finally {
    Pop-Location
}
