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
$prep = Join-Path $scriptDir "prepare-test33.ps1"

if ([string]::IsNullOrWhiteSpace($WorkRoot)) {
    $WorkRoot = Join-Path $env:USERPROFILE "t33-ryanewen"
}

$upstreamDir = Join-Path $WorkRoot "upstream"
$venvDir = Join-Path $WorkRoot "venv"
$example = Join-Path $upstreamDir "sunton-43-example.yaml"
$secrets = Join-Path $upstreamDir "secrets.yaml"
$upstreamCommit = "4d3ff33b242c6b6ff67dc76f1cfa9b1041473362"
$esphomeVersion = "2025.12.5"

if (-not (Test-Path $upstreamDir)) {
    & powershell -ExecutionPolicy Bypass -File $prep -WorkRoot $WorkRoot
    if ($LASTEXITCODE -ne 0) { throw "prepare-test33.ps1 failed" }
}

function Get-CompatiblePythonInfo {
    param([string]$Executable)

    if (-not $Executable -or -not (Test-Path $Executable)) {
        return $null
    }

    $probe = (& $Executable -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}|{sys.executable}')" 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $probe) {
        return $null
    }

    $parts = $probe.Split('|', 2)
    if ($parts.Count -ne 2) {
        return $null
    }

    $versionParts = $parts[0].Split('.')
    if ($versionParts.Count -lt 2) {
        return $null
    }

    $major = [int]$versionParts[0]
    $minor = [int]$versionParts[1]
    $compatible = ($major -eq 3 -and $minor -ge 11 -and $minor -lt 14)

    return [PSCustomObject]@{
        Path = $parts[1]
        Version = $parts[0]
        Major = $major
        Minor = $minor
        Compatible = $compatible
    }
}

function Resolve-Python {
    param([string]$Explicit)

    if ($Explicit) {
        if (-not (Test-Path $Explicit)) {
            throw "Specified Python executable not found: $Explicit"
        }
        $info = Get-CompatiblePythonInfo -Executable (Resolve-Path $Explicit).Path
        if (-not $info) {
            throw "Could not execute specified Python: $Explicit"
        }
        if (-not $info.Compatible) {
            throw "ESPHome $esphomeVersion requires Python >=3.11 and <3.14; specified Python is $($info.Version)"
        }
        return $info
    }

    $candidates = @()

    # Ask the Windows Python launcher for the interpreters that actually exist.
    # Do not probe missing versions with `py -3.x`: Windows PowerShell can turn
    # the launcher's "No suitable Python runtime found" stderr into an exception.
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

    # Common per-user/system CPython install paths.
    foreach ($minor in @(13, 12, 11)) {
        $candidates += (Join-Path $env:LOCALAPPDATA "Programs\Python\Python3$minor\python.exe")
        $candidates += (Join-Path $env:ProgramFiles "Python3$minor\python.exe")
        if (${env:ProgramFiles(x86)}) {
            $candidates += (Join-Path ${env:ProgramFiles(x86)} "Python3$minor\python.exe")
        }
    }

    foreach ($name in @("python", "python3")) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) { $candidates += $cmd.Source }
    }

    $detected = @()
    foreach ($candidate in ($candidates | Where-Object { $_ } | Select-Object -Unique)) {
        $info = Get-CompatiblePythonInfo -Executable $candidate
        if ($info) {
            $detected += $info
            if ($info.Compatible) {
                return $info
            }
        }
    }

    $detectedText = if ($detected.Count -gt 0) {
        (($detected | Sort-Object Major, Minor -Descending | ForEach-Object {
            "  Python $($_.Version) - $($_.Path)"
        }) -join "`n")
    }
    else {
        "  none"
    }

    throw @"
No ESPHome-compatible Python was found.

ESPHome $esphomeVersion requires Python >=3.11 and <3.14.
Detected Python interpreters:
$detectedText

Install Python 3.13 side-by-side with your existing Python 3.14:
  winget install -e --id Python.Python.3.13

Then verify:
  py -0p

Python 3.14 does not need to be removed. Rerun Test 33 after Python 3.13 is installed.
"@
}

$pythonInfo = Resolve-Python -Explicit $PythonPath
$python = $pythonInfo.Path
Write-Host "[PASS] Python: $python"
Write-Host "[PASS] Python version: $($pythonInfo.Version) (ESPHome-compatible)"

# Verify exact upstream state before introducing disposable local files.
& git -C $upstreamDir reset --hard $upstreamCommit
if ($LASTEXITCODE -ne 0) { throw "Could not reset pinned upstream source" }
& git -C $upstreamDir clean -fdx
if ($LASTEXITCODE -ne 0) { throw "Could not clean pinned upstream source" }

$head = (& git -C $upstreamDir rev-parse HEAD | Out-String).Trim()
if ($head -ne $upstreamCommit) { throw "Unexpected upstream HEAD: $head" }

$venvPython = Join-Path $venvDir "Scripts\python.exe"
$venvEspHome = Join-Path $venvDir "Scripts\esphome.exe"

# A previous Test 33 attempt may have created this venv with Python 3.14+.
# Reuse only an ESPHome-compatible environment; otherwise rebuild it.
if (Test-Path $venvPython) {
    $venvInfo = Get-CompatiblePythonInfo -Executable $venvPython
    if (-not $venvInfo -or -not $venvInfo.Compatible) {
        Write-Host "Removing incompatible previous virtual environment..."
        Remove-Item -Recurse -Force $venvDir
    }
}

if (-not (Test-Path $venvDir)) {
    Write-Host "Creating isolated Python environment with Python $($pythonInfo.Version)..."
    & $python -m venv $venvDir
    if ($LASTEXITCODE -ne 0) { throw "Python venv creation failed" }
}

if (-not (Test-Path $venvPython)) { throw "Virtual environment Python not found" }

$finalVenvInfo = Get-CompatiblePythonInfo -Executable $venvPython
if (-not $finalVenvInfo -or -not $finalVenvInfo.Compatible) {
    throw "Virtual environment Python is not compatible with ESPHome $esphomeVersion"
}
Write-Host "[PASS] Venv Python: $($finalVenvInfo.Version)"

Write-Host "Ensuring ESPHome $esphomeVersion..."
& $venvPython -m pip install --disable-pip-version-check --upgrade "esphome==$esphomeVersion"
if ($LASTEXITCODE -ne 0) { throw "ESPHome installation failed" }
if (-not (Test-Path $venvEspHome)) { throw "ESPHome executable not found in virtual environment" }

$versionText = (& $venvEspHome version | Out-String).Trim()
Write-Host "[PASS] $versionText"
if ($versionText -notmatch [regex]::Escape($esphomeVersion)) {
    throw "Unexpected ESPHome version: $versionText"
}

$escapedSsid = $WifiSsid.Replace("'", "''")
$escapedPassword = $WifiPassword.Replace("'", "''")
$secretsText = @"
wifi_ssid: '$escapedSsid'
wifi_password: '$escapedPassword'
"@
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($secrets, $secretsText, $utf8NoBom)

Push-Location $upstreamDir
try {
    Write-Host ""
    Write-Host "=== Test 33 ESPHome validation ==="
    & $venvEspHome config $example
    if ($LASTEXITCODE -ne 0) { throw "Test 33 ESPHome config validation failed" }

    Write-Host ""
    Write-Host "=== Test 33 historical build ==="
    Write-Host "Upstream commit : $upstreamCommit"
    Write-Host "ESPHome         : $esphomeVersion"
    Write-Host "Python          : $($finalVenvInfo.Version)"
    Write-Host "Target          : sunton-43-example.yaml"
    Write-Host "Architecture    : ESPHome / ESP-IDF / mipi_rgb / LVGL modular YAML / GT911"
    Write-Host ""

    & $venvEspHome compile $example
    if ($LASTEXITCODE -ne 0) { throw "Test 33 build failed" }

    if ($Upload) {
        Write-Host ""
        Write-Host "=== Test 33 upload ==="
        if ($UploadPort) {
            & $venvEspHome upload $example --device $UploadPort
        }
        else {
            & $venvEspHome upload $example
        }
        if ($LASTEXITCODE -ne 0) { throw "Test 33 upload failed" }
    }
    else {
        Write-Host ""
        Write-Host "Build complete. To upload:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\33_ESPHome_LVGL_RyanEwen\run-test33.ps1 -Upload"
        Write-Host ""
        Write-Host "Explicit port example:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\33_ESPHome_LVGL_RyanEwen\run-test33.ps1 -Upload -UploadPort COM7"
        Write-Host ""
        Write-Host "Optional real Wi-Fi credentials:"
        Write-Host "  ... -WifiSsid 'YOUR_SSID' -WifiPassword 'YOUR_PASSWORD'"
    }
}
finally {
    Pop-Location

    if (Test-Path $secrets) {
        Remove-Item -Force $secrets
    }

    # ESPHome writes .esphome and PlatformIO build files inside the upstream tree.
    # They are disposable build products and are removed before source verification.
    foreach ($generated in @(
        (Join-Path $upstreamDir ".esphome"),
        (Join-Path $upstreamDir ".pio")
    )) {
        if (Test-Path $generated) {
            Remove-Item -Recurse -Force $generated
        }
    }

    & git -C $upstreamDir reset --hard $upstreamCommit | Out-Null
    & git -C $upstreamDir clean -fdx | Out-Null

    $status = (& git -C $upstreamDir status --porcelain --untracked-files=all | Out-String).Trim()
    if ($status) {
        Write-Host "[FAIL] Upstream source tree changed after build:"
        Write-Host $status
        throw "Exact upstream source was not restored"
    }

    Write-Host "[PASS] Exact upstream source restored; ESPHome build artifacts removed"
}
