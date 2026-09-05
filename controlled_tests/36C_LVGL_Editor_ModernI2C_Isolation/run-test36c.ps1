param(
    [switch]$Upload,
    [string]$UploadPort = "",
    [string]$WorkRoot = "$HOME\t36c-modern-i2c",
    [string]$IdfExport = ""
)

$ErrorActionPreference = 'Stop'

$Commit = '79e862ca332525ba8721c4691f450fb44ec08738'
$Upstream = Join-Path $WorkRoot 'upstream'
$Prepare = Join-Path $PSScriptRoot 'prepare-test36c.ps1'

& powershell -ExecutionPolicy Bypass -File $Prepare -WorkRoot $WorkRoot

function Import-IdfEnvironment {
    param([string]$ExplicitExport)
    if ($ExplicitExport) {
        . $ExplicitExport
        return
    }
    if (Get-Command idf.py -ErrorAction SilentlyContinue) { return }
    $Candidates = @(
        "$HOME\esp\v5.5.5\esp-idf\export.ps1",
        "C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1"
    )
    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            . $Candidate
            return
        }
    }
    throw 'idf.py not available; pass -IdfExport'
}

function Show-LatestIdfFlashLogs {
    param([string]$BuildDir)
    $LogDir = Join-Path $BuildDir 'log'
    if (-not (Test-Path $LogDir)) { return }
    $Files = @(Get-ChildItem -Path $LogDir -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like 'idf_py_stderr_output_*' -or $_.Name -like 'idf_py_stdout_output_*' } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 4)
    if (-not $Files) { return }
    Write-Host ''
    Write-Host '========== ESP-IDF / esptool diagnostic tail =========='
    foreach ($File in $Files) {
        Write-Host "--- $($File.FullName) ---"
        Get-Content -Path $File.FullName -Tail 120 -ErrorAction SilentlyContinue
    }
    Write-Host '========================================================'
    Write-Host ''
}

function Write-Utf8NoBom {
    param([string]$Path, [string]$Text)
    [System.IO.File]::WriteAllText($Path, $Text, (New-Object System.Text.UTF8Encoding($false)))
}

Import-IdfEnvironment -ExplicitExport $IdfExport

Push-Location $Upstream
try {
    $Head = (git rev-parse HEAD).Trim()
    if ($Head -ne $Commit) { throw "Pinned commit mismatch: $Head" }

    $TouchHPath = Join-Path $Upstream 'main\touch_i2c.h'
    $TouchCPath = Join-Path $Upstream 'main\touch_i2c.c'
    $LcdCPath = Join-Path $Upstream 'main\lcd_display.c'
    $CmakePath = Join-Path $Upstream 'main\CMakeLists.txt'

    $TouchH = [System.IO.File]::ReadAllText($TouchHPath)
    $TouchC = [System.IO.File]::ReadAllText($TouchCPath)
    $LcdC = [System.IO.File]::ReadAllText($LcdCPath)
    $Cmake = [System.IO.File]::ReadAllText($CmakePath)

    if (-not $TouchH.Contains('#include <driver/i2c.h>')) { throw 'legacy touch_i2c.h baseline missing' }
    if (-not $TouchC.Contains('i2c_driver_install')) { throw 'legacy touch_i2c.c baseline missing' }
    if (-not $LcdC.Contains('(esp_lcd_i2c_bus_handle_t)1')) { throw 'legacy LCD I2C bus cast baseline missing' }
    if (-not $LcdC.Contains('#define LVGL_TASK_SLEEP 500')) { throw 'sleep=500 baseline missing' }

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

#define I2C_MASTER_FREQ_HZ  400000
#define I2C_MASTER_BUS      I2C_NUM_1
#define I2C_GPIO_SCL        GPIO_NUM_20
#define I2C_GPIO_SDA        GPIO_NUM_19
#define TAG_I2C "I2C DEV"

static i2c_master_bus_handle_t i2c_bus_handle = NULL;

esp_err_t i2c_dev_init(void)
{
    if (i2c_bus_handle != NULL) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_BUS,
        .sda_io_num = I2C_GPIO_SDA,
        .scl_io_num = I2C_GPIO_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };

    esp_err_t err = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_I2C, "Failed to install modern I2C master bus: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG_I2C, "Modern I2C master bus installed: port=%d SDA=%d SCL=%d", I2C_MASTER_BUS, I2C_GPIO_SDA, I2C_GPIO_SCL);
    return ESP_OK;
}

i2c_master_bus_handle_t i2c_dev_get_bus(void)
{
    return i2c_bus_handle;
}
'@

    Write-Utf8NoBom -Path $TouchHPath -Text $NewTouchH
    Write-Utf8NoBom -Path $TouchCPath -Text $NewTouchC

    $OldIoBlock = 'esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();'
    if (-not $LcdC.Contains($OldIoBlock)) { throw 'GT911 IO config line not found' }
    $LcdC = $LcdC.Replace($OldIoBlock, "esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();`r`n        tp_io_cfg.scl_speed_hz = 400000;")

    $OldCall = 'if (esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)1, &tp_io_cfg, tp_io) != ESP_OK)'
    if (-not $LcdC.Contains($OldCall)) { throw 'legacy esp_lcd_new_panel_io_i2c call not found' }
    $NewCall = 'if (esp_lcd_new_panel_io_i2c(i2c_dev_get_bus(), &tp_io_cfg, tp_io) != ESP_OK)'
    $LcdC = $LcdC.Replace($OldCall, $NewCall)

    if (-not $LcdC.Contains('#include "lcd_display.h"')) { throw 'lcd_display include anchor missing' }
    if (-not $LcdC.Contains('#include "touch_i2c.h"')) {
        $LcdC = $LcdC.Replace('#include "lcd_display.h"', "#include \"lcd_display.h\"`r`n#include \"touch_i2c.h\"")
    }
    Write-Utf8NoBom -Path $LcdCPath -Text $LcdC

    $OldReq = 'REQUIRES esp_lcd lvgl lvgl__lvgl)'
    if (-not $Cmake.Contains($OldReq)) { throw 'CMake REQUIRES baseline not found' }
    $Cmake = $Cmake.Replace($OldReq, 'REQUIRES esp_lcd esp_driver_i2c lvgl lvgl__lvgl)')
    Write-Utf8NoBom -Path $CmakePath -Text $Cmake

    $Diff = (git diff -- main/touch_i2c.h main/touch_i2c.c main/lcd_display.c main/CMakeLists.txt | Out-String)

    $MustContain = @(
        'driver/i2c_master.h',
        'i2c_new_master_bus',
        'glitch_ignore_cnt = 7',
        'enable_internal_pullup = true',
        'i2c_dev_get_bus()',
        'tp_io_cfg.scl_speed_hz = 400000',
        'esp_driver_i2c'
    )
    foreach ($Token in $MustContain) {
        if ($Diff -notmatch [regex]::Escape($Token)) { throw "Expected Test 36C diff token missing: $Token" }
    }
    if ($Diff -match 'LVGL_TASK_SLEEP 16') { throw 'Test 36B sleep delta leaked into Test 36C' }
    if (-not ([System.IO.File]::ReadAllText($LcdCPath)).Contains('#define LVGL_TASK_SLEEP 500')) {
        throw 'Test 36C failed to preserve sleep=500'
    }

    Write-Host '[PASS] Applied Test 36C modern-I2C transport delta'
    Write-Host '[PASS] Preserved sleep=500, GT911 config/mapping, display path and 400 kHz device speed'

    idf.py build
    if ($LASTEXITCODE -ne 0) { throw 'idf.py build failed' }
    Write-Host '[PASS] Test 36C build complete'

    if ($Upload) {
        if (-not $UploadPort) { throw '-Upload requires -UploadPort' }
        $Ports = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
        Write-Host "Detected serial ports: $($Ports -join ', ')"
        if ($Ports -notcontains $UploadPort) {
            throw "Requested upload port $UploadPort is not currently present. Available: $($Ports -join ', ')"
        }

        idf.py -p $UploadPort flash
        if ($LASTEXITCODE -ne 0) {
            Show-LatestIdfFlashLogs -BuildDir (Join-Path $Upstream 'build')
            throw "idf.py flash failed on $UploadPort"
        }
        Write-Host "[PASS] Test 36C flashed to $UploadPort"
    }
}
finally {
    git checkout -- . 2>$null
    $Status = (git status --porcelain --untracked-files=no | Out-String).Trim()
    if ($Status) {
        Write-Warning "Tracked upstream tree is not clean after full restore: $Status"
    } else {
        Write-Host '[PASS] Upstream tracked source fully restored after Test 36C'
    }
    Pop-Location
}
