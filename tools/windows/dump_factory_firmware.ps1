param(
    [Parameter(Mandatory=$true)]
    [string]$Port,

    [int]$Baud = 921600,
    [int]$SizeMB = 16,
    [int]$Reads = 2,
    [string]$OutDir = "evidence\specimens\sample-a\factory-firmware",
    [switch]$VerboseEsptool,
    [int]$FailTailLines = 12
)

$ErrorActionPreference = "Stop"

function Show-CompactFailure {
    param(
        [string]$Text,
        [int]$TailLines
    )

    if (-not $Text) { return }

    $lines = $Text -split "`r?`n" | Where-Object { $_.Trim().Length -gt 0 }
    $important = $lines | Where-Object {
        $_ -match "A fatal error|Corrupt data|PermissionError|Cannot configure port|could not open port|Failed to|Timed out|Invalid|Serial exception|No serial data|port is busy|error occurred"
    }

    if ($important.Count -gt 0) {
        $important | Select-Object -Last $TailLines | ForEach-Object { Write-Host "  $_" }
    } else {
        $lines | Select-Object -Last $TailLines | ForEach-Object { Write-Host "  $_" }
    }
}

function Invoke-LoggedPy {
    param(
        [string]$Step,
        [string[]]$Arguments,
        [string]$LogFile
    )

    $display = "py " + ($Arguments -join " ")
    Write-Host "$Step ... " -NoNewline
    Add-Content -Path $LogFile -Value "> $display"

    $tmpOut = [System.IO.Path]::GetTempFileName()
    $tmpErr = [System.IO.Path]::GetTempFileName()

    try {
        $p = Start-Process -FilePath "py" -ArgumentList $Arguments -NoNewWindow -Wait -PassThru -RedirectStandardOutput $tmpOut -RedirectStandardError $tmpErr
        $outText = Get-Content -Raw -Path $tmpOut -ErrorAction SilentlyContinue
        $errText = Get-Content -Raw -Path $tmpErr -ErrorAction SilentlyContinue
        $allText = ($outText + "`n" + $errText)

        if ($outText) { Add-Content -Path $LogFile -Value $outText }
        if ($errText) { Add-Content -Path $LogFile -Value $errText }
        Add-Content -Path $LogFile -Value "EXIT_CODE: $($p.ExitCode)"

        if ($p.ExitCode -eq 0) {
            Write-Host "OK"
            if ($VerboseEsptool) {
                if ($outText) { Write-Host $outText }
                if ($errText) { Write-Host $errText }
            }
        } else {
            Write-Host "FAIL $($p.ExitCode)" -ForegroundColor Red
            Show-CompactFailure $allText $FailTailLines
            throw "Command failed with exit code $($p.ExitCode): $display"
        }
    }
    finally {
        Remove-Item -Force -ErrorAction SilentlyContinue $tmpOut, $tmpErr
    }
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$outPath = Join-Path $repoRoot $OutDir
New-Item -ItemType Directory -Force -Path $outPath | Out-Null

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$sizeBytes = $SizeMB * 1024 * 1024
$sizeHex = "0x{0:X}" -f $sizeBytes
$logFile = Join-Path $outPath "factory-dump-$timestamp.log"
$hashFile = Join-Path $outPath "factory-dump-$timestamp.sha256.txt"

Add-Content -Path $logFile -Value "ESP32-8048S043 factory firmware dump"
Add-Content -Path $logFile -Value "Timestamp: $timestamp"
Add-Content -Path $logFile -Value "Port: $Port"
Add-Content -Path $logFile -Value "Baud: $Baud"
Add-Content -Path $logFile -Value "SizeMB: $SizeMB"
Add-Content -Path $logFile -Value "Reads: $Reads"
Add-Content -Path $logFile -Value ""

Write-Host "FACTORY DUMP  port=$Port  flash=${SizeMB}MB  reads=$Reads  baud=$Baud"

Invoke-LoggedPy "chip_id" @("-m", "esptool", "--chip", "esp32s3", "--port", $Port, "--baud", "$Baud", "chip-id") $logFile
Invoke-LoggedPy "flash_id" @("-m", "esptool", "--chip", "esp32s3", "--port", $Port, "--baud", "$Baud", "flash-id") $logFile

$hashes = @()
for ($i = 1; $i -le $Reads; $i++) {
    $dumpFile = Join-Path $outPath ("factory-flash-read{0}-{1}mb.bin" -f $i, $SizeMB)
    Invoke-LoggedPy "read_flash $i/$Reads" @("-m", "esptool", "--chip", "esp32s3", "--port", $Port, "--baud", "$Baud", "read-flash", "0x000000", $sizeHex, $dumpFile) $logFile

    $hash = Get-FileHash -Algorithm SHA256 -Path $dumpFile
    $hashes += $hash.Hash
    Add-Content -Path $hashFile -Value ("{0}  {1}" -f $hash.Hash, $dumpFile)
    Write-Host ("sha256 read{0} {1}" -f $i, $hash.Hash)
}

if ($hashes.Count -gt 1) {
    $unique = $hashes | Select-Object -Unique
    if ($unique.Count -eq 1) {
        Add-Content -Path $hashFile -Value "MATCH: all reads are identical"
        Write-Host "MATCH all reads are identical"
    } else {
        Add-Content -Path $hashFile -Value "MISMATCH: repeated reads differ"
        Write-Host "MISMATCH repeated reads differ" -ForegroundColor Red
        exit 2
    }
}

$stableDump = Join-Path $outPath ("factory-flash-{0}mb.bin" -f $SizeMB)
Copy-Item -Force -Path (Join-Path $outPath ("factory-flash-read1-{0}mb.bin" -f $SizeMB)) -Destination $stableDump
Add-Content -Path $hashFile -Value ("STABLE_COPY  {0}" -f $stableDump)

Write-Host "DONE dump=$stableDump log=$logFile hashes=$hashFile"
Write-Host "NEXT py tools\analysis\firmware_scan.py '$stableDump' --out '$outPath\analysis'"
