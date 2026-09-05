param(
    [switch]$Flash,
    [string]$Port = "",
    [string]$IdfPath = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Join-Path $scriptDir "idf"

function Add-Candidate {
    param([System.Collections.Generic.List[string]]$List, [string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return }
    $candidate = $Path.Trim('"')
    if (Test-Path $candidate -PathType Leaf) {
        $leaf = Split-Path $candidate -Leaf
        if ($leaf -ieq "idf.py") { $candidate = Split-Path (Split-Path $candidate -Parent) -Parent }
        elseif ($leaf -ieq "export.ps1") { $candidate = Split-Path $candidate -Parent }
    }
    if (Test-Path (Join-Path $candidate "tools\idf.py")) {
        $resolved = (Resolve-Path $candidate).Path
        if (-not $List.Contains($resolved)) { $List.Add($resolved) }
    }
}

function Discover-IdfRoots {
    param([string]$ExplicitPath)
    $roots = New-Object 'System.Collections.Generic.List[string]'
    Add-Candidate $roots $ExplicitPath
    Add-Candidate $roots $env:IDF_PATH
    $cmd = Get-Command idf.py -ErrorAction SilentlyContinue
    if ($cmd -and $cmd.Source) { Add-Candidate $roots $cmd.Source }

    foreach ($p in @(
        "C:\Espressif\frameworks\esp-idf-v5.5.5",
        "$HOME\esp\v5.5.5\esp-idf",
        (Join-Path $env:LOCALAPPDATA "Espressif\frameworks\esp-idf-v5.5.5")
    )) { Add-Candidate $roots $p }

    return $roots
}

function Select-Idf55 {
    param([System.Collections.Generic.List[string]]$Roots)
    foreach ($root in $Roots) {
        if ($root -match "5\.5") { return $root }
        $versionFile = Join-Path $root "version.txt"
        if (Test-Path $versionFile) {
            $v = ([System.IO.File]::ReadAllText($versionFile)).Trim()
            if ($v -match "5\.5") { return $root }
        }
    }
    return $null
}

$roots = @(Discover-IdfRoots $IdfPath)
$idfRoot = Select-Idf55 $roots
if (-not $idfRoot) {
    throw "ESP-IDF 5.5.x not found. Pass -IdfPath C:\path\to\esp-idf-v5.5.5"
}

$export = Join-Path $idfRoot "export.ps1"
if (-not (Test-Path $export)) { throw "export.ps1 not found: $export" }
. $export
if ($LASTEXITCODE -ne 0) { throw "ESP-IDF export failed" }

$idfPy = Join-Path $idfRoot "tools\idf.py"
Push-Location $projectDir
try {
    $version = (& python $idfPy --version 2>&1 | Out-String).Trim()
    Write-Host "ESP-IDF: $version"
    if ($version -notmatch "5\.5") { throw "App 01 requires ESP-IDF 5.5.x" }

    & python $idfPy set-target esp32s3
    if ($LASTEXITCODE -ne 0) { throw "set-target failed" }

    & python $idfPy build
    if ($LASTEXITCODE -ne 0) { throw "App 01 build failed" }
    Write-Host "[PASS] App 01 build complete"

    if ($Flash) {
        if ($Port) { & python $idfPy -p $Port flash }
        else { & python $idfPy flash }
        if ($LASTEXITCODE -ne 0) { throw "App 01 flash failed" }
        Write-Host "[PASS] App 01 flashed"
    }
}
finally {
    Pop-Location
}
