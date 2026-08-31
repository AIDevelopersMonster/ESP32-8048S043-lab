$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$projectDir = Join-Path $repoRoot ".external-test-work\21_LVGL9_ArduinoGFX_EEZ_clumsyCoder00\upstream\Sunton-ESP32-8048S043"
$platformioIni = Join-Path $projectDir "platformio.ini"
$pioDir = Join-Path $projectDir ".pio"
$historicalPlatform = "espressif32 @ 6.7.0"

if (-not (Test-Path $platformioIni)) {
    throw "Test 21 platformio.ini not found. Run prepare-test21.ps1 first."
}

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 21 historical platform layer ==="
Write-Host "Project             : $projectDir"
Write-Host "Upstream snapshot   : 2024-07-04"
Write-Host "Pinned platform     : $historicalPlatform"
Write-Host "Expected Arduino    : 2.0.16"
Write-Host "Expected ESP-IDF    : 4.4.7"
Write-Host ""

$ini = Get-Content $platformioIni
$platformLines = @($ini | Select-String -Pattern '^\s*platform\s*=')
if ($platformLines.Count -ne 1) {
    throw "Expected exactly one platform= line in platformio.ini, found $($platformLines.Count)"
}

$patched = $ini | ForEach-Object {
    if ($_ -match '^\s*platform\s*=') {
        "platform = $historicalPlatform"
    }
    else {
        $_
    }
}
$patched | Set-Content -Encoding ASCII $platformioIni

$verify = Select-String -Path $platformioIni -Pattern '^platform\s*=\s*espressif32\s*@\s*6\.7\.0\s*$'
if (-not $verify) {
    throw "Failed to pin platformio.ini to espressif32 @ 6.7.0"
}
Write-Host "[PASS] platformio.ini pinned to espressif32 @ 6.7.0"

if (Test-Path $pioDir) {
    Write-Host "[INFO] Removing cached PlatformIO build state..."
    Remove-Item $pioDir -Recurse -Force
}
if (Test-Path $pioDir) {
    throw "Failed to reset .pio directory"
}
Write-Host "[PASS] PlatformIO .pio state reset"

Write-Host ""
Write-Host "Pinned platformio.ini excerpt:"
Get-Content $platformioIni | Select-String -Pattern '^\[env:|^platform\s*=|^board\s*=|^framework\s*=|^board_build\.arduino\.memory_type|^board_upload\.flash_size|^build_flags'

$pio = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"
if (Test-Path $pio) {
    Write-Host ""
    Write-Host "PlatformIO Core:"
    & $pio --version
    Write-Host ""
    Write-Host "Next command:"
    Write-Host "  cd `"$projectDir`""
    Write-Host "  & `"$pio`" run"
}
