param(
    [Parameter(Mandatory=$true)]
    [string]$Port,

    [int]$Baud = 921600,
    [int]$SizeMB = 16,
    [int]$Reads = 2,
    [string]$OutDir = "evidence\specimens\sample-a\factory-firmware"
)

$ErrorActionPreference = "Stop"

function Invoke-LoggedPy {
    param(
        [string[]]$Arguments,
        [string]$LogFile
    )

    $display = "py " + ($Arguments -join " ")
    Write-Host "> $display"
    Add-Content -Path $LogFile -Value "> $display"

    # esptool sometimes writes normal progress/reset messages to stderr.
    # Windows PowerShell can wrap that as NativeCommandError when stderr is redirected.
    # Treat stderr as log output, then rely on the native exit code instead.
    $oldEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & py @Arguments 2>&1 | ForEach-Object {
            $line = $_.ToString()
            Write-Host $line
            Add-Content -Path $LogFile -Value $line
        }
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $oldEap
    }

    Add-Content -Path $LogFile -Value "EXIT_CODE: $exitCode"
    if ($exitCode -ne 0) {
        throw "Command failed with exit code $exitCode: $display"
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

Invoke-LoggedPy @("-m", "esptool", "--chip", "esp32s3", "--port", $Port, "--baud", "$Baud", "chip_id") $logFile
Invoke-LoggedPy @("-m", "esptool", "--chip", "esp32s3", "--port", $Port, "--baud", "$Baud", "flash_id") $logFile

$hashes = @()
for ($i = 1; $i -le $Reads; $i++) {
    $dumpFile = Join-Path $outPath ("factory-flash-read{0}-{1}mb.bin" -f $i, $SizeMB)
    Invoke-LoggedPy @("-m", "esptool", "--chip", "esp32s3", "--port", $Port, "--baud", "$Baud", "read_flash", "0x000000", $sizeHex, $dumpFile) $logFile

    $hash = Get-FileHash -Algorithm SHA256 -Path $dumpFile
    $hashes += $hash.Hash
    Add-Content -Path $hashFile -Value ("{0}  {1}" -f $hash.Hash, $dumpFile)
    Write-Host ("SHA256 read {0}: {1}" -f $i, $hash.Hash)
}

if ($hashes.Count -gt 1) {
    $unique = $hashes | Select-Object -Unique
    if ($unique.Count -eq 1) {
        Add-Content -Path $hashFile -Value "MATCH: all reads are identical"
        Write-Host "MATCH: all reads are identical"
    } else {
        Add-Content -Path $hashFile -Value "MISMATCH: repeated reads differ"
        Write-Host "MISMATCH: repeated reads differ" -ForegroundColor Red
        exit 2
    }
}

$stableDump = Join-Path $outPath ("factory-flash-{0}mb.bin" -f $SizeMB)
Copy-Item -Force -Path (Join-Path $outPath ("factory-flash-read1-{0}mb.bin" -f $SizeMB)) -Destination $stableDump
Add-Content -Path $hashFile -Value ("STABLE_COPY  {0}" -f $stableDump)

Write-Host ""
Write-Host "Factory firmware dump complete."
Write-Host "Dump directory: $outPath"
Write-Host "Log: $logFile"
Write-Host "Hashes: $hashFile"
Write-Host "Stable copy: $stableDump"
Write-Host ""
Write-Host "Next analysis command:"
Write-Host "py tools\analysis\firmware_scan.py '$stableDump' --out '$outPath\analysis'"
