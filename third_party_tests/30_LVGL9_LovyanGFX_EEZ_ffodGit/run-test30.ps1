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

# Historical environment reconstruction for upstream commit 2024-09-22.
# PlatformIO Espressif32 6.8.1 was the latest official release before 6.9.0
# (6.9.0 was released 2024-09-26). 6.8.1 uses Arduino-ESP32 2.0.17 / IDF 4.4.7.
$platformCommit = "3f33ccea90eb316581cdb7524d6a78c1335b9731"
$platformSpec = "https://github.com/platformio/platform-espressif32.git#$platformCommit"
$lovyanVersion = "1.1.16"
$envStamp = "platformio-espressif32=$platformCommit`nlovyangfx=$lovyanVersion`n"
$envStampPath = Join-Path $workRoot "HISTORICAL_ENV.txt"
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

function Write-Utf8NoBom {
    param(
        [string]$Path,
        [string]$Text
    )
    [System.IO.File]::WriteAllText($Path, $Text, $utf8NoBom)
}

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
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python314\Scripts\platformio.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python313\Scripts\platformio.exe"),
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
    throw "PlatformIO CLI not found."
}

Write-Host "[PASS] PlatformIO CLI: $pioExe"

$platformioIni = Join-Path $upstreamDir "platformio.ini"

# The previous script revision could leave platformio.ini with a UTF-8 BOM on
# Windows PowerShell 5.1. This is a disposable pinned upstream checkout, so
# restore the tracked file from HEAD before constructing the temporary overlay.
Push-Location $upstreamDir
try {
    $head = (git rev-parse HEAD).Trim()
    if ($head -ne "18b6d4de509abb61feb0084c1583d41497836cfd") {
        throw "Unexpected upstream HEAD: $head"
    }

    git checkout -- platformio.ini
    if ($LASTEXITCODE -ne 0) { throw "Could not restore pristine upstream platformio.ini" }
}
finally {
    Pop-Location
}

$originalIni = [System.IO.File]::ReadAllText($platformioIni)

if (-not $originalIni.Contains("platform = espressif32")) {
    throw "Expected unpinned upstream platform declaration not found. Refusing to alter unknown project state."
}
if (-not $originalIni.Contains("lovyan03/LovyanGFX@^1.1.16")) {
    throw "Expected upstream LovyanGFX dependency declaration not found. Refusing to alter unknown project state."
}

$pinnedIni = $originalIni.Replace(
    "platform = espressif32",
    "platform = $platformSpec"
).Replace(
    "lovyan03/LovyanGFX@^1.1.16",
    "lovyan03/LovyanGFX@$lovyanVersion"
)

$needClean = $true
if (Test-Path $envStampPath) {
    $oldStamp = Get-Content $envStampPath -Raw
    if ($oldStamp -eq $envStamp) {
        $needClean = $false
    }
}

if ($needClean) {
    $pioBuildDir = Join-Path $upstreamDir ".pio"
    if (Test-Path $pioBuildDir) {
        Write-Host "Removing previous .pio state produced by a different environment..."
        Remove-Item $pioBuildDir -Recurse -Force
    }
    [System.IO.File]::WriteAllText($envStampPath, $envStamp, [System.Text.Encoding]::ASCII)
}

Push-Location $upstreamDir
try {
    # Temporary build-environment overlay only. Application/display/touch/UI source remains exact upstream.
    # IMPORTANT: PlatformIO 6.1.x rejects an INI whose first byte is a UTF-8 BOM,
    # so use .NET UTF8Encoding(false), not PowerShell 5.1 Set-Content -Encoding UTF8.
    Write-Utf8NoBom -Path $platformioIni -Text $pinnedIni

    $firstBytes = [System.IO.File]::ReadAllBytes($platformioIni)
    if ($firstBytes.Length -ge 3 -and $firstBytes[0] -eq 0xEF -and $firstBytes[1] -eq 0xBB -and $firstBytes[2] -eq 0xBF) {
        throw "Internal error: temporary platformio.ini still contains UTF-8 BOM"
    }
    Write-Host "[PASS] Temporary platformio.ini written as UTF-8 without BOM"

    Write-Host ""
    Write-Host "=== Test 30 historical upstream build ==="
    Write-Host "Project          : $upstreamDir"
    Write-Host "Application code : exact ffodGit commit 18b6d4de509abb61feb0084c1583d41497836cfd"
    Write-Host "Platform         : official PlatformIO Espressif32 6.8.1"
    Write-Host "Platform commit  : $platformCommit"
    Write-Host "Arduino-ESP32    : 2.0.17"
    Write-Host "ESP-IDF          : 4.4.7"
    Write-Host "LovyanGFX        : $lovyanVersion"
    Write-Host "LVGL             : upstream vendored 9.1.1-dev"
    Write-Host ""

    & $pioExe --version
    if ($LASTEXITCODE -ne 0) { throw "PlatformIO CLI failed to start" }

    & $pioExe run -e esp32-s3-devkitm-1
    if ($LASTEXITCODE -ne 0) { throw "PlatformIO historical build failed" }

    Write-Host ""
    Write-Host "=== Project package inventory ==="
    & $pioExe pkg list
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Could not print package inventory; build itself succeeded."
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
    # Restore the exact upstream file byte-for-byte from git instead of rewriting
    # it through PowerShell, which could change encoding/BOM/newline details.
    Pop-Location

    Push-Location $upstreamDir
    try {
        git checkout -- platformio.ini
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Could not restore platformio.ini from pinned upstream HEAD"
        }

        $trackedChanges = @(git status --porcelain --untracked-files=no)
        if ($trackedChanges.Count -eq 0) {
            Write-Host "[PASS] Exact tracked upstream source restored after build"
        }
        else {
            Write-Warning "Tracked upstream files differ after build. Inspect before using result:"
            $trackedChanges | ForEach-Object { Write-Warning $_ }
        }
    }
    finally {
        Pop-Location
    }
}
