param(
    [switch]$Flash,
    [string]$Port = "",
    [string]$IdfPath = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$workRoot = Join-Path $repoRoot ".third-party-work\31_LVGL9_NativeIDF_duck4i"
$upstreamDir = Join-Path $workRoot "upstream"
$projectDir = Join-Path $upstreamDir "st7262"
$prep = Join-Path $scriptDir "prepare-test31.ps1"
$requiredIdfVersion = "5.3.2"

if (-not (Test-Path $upstreamDir)) {
    & powershell -ExecutionPolicy Bypass -File $prep
    if ($LASTEXITCODE -ne 0) { throw "prepare-test31.ps1 failed" }
}

function Add-Candidate {
    param(
        [System.Collections.Generic.List[string]]$List,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) { return }

    # Accept an IDF root, tools\idf.py, or export.ps1.
    $candidate = $Path.Trim('"')
    if (Test-Path $candidate -PathType Leaf) {
        $leaf = Split-Path $candidate -Leaf
        if ($leaf -ieq "idf.py") {
            $candidate = Split-Path (Split-Path $candidate -Parent) -Parent
        }
        elseif ($leaf -ieq "export.ps1") {
            $candidate = Split-Path $candidate -Parent
        }
    }

    if (Test-Path (Join-Path $candidate "tools\idf.py")) {
        $resolved = (Resolve-Path $candidate).Path
        if (-not $List.Contains($resolved)) {
            $List.Add($resolved)
        }
    }
}

function Discover-IdfRoots {
    param([string]$ExplicitPath)

    $candidates = New-Object 'System.Collections.Generic.List[string]'

    Add-Candidate -List $candidates -Path $ExplicitPath
    Add-Candidate -List $candidates -Path $env:IDF_PATH

    # Active PATH, if any.
    $idfCmd = Get-Command idf.py -ErrorAction SilentlyContinue
    if ($idfCmd -and $idfCmd.Source) {
        Add-Candidate -List $candidates -Path $idfCmd.Source
    }

    # Standard Espressif Windows installer / common manual layouts.
    $known = @(
        "C:\Espressif\frameworks\esp-idf-v5.3.2",
        "C:\Espressif\frameworks\esp-idf-v5.3",
        (Join-Path $env:USERPROFILE "esp\v5.3.2\esp-idf"),
        (Join-Path $env:USERPROFILE "esp\esp-idf-v5.3.2"),
        (Join-Path $env:USERPROFILE "esp\esp-idf"),
        (Join-Path $env:LOCALAPPDATA "Espressif\frameworks\esp-idf-v5.3.2"),
        (Join-Path $env:LOCALAPPDATA "Espressif\frameworks\esp-idf-v5.3")
    )

    foreach ($path in $known) {
        Add-Candidate -List $candidates -Path $path
    }

    # Targeted shallow discovery only; do not recursively scan the whole drive.
    $frameworkRoots = @(
        "C:\Espressif\frameworks",
        (Join-Path $env:LOCALAPPDATA "Espressif\frameworks"),
        (Join-Path $env:USERPROFILE "esp")
    )

    foreach ($root in $frameworkRoots) {
        if (-not (Test-Path $root -PathType Container)) { continue }

        Get-ChildItem -Path $root -Directory -ErrorAction SilentlyContinue | ForEach-Object {
            Add-Candidate -List $candidates -Path $_.FullName

            # Handles layouts such as %USERPROFILE%\esp\v5.3.2\esp-idf.
            Get-ChildItem -Path $_.FullName -Directory -ErrorAction SilentlyContinue | ForEach-Object {
                Add-Candidate -List $candidates -Path $_.FullName
            }
        }
    }

    return $candidates
}

function Get-IdfSourceVersionHint {
    param([string]$Root)

    $versionFile = Join-Path $Root "version.txt"
    if (Test-Path $versionFile) {
        $text = ([System.IO.File]::ReadAllText($versionFile)).Trim()
        if ($text) { return $text }
    }

    if (Test-Path (Join-Path $Root ".git")) {
        $gitVersion = (& git -C $Root describe --tags --always 2>$null | Out-String).Trim()
        if ($gitVersion) { return $gitVersion }
    }

    return ""
}

function Select-Idf532 {
    param([System.Collections.Generic.List[string]]$Roots)

    if ($Roots.Count -eq 0) { return $null }

    Write-Host "Detected ESP-IDF source candidates:"
    foreach ($root in $Roots) {
        $hint = Get-IdfSourceVersionHint -Root $root
        if ($hint) {
            Write-Host "  $root  [$hint]"
        }
        else {
            Write-Host "  $root"
        }
    }
    Write-Host ""

    # Prefer a candidate whose source metadata explicitly identifies 5.3.2.
    foreach ($root in $Roots) {
        $hint = Get-IdfSourceVersionHint -Root $root
        if ($hint -match "(^|v|[^0-9])5\.3\.2([^0-9]|$)") {
            return $root
        }
    }

    # Also accept a conventional folder name; final idf.py --version validation
    # below is authoritative after environment activation.
    foreach ($root in $Roots) {
        if ($root -match "5\.3\.2") {
            return $root
        }
    }

    return $null
}

function Activate-Idf {
    param([string]$Root)

    $exportScript = Join-Path $Root "export.ps1"
    if (-not (Test-Path $exportScript)) {
        throw "ESP-IDF export.ps1 not found: $exportScript"
    }

    Write-Host "Activating ESP-IDF environment: $Root"
    $env:IDF_PATH = $Root

    # Dot-source so PATH, Python virtual environment and ESP-IDF tool variables
    # remain active inside this runner process.
    . $exportScript

    if ($LASTEXITCODE -ne 0) {
        throw "ESP-IDF export.ps1 failed"
    }
}

$roots = @(Discover-IdfRoots -ExplicitPath $IdfPath)
$idfRoot = Select-Idf532 -Roots $roots

if (-not $idfRoot) {
    Write-Host "ESP-IDF 5.3.2 source installation was not found."
    Write-Host ""
    Write-Host "Searched the active environment and common Windows locations including:"
    Write-Host "  C:\Espressif\frameworks\esp-idf-v5.3.2"
    Write-Host "  $env:USERPROFILE\esp\v5.3.2\esp-idf"
    Write-Host "  $env:USERPROFILE\esp\esp-idf-v5.3.2"
    Write-Host ""
    Write-Host "If 5.3.2 is installed elsewhere, rerun with:"
    Write-Host "  -IdfPath C:\path\to\esp-idf-v5.3.2"
    Write-Host ""
    throw "ESP-IDF 5.3.2 not installed or not discoverable"
}

Activate-Idf -Root $idfRoot

$idfPy = Join-Path $idfRoot "tools\idf.py"
if (-not (Test-Path $idfPy)) {
    throw "idf.py not found after activation: $idfPy"
}

$pythonCmd = Get-Command python -ErrorAction SilentlyContinue
if (-not $pythonCmd) {
    throw "Python was not placed on PATH by ESP-IDF export.ps1"
}

Write-Host "[PASS] ESP-IDF root : $idfRoot"
Write-Host "[PASS] idf.py       : $idfPy"
Write-Host "[PASS] Python       : $($pythonCmd.Source)"

Push-Location $projectDir
try {
    $versionOutput = & python $idfPy --version 2>&1
    if ($LASTEXITCODE -ne 0) { throw "idf.py failed to start" }
    $versionText = ($versionOutput | Out-String).Trim()
    Write-Host "ESP-IDF reported: $versionText"

    if ($versionText -notmatch "v?5\.3\.2") {
        throw "Test 31 requires ESP-IDF 5.3.2. Activated environment reports: $versionText"
    }

    Write-Host "[PASS] Exact ESP-IDF 5.3.2 environment active"
    Write-Host ""
    Write-Host "=== Test 31 build ==="
    Write-Host "Upstream commit : 578966c969577309b37cf9afb698852e2e81491b"
    Write-Host "ESP-IDF         : 5.3.2"
    Write-Host "LVGL            : 9.2.2"
    Write-Host "Architecture    : native esp_lcd RGB / PSRAM FB / INTERNAL partial LVGL / bounce disabled"
    Write-Host ""

    & python $idfPy set-target esp32s3
    if ($LASTEXITCODE -ne 0) { throw "idf.py set-target failed" }

    & python $idfPy build
    if ($LASTEXITCODE -ne 0) { throw "Test 31 build failed" }

    if ($Flash) {
        Write-Host ""
        Write-Host "=== Test 31 flash ==="
        if ($Port) {
            & python $idfPy -p $Port flash
        }
        else {
            & python $idfPy flash
        }
        if ($LASTEXITCODE -ne 0) { throw "Test 31 flash failed" }
    }
    else {
        Write-Host ""
        Write-Host "Build complete. To flash:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\31_LVGL9_NativeIDF_duck4i\run-test31.ps1 -Flash"
        Write-Host ""
        Write-Host "If a COM port is required explicitly:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\third_party_tests\31_LVGL9_NativeIDF_duck4i\run-test31.ps1 -Flash -Port COM7"
    }
}
finally {
    Pop-Location
}
