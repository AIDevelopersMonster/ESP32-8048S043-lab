param(
    [switch]$Upload,
    [string]$UploadPort = "",
    [string]$WifiSsid = "TEST-NETWORK",
    [string]$WifiPassword = "test-password-1234",
    [string]$WorkRoot = "",
    [string]$PythonPath = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$prep = Join-Path $scriptDir "prepare-test34.ps1"

if ([string]::IsNullOrWhiteSpace($WorkRoot)) {
    $WorkRoot = Join-Path $env:USERPROFILE "t34-xoquox"
}

$upstreamDir = Join-Path $WorkRoot "upstream"
$venvDir = Join-Path $WorkRoot "venv"
$wrapper = Join-Path $WorkRoot "xoquox-43-test.yaml"
$secrets = Join-Path $WorkRoot "secrets.yaml"
$upstreamCommit = "33a2a35c0c09c9b3c825e98a0a4abe41931b5708"
$esphomeVersion = "2025.12.7"

if (-not (Test-Path $upstreamDir) -or -not (Test-Path $wrapper)) {
    & powershell -ExecutionPolicy Bypass -File $prep -WorkRoot $WorkRoot
    if ($LASTEXITCODE -ne 0) { throw "prepare-test34.ps1 failed" }
}

function Get-PythonInfo {
    param([string]$Executable)

    if (-not $Executable -or -not (Test-Path $Executable)) { return $null }

    $probe = (& $Executable -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}|{sys.executable}')" 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $probe) { return $null }

    $parts = $probe.Split('|', 2)
    if ($parts.Count -ne 2) { return $null }

    $versionParts = $parts[0].Split('.')
    if ($versionParts.Count -lt 2) { return $null }

    $major = [int]$versionParts[0]
    $minor = [int]$versionParts[1]

    return [PSCustomObject]@{
        Path = $parts[1]
        Version = $parts[0]
        Major = $major
        Minor = $minor
        Compatible = ($major -eq 3 -and $minor -ge 11 -and $minor -lt 14)
    }
}

function Resolve-Python {
    param([string]$Explicit)

    if ($Explicit) {
        if (-not (Test-Path $Explicit)) { throw "Specified Python executable not found: $Explicit" }
        $info = Get-PythonInfo -Executable (Resolve-Path $Explicit).Path
        if (-not $info) { throw "Could not execute specified Python: $Explicit" }
        if (-not $info.Compatible) {
            throw "ESPHome $esphomeVersion requires the historical Test 34 Python range 3.11-3.13; specified Python is $($info.Version)"
        }
        return $info
    }

    $candidates = @()
    $pyLauncher = Get-Command py -ErrorAction SilentlyContinue
    if ($pyLauncher) {
        $launcherLines = @(& $pyLauncher.Source -0p 2>$null)
        if ($LASTEXITCODE -eq 0) {
            foreach ($line in $launcherLines) {
                $text = [string]$line
                if ($text -match '(?<path>[A-Za-z]:\\.*?python(?:\.exe)?)\s*$') {
                    $candidates += $Matches['path']
                }
            }
        }
    }

    foreach ($minor in @(13, 12, 11)) {
        $candidates += (Join-Path $env:LOCALAPPDATA "Programs\Python\Python3$minor\python.exe")
        $candidates += (Join-Path $env:ProgramFiles "Python3$minor\python.exe")
        if (${env:ProgramFiles(x86)}) {
            $candidates += (Join-Path ${env:ProgramFiles(x86)} "Python3$minor\python.exe")
        }
    }

    foreach ($candidate in ($candidates | Where-Object { $_ } | Select-Object -Unique)) {
        $info = Get-PythonInfo -Executable $candidate
        if ($info -and $info.Compatible) { return $info }
    }

    Write-Host "[BLOCKED] No compatible Python 3.11-3.13 found for Test 34."
    Write-Host "Recommended: winget install -e --id Python.Python.3.13"
    exit 2
}

$pythonInfo = Resolve-Python -Explicit $PythonPath
$python = $pythonInfo.Path
Write-Host "[PASS] Python: $python"
Write-Host "[PASS] Python version: $($pythonInfo.Version)"

# Restore exact upstream state before the build.
& git -C $upstreamDir reset --hard $upstreamCommit
if ($LASTEXITCODE -ne 0) { throw "Could not reset pinned upstream source" }
& git -C $upstreamDir clean -fdx
if ($LASTEXITCODE -ne 0) { throw "Could not clean pinned upstream source" }

$head = (& git -C $upstreamDir rev-parse HEAD | Out-String).Trim()
if ($head -ne $upstreamCommit) { throw "Unexpected upstream HEAD: $head" }

$venvPython = Join-Path $venvDir "Scripts\python.exe"
$venvEspHome = Join-Path $venvDir "Scripts\esphome.exe"
$venvScripts = Join-Path $venvDir "Scripts"

if (Test-Path $venvPython) {
    $venvInfo = Get-PythonInfo -Executable $venvPython
    if (-not $venvInfo -or -not $venvInfo.Compatible) {
        Write-Host "Removing incompatible previous Test 34 virtual environment..."
        Remove-Item -Recurse -Force $venvDir
    }
}

if (-not (Test-Path $venvDir)) {
    Write-Host "Creating isolated Test 34 Python environment..."
    & $python -m venv $venvDir
    if ($LASTEXITCODE -ne 0) { throw "Python venv creation failed" }
}

$finalVenvInfo = Get-PythonInfo -Executable $venvPython
if (-not $finalVenvInfo -or -not $finalVenvInfo.Compatible) {
    throw "Test 34 virtual environment Python is incompatible"
}
Write-Host "[PASS] Venv Python: $($finalVenvInfo.Version)"

Write-Host "Ensuring ESPHome $esphomeVersion..."
& $venvPython -m pip install --disable-pip-version-check --upgrade "esphome==$esphomeVersion"
if ($LASTEXITCODE -ne 0) { throw "ESPHome installation failed" }
if (-not (Test-Path $venvEspHome)) { throw "ESPHome executable not found" }

$versionText = (& $venvEspHome version | Out-String).Trim()
Write-Host "[PASS] $versionText"
if ($versionText -notmatch [regex]::Escape($esphomeVersion)) {
    throw "Unexpected ESPHome version: $versionText"
}

# Keep PlatformIO isolated and short-path from the first Test 34 run.
$pioCoreDir = Join-Path $env:USERPROFILE ("p34-pio-py{0}{1}" -f $finalVenvInfo.Major, $finalVenvInfo.Minor)
New-Item -ItemType Directory -Force -Path $pioCoreDir | Out-Null

$env:PLATFORMIO_CORE_DIR = $pioCoreDir
$env:VIRTUAL_ENV = $venvDir
$env:UV_PYTHON = $venvPython
$env:PATH = "$venvScripts;$($env:PATH)"

Write-Host "[PASS] Short isolated PlatformIO core: $pioCoreDir"
Write-Host "[PASS] Child Python forced to: $venvPython"
Write-Host "[PASS] Global $env:USERPROFILE\.platformio is not used by Test 34"

$idfEnvDir = Join-Path $pioCoreDir "penv\.espidf-5.5.1"
$idfEnvPython = Join-Path $idfEnvDir "Scripts\python.exe"
if (Test-Path $idfEnvPython) {
    $idfInfo = Get-PythonInfo -Executable $idfEnvPython
    if (-not $idfInfo -or -not $idfInfo.Compatible) {
        Write-Host "Removing incompatible isolated ESP-IDF Python environment..."
        Remove-Item -Recurse -Force $idfEnvDir
    }
}

$escapedSsid = $WifiSsid.Replace("'", "''")
$escapedPassword = $WifiPassword.Replace("'", "''")
$secretsText = @"
wifi_ssid: '$escapedSsid'
wifi_password: '$escapedPassword'
"@
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($secrets, $secretsText, $utf8NoBom)

Push-Location $WorkRoot
try {
    Write-Host ""
    Write-Host "=== Test 34 ESPHome validation ==="
    & $venvEspHome config $wrapper
    if ($LASTEXITCODE -ne 0) { throw "Test 34 ESPHome config validation failed" }

    Write-Host ""
    Write-Host "=== Test 34 historical build ==="
    Write-Host "Upstream commit : $upstreamCommit"
    Write-Host "ESPHome         : $esphomeVersion"
    Write-Host "Python          : $($finalVenvInfo.Version)"
    Write-Host "PIO core        : $pioCoreDir"
    Write-Host "Target wrapper  : xoquox-43-test.yaml"
    Write-Host "Architecture    : ESPHome / ESP-IDF / rpi_dpi_rgb / LVGL YAML / GT911 dual-I2C fork"
    Write-Host "BME680 package  : present upstream, excluded from baseline"
    Write-Host ""

    & $venvEspHome compile $wrapper
    if ($LASTEXITCODE -ne 0) { throw "Test 34 build failed" }

    if ($Upload) {
        Write-Host ""
        Write-Host "=== Test 34 upload ==="
        if ($UploadPort) {
            & $venvEspHome upload $wrapper --device $UploadPort
        }
        else {
            & $venvEspHome upload $wrapper
        }
        if ($LASTEXITCODE -ne 0) { throw "Test 34 upload failed" }
    }
    else {
        Write-Host ""
        Write-Host "Build complete. To upload:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\34_ESPHome_LVGL_xoquox\run-test34.ps1 -Upload"
        Write-Host ""
        Write-Host "Explicit port example:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\34_ESPHome_LVGL_xoquox\run-test34.ps1 -Upload -UploadPort COM7"
    }
}
finally {
    Pop-Location

    if (Test-Path $secrets) { Remove-Item -Force $secrets }

    foreach ($generated in @(
        (Join-Path $WorkRoot ".esphome"),
        (Join-Path $WorkRoot ".pio")
    )) {
        if (Test-Path $generated) { Remove-Item -Recurse -Force $generated }
    }

    & git -C $upstreamDir reset --hard $upstreamCommit | Out-Null
    & git -C $upstreamDir clean -fdx | Out-Null

    $status = (& git -C $upstreamDir status --porcelain --untracked-files=all | Out-String).Trim()
    if ($status) {
        Write-Host "[FAIL] Upstream source tree changed after build:"
        Write-Host $status
        throw "Exact upstream source was not restored"
    }

    Write-Host "[PASS] Exact upstream source restored; generated build artifacts removed"
    Write-Host "[PASS] Isolated PlatformIO cache retained at: $pioCoreDir"
}
