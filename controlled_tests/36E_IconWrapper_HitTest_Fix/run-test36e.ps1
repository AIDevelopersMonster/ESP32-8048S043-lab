param(
    [switch]$Upload,
    [string]$UploadPort = "",
    [string]$WorkRoot = "$HOME\t36e-icon-wrapper-hit-test",
    [string]$IdfExport = ""
)

$ErrorActionPreference = 'Stop'
$Commit = '79e862ca332525ba8721c4691f450fb44ec08738'
$Upstream = Join-Path $WorkRoot 'upstream'
$Prepare = Join-Path $PSScriptRoot 'prepare-test36e.ps1'

& powershell -ExecutionPolicy Bypass -File $Prepare -WorkRoot $WorkRoot

function Import-IdfEnvironment {
    param([string]$ExplicitExport)
    if ($ExplicitExport) { . $ExplicitExport; return }
    if (Get-Command idf.py -ErrorAction SilentlyContinue) { return }
    foreach ($Candidate in @(
        "$HOME\esp\v5.5.5\esp-idf\export.ps1",
        "C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1"
    )) {
        if (Test-Path $Candidate) { . $Candidate; return }
    }
    throw 'idf.py not available; pass -IdfExport'
}

function Write-Utf8NoBom {
    param([string]$Path, [string]$Text)
    [System.IO.File]::WriteAllText($Path, $Text, (New-Object System.Text.UTF8Encoding($false)))
}

function Show-LatestIdfFlashLogs {
    param([string]$BuildDir)
    $LogDir = Join-Path $BuildDir 'log'
    if (-not (Test-Path $LogDir)) { return }
    $Files = @(Get-ChildItem -Path $LogDir -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like 'idf_py_stderr_output_*' -or $_.Name -like 'idf_py_stdout_output_*' } |
        Sort-Object LastWriteTime -Descending | Select-Object -First 4)
    foreach ($File in $Files) {
        Write-Host "--- $($File.FullName) ---"
        Get-Content -Path $File.FullName -Tail 120 -ErrorAction SilentlyContinue
    }
}

Import-IdfEnvironment -ExplicitExport $IdfExport

Push-Location $Upstream
try {
    if ((git rev-parse HEAD).Trim() -ne $Commit) { throw 'Pinned commit mismatch' }

    $TouchHPath = Join-Path $Upstream 'main\touch_i2c.h'
    $TouchCPath = Join-Path $Upstream 'main\touch_i2c.c'
    $LcdCPath = Join-Path $Upstream 'main\lcd_display.c'
    $CmakePath = Join-Path $Upstream 'main\CMakeLists.txt'
    $DeckPath = Join-Path $Upstream 'main\examples\components\deck_btn_gen.c'

    # Reproduce the Test 36C modern-I2C runtime baseline.
    $NewTouchH = @'
#pragma once
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
esp_err_t i2c_dev_init(void);
i2c_master_bus_handle_t i2c_dev_get_bus(void);
'@

    $NewTouchC = @'
#include "touch_i2c.h"
#include "esp_log.h"
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_MASTER_BUS I2C_NUM_1
#define I2C_GPIO_SCL GPIO_NUM_20
#define I2C_GPIO_SDA GPIO_NUM_19
#define TAG_I2C "I2C DEV"
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
esp_err_t i2c_dev_init(void)
{
    if (i2c_bus_handle != NULL) return ESP_OK;
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_BUS,
        .sda_io_num = I2C_GPIO_SDA,
        .scl_io_num = I2C_GPIO_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = { .enable_internal_pullup = true },
    };
    esp_err_t err = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (err != ESP_OK) return err;
    ESP_LOGI(TAG_I2C, "Modern I2C master bus installed: port=%d SDA=%d SCL=%d", I2C_MASTER_BUS, I2C_GPIO_SDA, I2C_GPIO_SCL);
    return ESP_OK;
}
i2c_master_bus_handle_t i2c_dev_get_bus(void) { return i2c_bus_handle; }
'@

    Write-Utf8NoBom $TouchHPath $NewTouchH
    Write-Utf8NoBom $TouchCPath $NewTouchC

    $LcdC = [System.IO.File]::ReadAllText($LcdCPath)
    $OldInclude = '#include "lcd_display.h"'
    if (-not $LcdC.Contains('#include "touch_i2c.h"')) {
        $NewInclude = @'
#include "lcd_display.h"
#include "touch_i2c.h"
'@
        $LcdC = $LcdC.Replace($OldInclude, $NewInclude.TrimEnd())
    }
    $LcdC = $LcdC.Replace('esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();', "esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();`r`n        tp_io_cfg.scl_speed_hz = 400000;")
    $LcdC = $LcdC.Replace('if (esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)1, &tp_io_cfg, tp_io) != ESP_OK)', 'if (esp_lcd_new_panel_io_i2c(i2c_dev_get_bus(), &tp_io_cfg, tp_io) != ESP_OK)')
    Write-Utf8NoBom $LcdCPath $LcdC

    $Cmake = [System.IO.File]::ReadAllText($CmakePath)
    $Cmake = $Cmake.Replace('REQUIRES esp_lcd lvgl lvgl__lvgl)', 'REQUIRES esp_lcd esp_driver_i2c lvgl lvgl__lvgl)')
    Write-Utf8NoBom $CmakePath $Cmake

    # Test 36E single UI delta: make the generic 64x64 icon wrapper transparent to hit-testing.
    $Deck = [System.IO.File]::ReadAllText($DeckPath)
    $Anchor = 'lv_obj_t * lv_obj_0 = lv_obj_create(lv_button_0);'
    if (-not $Deck.Contains($Anchor)) { throw 'icon-wrapper anchor missing' }
    if ($Deck.Contains('lv_obj_remove_flag(lv_obj_0, LV_OBJ_FLAG_CLICKABLE);')) { throw 'fix already present unexpectedly' }
    $Deck = $Deck.Replace($Anchor, "$Anchor`r`n    lv_obj_remove_flag(lv_obj_0, LV_OBJ_FLAG_CLICKABLE);")
    Write-Utf8NoBom $DeckPath $Deck

    $Diff = (git diff -- main/examples/components/deck_btn_gen.c | Out-String)
    if ($Diff -notmatch [regex]::Escape('lv_obj_remove_flag(lv_obj_0, LV_OBJ_FLAG_CLICKABLE);')) {
        throw 'Test 36E interaction delta not present in diff'
    }
    if (-not ([System.IO.File]::ReadAllText($LcdCPath)).Contains('#define LVGL_TASK_SLEEP 500')) { throw 'sleep=500 not preserved' }

    Write-Host '[PASS] Test 36C modern-I2C baseline reproduced'
    Write-Host '[PASS] Applied ONLY Test 36E UI delta: icon-wrapper CLICKABLE removed'

    idf.py build
    if ($LASTEXITCODE -ne 0) { throw 'idf.py build failed' }
    Write-Host '[PASS] Test 36E build complete'

    if ($Upload) {
        if (-not $UploadPort) { throw '-Upload requires -UploadPort' }
        $Ports = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
        if ($Ports -notcontains $UploadPort) { throw "Requested port $UploadPort not present. Available: $($Ports -join ', ')" }
        idf.py -p $UploadPort flash
        if ($LASTEXITCODE -ne 0) {
            Show-LatestIdfFlashLogs -BuildDir (Join-Path $Upstream 'build')
            throw "idf.py flash failed on $UploadPort"
        }
        Write-Host "[PASS] Test 36E flashed to $UploadPort"
    }
}
finally {
    git checkout -- . 2>$null
    $Status = (git status --porcelain --untracked-files=no | Out-String).Trim()
    if ($Status) { Write-Warning "Tracked upstream tree is not clean after restore: $Status" }
    else { Write-Host '[PASS] Upstream tracked source fully restored after Test 36E' }
    Pop-Location
}
