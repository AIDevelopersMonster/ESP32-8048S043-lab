# Local Arduino hardware platform setup

Status: `VALIDATED ON SAMPLE A / LOCAL DEVELOPMENT PROFILE / NOT BOARD MANAGER PACKAGE`.

This guide describes the local Arduino IDE board profile used for the ESP32-8048S043 Lab project.

It is intended as a reader-facing reference for setting up a custom Arduino target without modifying the installed Espressif Arduino-ESP32 package.

Validated board target:

```text
AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
```

Validated local platform path:

```text
Documents/Arduino/hardware/AIDevelopersMonster/esp32
```

## Boundary

This is a local sketchbook hardware platform, not a public Arduino Boards Manager package.

The installed Espressif core under Arduino15 remains untouched:

```text
%LOCALAPPDATA%/Arduino15/packages/esp32/hardware/esp32/<version>
```

The local platform is disposable. Rollback is just removing:

```text
Documents/Arduino/hardware/AIDevelopersMonster/esp32
```

## What was validated on Sample A

The local board profile was validated on Sample A with the direct Arduino examples in this order:

```text
01_BoardInfo       compile / upload / serial / ESP32-S3 / flash / PSRAM / app partition
02_DisplayRGBTest  RGB display / Arduino_GFX / 800x480 / color bars / stripe pattern
03_TouchGT911Test  GT911 I2C / product ID / firmware register / touch points
04_BacklightTest   GPIO2 backlight ON/OFF/blink/PWM duty stepping
05_TestConsole     combined RGB + GT911 + backlight diagnostic console
```

Important memory boundary:

```text
01_BoardInfo under the local board profile reported 16 MB flash, 8 MB OPI PSRAM and a 3 MB app partition.
05_TestConsole validated RGB + GT911 + backlight integration, but its serial report in the observed run printed PSRAM as 0 bytes. Treat 01_BoardInfo as the current PSRAM acceptance test and re-check PSRAM after platform/profile changes.
```

## Required Arduino IDE tools

Install before using the examples:

```text
Arduino IDE 2.x
esp32 by Espressif Systems
Arduino_GFX_Library by moononournation
```

Use the board USB-UART bridge as the upload port. On Sample A it appeared as a CH340 USB-serial port.

## Step 1: copy the installed Espressif ESP32 platform locally

Close Arduino IDE first.

Open PowerShell:

```powershell
$ErrorActionPreference = "Stop"

$CoreBase = Join-Path $env:LOCALAPPDATA "Arduino15\packages\esp32\hardware\esp32"
$Core = Get-ChildItem $CoreBase -Directory |
    Where-Object { Test-Path (Join-Path $_.FullName "boards.txt") } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if (-not $Core) {
    throw "Could not find installed Espressif Arduino-ESP32 core under $CoreBase"
}

$LocalPlatform = "$HOME\Documents\Arduino\hardware\AIDevelopersMonster\esp32"
New-Item -ItemType Directory -Force -Path (Split-Path $LocalPlatform -Parent) | Out-Null

Write-Host "Source core:"
Write-Host $Core.FullName
Write-Host ""
Write-Host "Local platform:"
Write-Host $LocalPlatform
Write-Host ""

robocopy $Core.FullName $LocalPlatform /MIR /XD .git
if ($LASTEXITCODE -gt 7) {
    throw "robocopy failed with exit code $LASTEXITCODE"
}

Write-Host "Local ESP32 platform copied."
```

## Step 2: install the project variant

The project keeps a staged variant file in the repository:

```text
boards/arduino-ide/esp32-8048s043-lab/variants/esp32_8048s043_lab/pins_arduino.h
```

Copy it into the local platform:

```powershell
$ErrorActionPreference = "Stop"

$Repo = "$HOME\Documents\GitHub\ESP32-8048S043-lab"
$LocalPlatform = "$HOME\Documents\Arduino\hardware\AIDevelopersMonster\esp32"

$VariantDir = Join-Path $LocalPlatform "variants\esp32_8048s043_lab"
New-Item -ItemType Directory -Force -Path $VariantDir | Out-Null

$SourcePins = Join-Path $Repo "boards\arduino-ide\esp32-8048s043-lab\variants\esp32_8048s043_lab\pins_arduino.h"
$TargetPins = Join-Path $VariantDir "pins_arduino.h"

if (-not (Test-Path $SourcePins)) {
    throw "Source pins_arduino.h not found: $SourcePins"
}

Copy-Item $SourcePins $TargetPins -Force

Write-Host "Variant installed:"
Write-Host $TargetPins
```

The variant supplies Arduino-level aliases and project macros. The RGB panel, GT911 driver and backlight logic remain in the `ESP32_8048S043` library.

## Step 3: create the local board target from the original esp32s3 block

This script rebuilds local `boards.txt` from the installed Espressif `esp32s3` board block and keeps only the validated options for this board.

It intentionally removes earlier experimental board IDs and leaves one local board target:

```text
esp32_8048s043_lab_n16r8
```

PowerShell:

```powershell
$ErrorActionPreference = "Stop"

$Platform = "$HOME\Documents\Arduino\hardware\AIDevelopersMonster\esp32"
$BoardsFile = Join-Path $Platform "boards.txt"

$BoardId = "esp32_8048s043_lab_n16r8"
$SourceId = "esp32s3"

$CoreBase = Join-Path $env:LOCALAPPDATA "Arduino15\packages\esp32\hardware\esp32"
$Core = Get-ChildItem $CoreBase -Directory |
    Where-Object { Test-Path (Join-Path $_.FullName "boards.txt") } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if (-not $Core) {
    throw "Could not find installed Espressif Arduino-ESP32 core under $CoreBase"
}

$SourceBoards = Join-Path $Core.FullName "boards.txt"

Write-Host "Using original Espressif boards.txt:"
Write-Host $SourceBoards
Write-Host ""

$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$Backup = "$BoardsFile.bak-rebuild-n16r8-$Stamp"

if (Test-Path $BoardsFile) {
    Copy-Item $BoardsFile $Backup -Force
    Write-Host "Backup created:"
    Write-Host $Backup
    Write-Host ""
}

$SourceAll = Get-Content $SourceBoards -Raw

$HeaderMatch = [regex]::Match($SourceAll, "(?ms)\A.*?(?=^\S+\.name=)")
if (-not $HeaderMatch.Success) {
    throw "Could not extract source boards.txt header"
}
$Header = $HeaderMatch.Value.TrimEnd()

$SourcePattern = "(?ms)^" + [regex]::Escape($SourceId) + "\.name=.*?(?=^\S+\.name=|\z)"
$SourceMatch = [regex]::Match($SourceAll, $SourcePattern)
if (-not $SourceMatch.Success) {
    throw "Could not find original esp32s3 block"
}

$Block = $SourceMatch.Value.TrimEnd()
$Block = [regex]::Replace($Block, "(?m)^" + [regex]::Escape($SourceId) + "\.", "$BoardId.")

function Set-BoardLine {
    param([string]$Text, [string]$Key, [string]$Value)
    $Line = "$Key=$Value"
    $Pat = "(?m)^" + [regex]::Escape($Key) + "=.*$"
    if ([regex]::IsMatch($Text, $Pat)) {
        return [regex]::Replace($Text, $Pat, $Line)
    }
    return $Text.TrimEnd() + "`r`n" + $Line + "`r`n"
}

function Add-BoardFlag {
    param([string]$Text, [string]$Key, [string]$Flag)
    $Pat = "(?m)^" + [regex]::Escape($Key) + "=(.*)$"
    $Match = [regex]::Match($Text, $Pat)
    if ($Match.Success) {
        $Current = $Match.Groups[1].Value.Trim()
        if ($Current -notmatch [regex]::Escape($Flag)) {
            $Current = "$Current $Flag"
        }
        return [regex]::Replace($Text, $Pat, "$Key=$Current")
    }
    return $Text.TrimEnd() + "`r`n" + "$Key=$Flag" + "`r`n"
}

function Find-MenuOption {
    param([string]$Text, [string]$Menu, [string]$Needle1, [string]$Needle2 = "")
    $MatchesList = [regex]::Matches(
        $Text,
        "(?m)^" + [regex]::Escape($BoardId) + "\.menu\." + [regex]::Escape($Menu) + "\.([^.=\r\n]+)=([^\r\n]*)$"
    )
    foreach ($m in $MatchesList) {
        $option = $m.Groups[1].Value
        $label = $m.Groups[2].Value
        if ($label -match $Needle1 -and (($Needle2 -eq "") -or ($label -match $Needle2))) {
            return $option
        }
    }
    Write-Host ""
    Write-Host "Available $Menu options:"
    $MatchesList | ForEach-Object { Write-Host $_.Value }
    throw "Could not find menu option for $Menu matching '$Needle1' '$Needle2'"
}

function Keep-OnlyMenuOption {
    param([string]$Text, [string]$Menu, [string]$Option)
    $Lines = $Text -split "`r?`n"
    $Kept = New-Object System.Collections.Generic.List[string]
    foreach ($line in $Lines) {
        if ($line -match ("^" + [regex]::Escape($BoardId) + "\.menu\." + [regex]::Escape($Menu) + "\.([^.=\r\n]+)(\.|=)")) {
            if ($Matches[1] -eq $Option) {
                $Kept.Add($line)
            }
            continue
        }
        $Kept.Add($line)
    }
    return ($Kept -join "`r`n").TrimEnd()
}

$FlashOption = Find-MenuOption $Block "FlashSize" "16" "MB|128"
$PsramOption = Find-MenuOption $Block "PSRAM" "OPI"
$PartitionOption = Find-MenuOption $Block "PartitionScheme" "16M" "3MB"

Write-Host "Detected menu options:"
Write-Host "  FlashSize       = $FlashOption"
Write-Host "  PSRAM           = $PsramOption"
Write-Host "  PartitionScheme = $PartitionOption"
Write-Host ""

$Block = Keep-OnlyMenuOption $Block "FlashSize" $FlashOption
$Block = Keep-OnlyMenuOption $Block "PSRAM" $PsramOption
$Block = Keep-OnlyMenuOption $Block "PartitionScheme" $PartitionOption

$Block = Set-BoardLine $Block "$BoardId.name" "ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)"
$Block = Set-BoardLine $Block "$BoardId.build.variant" "esp32_8048s043_lab"
$Block = Set-BoardLine $Block "$BoardId.build.board" "ESP32_8048S043_LAB"
$Block = Set-BoardLine $Block "$BoardId.build.mcu" "esp32s3"
$Block = Set-BoardLine $Block "$BoardId.build.target" "esp32s3"
$Block = Set-BoardLine $Block "$BoardId.build.core" "esp32"

$Block = Set-BoardLine $Block "$BoardId.build.flash_size" "16MB"
$Block = Set-BoardLine $Block "$BoardId.upload.flash_size" "16MB"
$Block = Set-BoardLine $Block "$BoardId.upload.maximum_size" "3145728"
$Block = Set-BoardLine $Block "$BoardId.build.psram_type" "opi"
$Block = Set-BoardLine $Block "$BoardId.build.partitions" "default_16MB"

$ExtraKey = "$BoardId.build.extra_flags"
$Block = Add-BoardFlag $Block $ExtraKey "-DBOARD_HAS_PSRAM"
$Block = Add-BoardFlag $Block $ExtraKey "-DARDUINO_ESP32_8048S043_LAB"
$Block = Add-BoardFlag $Block $ExtraKey "-DESP32_8048S043_HAS_RGB_PANEL=1"
$Block = Add-BoardFlag $Block $ExtraKey "-DESP32_8048S043_HAS_GT911=1"
$Block = Add-BoardFlag $Block $ExtraKey "-DESP32_8048S043_LCD_WIDTH=800"
$Block = Add-BoardFlag $Block $ExtraKey "-DESP32_8048S043_LCD_HEIGHT=480"
$Block = Add-BoardFlag $Block $ExtraKey "-DESP32_8048S043_TOUCH_GT911=1"

$NewBoards = $Header.TrimEnd() + "`r`n`r`n" + $Block.TrimEnd() + "`r`n"

$Utf8NoBom = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllText($BoardsFile, $NewBoards, $Utf8NoBom)

Write-Host "Rebuilt local boards.txt:"
Write-Host $BoardsFile
Write-Host ""
Write-Host "Board names now:"
Select-String -Path $BoardsFile -Pattern "^\S+\.name=" | Select-Object Line
Write-Host ""
Write-Host "DONE."
```

## Step 4: create platform.local.txt for Arduino_GFX RGB classes

The RGB classes in `Arduino_GFX_Library` need ESP32-S3 target macros during compilation of both the sketch and the library `.cpp` files.

Without this local override, two different failures were observed:

```text
Arduino_ESP32RGBPanel does not name a type
undefined reference to Arduino_ESP32RGBPanel::Arduino_ESP32RGBPanel(...)
```

Create `platform.local.txt` next to `platform.txt`:

```powershell
$ErrorActionPreference = "Stop"

$Platform = "$HOME\Documents\Arduino\hardware\AIDevelopersMonster\esp32"
$PlatformTxt = Join-Path $Platform "platform.txt"
$PlatformLocal = Join-Path $Platform "platform.local.txt"

if (-not (Test-Path $PlatformTxt)) {
    throw "platform.txt not found: $PlatformTxt"
}

$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
if (Test-Path $PlatformLocal) {
    Copy-Item $PlatformLocal "$PlatformLocal.bak-$Stamp" -Force
}

$Flags = "-DESP32 -DARDUINO_ARCH_ESP32 -DCONFIG_IDF_TARGET_ESP32S3=1 -DBOARD_HAS_PSRAM -DARDUINO_ESP32_8048S043_LAB -DESP32_8048S043_HAS_RGB_PANEL=1 -DESP32_8048S043_HAS_GT911=1 -DESP32_8048S043_LCD_WIDTH=800 -DESP32_8048S043_LCD_HEIGHT=480 -DESP32_8048S043_TOUCH_GT911=1"

$Content = @"
# ESP32-8048S043 Lab local platform override.
# Purpose: make ESP32-S3 target macros visible to sketches and third-party library .cpp files.

compiler.cpp.extra_flags=$Flags
compiler.c.extra_flags=$Flags
compiler.S.extra_flags=$Flags
"@

$Utf8NoBom = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllText($PlatformLocal, $Content, $Utf8NoBom)

Write-Host "Written:"
Write-Host $PlatformLocal
Write-Host ""
Get-Content $PlatformLocal
```

Restart Arduino IDE completely after creating this file.

## Step 5: select the board in Arduino IDE

Choose:

```text
Tools -> Board -> AIDevelopersMonster esp32 -> ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
```

Expected menu state:

```text
Flash Size       : 16MB (128Mb)
Flash Mode       : QIO 80MHz
PSRAM            : OPI PSRAM
Partition Scheme : 16M Flash (3MB APP/9.9MB FATFS)
Upload Mode      : UART0 / Hardware CDC
Upload Speed     : 921600, fallback 460800
Serial Monitor   : 115200 baud
```

## Step 6: validation order

Run examples from the Arduino IDE examples menu, reopening them after board/platform changes:

```text
File -> Examples -> ESP32_8048S043 -> 01_BoardInfo
File -> Examples -> ESP32_8048S043 -> 02_DisplayRGBTest
File -> Examples -> ESP32_8048S043 -> 03_TouchGT911Test
File -> Examples -> ESP32_8048S043 -> 04_BacklightTest
File -> Examples -> ESP32_8048S043 -> 05_TestConsole
```

Acceptance checkpoints:

```text
01_BoardInfo:
  ESP32-S3 rev 2
  Flash chip size 16777216 bytes / 16 MB
  PSRAM size 8388608 bytes / 8 MB
  Running app size 3145728 bytes
  ALIVE lines continue

02_DisplayRGBTest:
  Display begin: OK
  RED / GREEN / BLUE / WHITE / BLACK screens
  landscape 800x480 orientation frame
  RGB color bars
  stripe pattern

03_TouchGT911Test:
  gfx->begin(): OK
  GT911 at 0x5D or 0x14
  Product ID text: 911
  touch raw and screen coordinates change
  visible marker follows touch

04_BacklightTest:
  backlight OFF/ON blink is visible
  PWM duty steps are observed
  no brownout or crash

05_TestConsole:
  diagnostic console is visible
  GT911 is detected
  touch events increment
  marker follows touch
  BACKLIGHT / CLEAR / REPORT buttons work
```

## Troubleshooting

### Boot loop before sketch starts

Symptom:

```text
partition invalid - offset ... exceeds flash chip size 0x400000
Failed to verify partition table
```

Cause:

```text
Flash Size is still 4MB while the selected partition profile is 16MB.
```

Fix:

```text
Select Flash Size: 16MB (128Mb), or rebuild local boards.txt from the esp32s3 block.
```

### Arduino_ESP32RGBPanel does not name a type

Cause:

```text
Arduino_GFX headers do not see CONFIG_IDF_TARGET_ESP32S3.
```

Fix:

```text
Create platform.local.txt with ESP32-S3 target macros and restart Arduino IDE.
```

### Undefined reference to Arduino_ESP32RGBPanel

Cause:

```text
The sketch saw the target macros, but Arduino_GFX_Library .cpp files did not.
```

Fix:

```text
Use platform.local.txt so macros are visible to third-party library compilation units too.
```

### PSRAM reports 0 bytes

Check first with `01_BoardInfo`.

Expected acceptance line:

```text
PSRAM size              : 8388608 bytes / 8192 KB / 8 MB
```

If `05_TestConsole` reports `PSRAM: 0 bytes` while `01_BoardInfo` reports 8 MB, treat `01_BoardInfo` as the current PSRAM authority and keep the 05 result limited to RGB + GT911 + backlight integration until the console memory report is rechecked.

### Arduino IDE board list becomes confusing

Rollback:

```powershell
Remove-Item "$HOME\Documents\Arduino\hardware\AIDevelopersMonster\esp32" -Recurse -Force
```

Restart Arduino IDE. The installed Espressif core remains available.

## Why this is not yet a Board Manager package

A public Board Manager package needs a package index, archive, versioning and compatibility testing across Arduino-ESP32 core versions.

This local profile is a validated development step. It proves the board definition locally without changing the installed Espressif platform.

## Evidence

Primary local board-profile evidence:

```text
evidence/specimens/sample-a/arduino/01-boardinfo-local-board-profile-20260825.md
evidence/specimens/sample-a/arduino/local-board-profile-01-05-validation-20260825.md
```

Related project files:

```text
boards/arduino-ide/esp32-8048s043-lab/README.md
docs/arduino-board-profile.md
libraries/ESP32_8048S043/README.md
```
