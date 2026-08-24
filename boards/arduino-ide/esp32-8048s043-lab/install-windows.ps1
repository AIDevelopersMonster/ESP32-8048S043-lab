param(
    [string]$CorePath = "",
    [string]$SourceBoardId = "esp32s3",
    [string]$BoardId = "esp32_8048s043_lab"
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[ESP32-8048S043] $Message"
}

function Set-BoardProperty {
    param(
        [string]$Text,
        [string]$Key,
        [string]$Value
    )

    $line = "$Key=$Value"
    $pattern = "(?m)^" + [regex]::Escape($Key) + "=.*$"
    if ([regex]::IsMatch($Text, $pattern)) {
        return [regex]::Replace($Text, $pattern, $line)
    }

    return ($Text.TrimEnd() + "`r`n" + $line + "`r`n")
}

$KitRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$VariantSource = Join-Path $KitRoot "variants\esp32_8048s043_lab"
$PartitionSource = Join-Path $KitRoot "partitions\esp32_8048s043_16m_lab.csv"

if (-not (Test-Path $VariantSource)) {
    throw "Variant source not found: $VariantSource"
}
if (-not (Test-Path $PartitionSource)) {
    throw "Partition source not found: $PartitionSource"
}

if ([string]::IsNullOrWhiteSpace($CorePath)) {
    $CoreBase = Join-Path $env:LOCALAPPDATA "Arduino15\packages\esp32\hardware\esp32"
    if (-not (Test-Path $CoreBase)) {
        throw "Espressif Arduino-ESP32 core directory not found: $CoreBase. Install 'esp32 by Espressif Systems' in Arduino IDE first."
    }

    $candidates = Get-ChildItem $CoreBase -Directory |
        Where-Object { Test-Path (Join-Path $_.FullName "boards.txt") } |
        Sort-Object LastWriteTime -Descending

    if (-not $candidates -or $candidates.Count -eq 0) {
        throw "No Arduino-ESP32 core versions with boards.txt were found under: $CoreBase"
    }

    $CorePath = $candidates[0].FullName
}

$BoardsTxt = Join-Path $CorePath "boards.txt"
$BoardsLocalTxt = Join-Path $CorePath "boards.local.txt"
$VariantTarget = Join-Path $CorePath "variants\esp32_8048s043_lab"
$PartitionTargetDir = Join-Path $CorePath "tools\partitions"
$PartitionTarget = Join-Path $PartitionTargetDir "esp32_8048s043_16m_lab.csv"

if (-not (Test-Path $BoardsTxt)) {
    throw "boards.txt not found: $BoardsTxt"
}

Write-Step "Using Arduino-ESP32 core: $CorePath"
Write-Step "Original boards.txt will not be modified. The custom board is written to boards.local.txt."

$boardsContent = Get-Content $BoardsTxt -Raw
$sourcePattern = "(?ms)^" + [regex]::Escape($SourceBoardId) + "\.name=.*?(?=^\S+\.name=|\z)"
$sourceMatch = [regex]::Match($boardsContent, $sourcePattern)
if (-not $sourceMatch.Success) {
    throw "Could not find source board block '$SourceBoardId' in: $BoardsTxt"
}

$newBlock = $sourceMatch.Value.TrimEnd()
$newBlock = [regex]::Replace($newBlock, "(?m)^" + [regex]::Escape($SourceBoardId) + "\.", "$BoardId.")

$newBlock = Set-BoardProperty $newBlock "$BoardId.name" "ESP32-8048S043 Lab (ESP32-S3 N16R8 RGB800x480 GT911)"
$newBlock = Set-BoardProperty $newBlock "$BoardId.build.variant" "esp32_8048s043_lab"
$newBlock = Set-BoardProperty $newBlock "$BoardId.build.board" "ESP32_8048S043_LAB"

$extraKey = "$BoardId.build.extra_flags"
$extraMatch = [regex]::Match($newBlock, "(?m)^" + [regex]::Escape($extraKey) + "=(.*)$")
$projectFlags = "-DARDUINO_ESP32_8048S043_LAB -DESP32_8048S043_HAS_RGB_PANEL=1 -DESP32_8048S043_HAS_GT911=1 -DESP32_8048S043_LCD_WIDTH=800 -DESP32_8048S043_LCD_HEIGHT=480 -DESP32_8048S043_TOUCH_GT911=1"
if ($extraMatch.Success) {
    $currentFlags = $extraMatch.Groups[1].Value.Trim()
    if ($currentFlags -notmatch "ARDUINO_ESP32_8048S043_LAB") {
        $newFlags = ($currentFlags + " " + $projectFlags).Trim()
        $newBlock = Set-BoardProperty $newBlock $extraKey $newFlags
    }
} else {
    $newBlock = Set-BoardProperty $newBlock $extraKey $projectFlags
}

$partitionMenu = @"
$BoardId.menu.PartitionScheme.esp32_8048s043_16m_lab=ESP32-8048S043 Lab 16M (3MB APP/9.9MB SPIFFS)
$BoardId.menu.PartitionScheme.esp32_8048s043_16m_lab.build.partitions=esp32_8048s043_16m_lab
$BoardId.menu.PartitionScheme.esp32_8048s043_16m_lab.upload.maximum_size=3145728
"@.TrimEnd()

New-Item -ItemType Directory -Path $VariantTarget -Force | Out-Null
Copy-Item -Path (Join-Path $VariantSource "*") -Destination $VariantTarget -Recurse -Force
Write-Step "Variant copied: $VariantTarget"

New-Item -ItemType Directory -Path $PartitionTargetDir -Force | Out-Null
Copy-Item -Path $PartitionSource -Destination $PartitionTarget -Force
Write-Step "Partition CSV copied: $PartitionTarget"

$begin = "# BEGIN ESP32-8048S043 Lab custom board"
$end = "# END ESP32-8048S043 Lab custom board"
$markerPattern = "(?ms)^" + [regex]::Escape($begin) + ".*?^" + [regex]::Escape($end) + "\r?\n?"

$localContent = ""
if (Test-Path $BoardsLocalTxt) {
    $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $backup = "$BoardsLocalTxt.bak-$timestamp"
    Copy-Item $BoardsLocalTxt $backup -Force
    Write-Step "Existing boards.local.txt backed up: $backup"
    $localContent = Get-Content $BoardsLocalTxt -Raw
    $localContent = [regex]::Replace($localContent, $markerPattern, "")
}

$generated = @"
$begin
# Generated by ESP32-8048S043-lab install-windows.ps1.
# Source board cloned from '$SourceBoardId'.
# Original boards.txt is not modified.
$newBlock

# Project custom partition menu option.
$partitionMenu
$end
"@

if ([string]::IsNullOrWhiteSpace($localContent)) {
    Set-Content -Path $BoardsLocalTxt -Value $generated -Encoding UTF8
} else {
    Set-Content -Path $BoardsLocalTxt -Value ($localContent.TrimEnd() + "`r`n`r`n" + $generated) -Encoding UTF8
}

Write-Step "Custom board entry written: $BoardsLocalTxt"
Write-Step "Done. Restart Arduino IDE completely."
Write-Host ""
Write-Host "Select board: ESP32-8048S043 Lab (ESP32-S3 N16R8 RGB800x480 GT911)"
Write-Host "Recommended Tools menu values:"
Write-Host "  Flash Size       : 16MB (128Mb)"
Write-Host "  Flash Mode       : QIO 80MHz"
Write-Host "  PSRAM            : OPI PSRAM"
Write-Host "  Partition Scheme : ESP32-8048S043 Lab 16M (3MB APP/9.9MB SPIFFS)"
Write-Host "  Upload Speed     : 921600, fallback 460800"
Write-Host ""
Write-Host "First validation example: File -> Examples -> ESP32_8048S043 -> 01_BoardInfo"
