param(
    [switch]$Upload,
    [string]$UploadPort = "",
    [string]$PlatformIOPath = ""
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

function Resolve-PlatformIO {
    param([string]$ExplicitPath)

    if ($ExplicitPath) {
        if (Test-Path $ExplicitPath) {
            return (Resolve-Path $ExplicitPath).Path
        }
        throw "Explicit PlatformIO path does not exist: $ExplicitPath"
    }

    foreach ($name in @("pio", "platformio")) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) {
            return $cmd.Source
        }
    }

    $candidates = @(
        (Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"),
        (Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python312\Scripts\platformio.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python311\Scripts\platformio.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python310\Scripts\platformio.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

$pioExe = Resolve-PlatformIO -ExplicitPath $PlatformIOPath
if (-not $pioExe) {
    Write-Host "PlatformIO CLI was not found in PATH or standard Windows locations."
    Write-Host "Checked in particular:"
    Write-Host "  $env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
    Write-Host "  $env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
    Write-Host ""
    Write-Host "If VS Code PlatformIO is installed, open a PlatformIO terminal once, or locate the executable with:"
    Write-Host '  Get-ChildItem "$env:USERPROFILE\.platformio" -Filter platformio.exe -Recurse -ErrorAction SilentlyContinue'
    Write-Host ""
    Write-Host "Then pass it explicitly, for example:"
    Write-Host '  .\third_party_tests\30_LVGL9_LovyanGFX_EEZ_ffodGit\run-test30.ps1 -PlatformIOPath "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"'
    throw "PlatformIO CLI not found."
}

Write-Host "[PASS] PlatformIO CLI: $pioExe"

Push-Location $upstreamDir
try {
    Write-Host ""
    Write-Host "=== Test 30 upstream build ==="
    Write-Host "Project : $upstreamDir"
    Write-Host "Source  : exact pinned ffodGit upstream"
    Write-Host ""

    & $pioExe --version
    if ($LASTEXITCODE -ne 0) { throw "PlatformIO CLI failed to start" }

    & $pioExe run -e esp32-s3-devkitm-1
    if ($LASTEXITCODE -ne 0) { throw "PlatformIO build failed" }

    Write-Host ""
    Write-Host "=== PlatformIO resolved environment ==="
    & $pioExe platform show espressif32
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
        & $pioExe @args
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
