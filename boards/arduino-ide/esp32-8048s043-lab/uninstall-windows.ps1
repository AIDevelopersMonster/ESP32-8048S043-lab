param(
    [string]$CorePath = ""
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[ESP32-8048S043] $Message"
}

if ([string]::IsNullOrWhiteSpace($CorePath)) {
    $CoreBase = Join-Path $env:LOCALAPPDATA "Arduino15\packages\esp32\hardware\esp32"
    if (-not (Test-Path $CoreBase)) {
        throw "Espressif Arduino-ESP32 core directory not found: $CoreBase"
    }

    $candidates = Get-ChildItem $CoreBase -Directory |
        Where-Object { Test-Path (Join-Path $_.FullName "boards.txt") } |
        Sort-Object LastWriteTime -Descending

    if (-not $candidates -or $candidates.Count -eq 0) {
        throw "No Arduino-ESP32 core versions with boards.txt were found under: $CoreBase"
    }

    $CorePath = $candidates[0].FullName
}

$BoardsLocalTxt = Join-Path $CorePath "boards.local.txt"
$VariantTarget = Join-Path $CorePath "variants\esp32_8048s043_lab"
$PartitionTarget = Join-Path $CorePath "tools\partitions\esp32_8048s043_16m_lab.csv"

Write-Step "Using Arduino-ESP32 core: $CorePath"

$begin = "# BEGIN ESP32-8048S043 Lab custom board"
$end = "# END ESP32-8048S043 Lab custom board"
$markerPattern = "(?ms)^" + [regex]::Escape($begin) + ".*?^" + [regex]::Escape($end) + "\r?\n?"

if (Test-Path $BoardsLocalTxt) {
    $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $backup = "$BoardsLocalTxt.bak-uninstall-$timestamp"
    Copy-Item $BoardsLocalTxt $backup -Force
    Write-Step "boards.local.txt backed up: $backup"

    $content = Get-Content $BoardsLocalTxt -Raw
    $content = [regex]::Replace($content, $markerPattern, "")
    Set-Content -Path $BoardsLocalTxt -Value $content.TrimEnd() -Encoding UTF8
    Write-Step "Custom board block removed from boards.local.txt"
} else {
    Write-Step "boards.local.txt not found; nothing to remove there"
}

if (Test-Path $VariantTarget) {
    Remove-Item $VariantTarget -Recurse -Force
    Write-Step "Variant removed: $VariantTarget"
}

if (Test-Path $PartitionTarget) {
    Remove-Item $PartitionTarget -Force
    Write-Step "Partition CSV removed: $PartitionTarget"
}

Write-Step "Done. Restart Arduino IDE completely."
