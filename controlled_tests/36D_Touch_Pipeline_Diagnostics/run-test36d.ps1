param(
    [switch]$Upload,
    [string]$UploadPort = "",
    [string]$WorkRoot = "$HOME\t36d-touch-pipeline",
    [string]$IdfExport = ""
)

$ErrorActionPreference = 'Stop'
$Commit = '79e862ca332525ba8721c4691f450fb44ec08738'
$Upstream = Join-Path $WorkRoot 'upstream'
$Prepare = Join-Path $PSScriptRoot 'prepare-test36d.ps1'

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
    if (-not $Files) { return }
    Write-Host '========== ESP-IDF diagnostic tail =========='
    foreach ($File in $Files) {
        Write-Host "--- $($File.FullName) ---"
        Get-Content -Path $File.FullName -Tail 120 -ErrorAction SilentlyContinue
    }
    Write-Host '============================================='
}

Import-IdfEnvironment -ExplicitExport $IdfExport

Push-Location $Upstream
try {
    if ((git rev-parse HEAD).Trim() -ne $Commit) { throw 'Pinned commit mismatch' }

    $TouchHPath = Join-Path $Upstream 'main\touch_i2c.h'
    $TouchCPath = Join-Path $Upstream 'main\touch_i2c.c'
    $LcdCPath = Join-Path $Upstream 'main\lcd_display.c'
    $CmakePath = Join-Path $Upstream 'main\CMakeLists.txt'
    $ExamplesPath = Join-Path $Upstream 'main\examples\examples.c'
    $GenPath = Join-Path $Upstream 'main\examples\screens\stream_deck_main_gen.c'

    # Reproduce Test 36C modern-I2C baseline first.
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
    if (err != ESP_OK) {
        ESP_LOGE(TAG_I2C, "Failed to install modern I2C master bus: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG_I2C, "Modern I2C master bus installed: port=%d SDA=%d SCL=%d", I2C_MASTER_BUS, I2C_GPIO_SDA, I2C_GPIO_SCL);
    return ESP_OK;
}
i2c_master_bus_handle_t i2c_dev_get_bus(void) { return i2c_bus_handle; }
'@
    Write-Utf8NoBom $TouchHPath $NewTouchH
    Write-Utf8NoBom $TouchCPath $NewTouchC

    $LcdC = [System.IO.File]::ReadAllText($LcdCPath)
    $LcdC = $LcdC.Replace('#include "lcd_display.h"', "#include \"lcd_display.h\"`r`n#include \"touch_i2c.h\"")
    $LcdC = $LcdC.Replace('esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();', "esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();`r`n        tp_io_cfg.scl_speed_hz = 400000;")
    $LcdC = $LcdC.Replace('if (esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)1, &tp_io_cfg, tp_io) != ESP_OK)', 'if (esp_lcd_new_panel_io_i2c(i2c_dev_get_bus(), &tp_io_cfg, tp_io) != ESP_OK)')
    Write-Utf8NoBom $LcdCPath $LcdC

    $Cmake = [System.IO.File]::ReadAllText($CmakePath)
    $Cmake = $Cmake.Replace('REQUIRES esp_lcd lvgl lvgl__lvgl)', 'REQUIRES esp_lcd esp_driver_i2c lvgl lvgl__lvgl)')
    Write-Utf8NoBom $CmakePath $Cmake

    # Resolve managed components so esp_lvgl_port 2.6.0 source exists locally.
    idf.py reconfigure
    if ($LASTEXITCODE -ne 0) { throw 'idf.py reconfigure failed' }

    $PortTouchPath = Join-Path $Upstream 'managed_components\espressif__esp_lvgl_port\src\lvgl9\esp_lvgl_port_touch.c'
    if (-not (Test-Path $PortTouchPath)) { throw "esp_lvgl_port touch source not found: $PortTouchPath" }
    $PortTouch = [System.IO.File]::ReadAllText($PortTouchPath)

    $Needle = 'bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_ctx->handle, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);'
    if (-not $PortTouch.Contains($Needle)) { throw 'esp_lvgl_port 2.6.0 touch read anchor not found' }
    $Diag = @'
bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_ctx->handle, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    static bool diag_prev_pressed = false;
    bool diag_pressed = touchpad_pressed && touchpad_cnt > 0;
    if (diag_pressed && !diag_prev_pressed) {
        ESP_LOGI("T36D", "RAW PRESS x=%u y=%u cnt=%u", touchpad_x[0], touchpad_y[0], touchpad_cnt);
    } else if (!diag_pressed && diag_prev_pressed) {
        ESP_LOGI("T36D", "RAW RELEASE");
    }
    diag_prev_pressed = diag_pressed;
'@
    $PortTouch = $PortTouch.Replace($Needle, $Diag)
    Write-Utf8NoBom $PortTouchPath $PortTouch

    # Ask the existing button callback to receive all events, then log the relevant transition classes.
    $Gen = [System.IO.File]::ReadAllText($GenPath)
    if (($Gen -split 'LV_EVENT_CLICKED').Count -lt 7) { throw 'Expected six generated CLICKED registrations not found' }
    $Gen = $Gen.Replace('button_cb, LV_EVENT_CLICKED, NULL', 'button_cb, LV_EVENT_ALL, NULL')
    Write-Utf8NoBom $GenPath $Gen

    $Examples = [System.IO.File]::ReadAllText($ExamplesPath)
    $OldCb = @'
void button_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *deck_btn = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED)
    {
        lv_obj_t *btn_label = lv_obj_get_child(deck_btn, 1);
        ESP_LOGI(demo_tag,"Button Name : %s", lv_label_get_text(btn_label));
    }
}
'@
    if (-not $Examples.Contains($OldCb)) { throw 'button_cb baseline anchor not found' }
    $NewCb = @'
void button_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *deck_btn = lv_event_get_target(e);
    lv_obj_t *btn_label = lv_obj_get_child(deck_btn, 1);
    const char *name = lv_label_get_text(btn_label);

    switch (code) {
        case LV_EVENT_PRESSED:
            ESP_LOGI("T36D", "LVGL PRESSED %s", name);
            break;
        case LV_EVENT_RELEASED:
            ESP_LOGI("T36D", "LVGL RELEASED %s", name);
            break;
        case LV_EVENT_PRESS_LOST:
            ESP_LOGI("T36D", "LVGL PRESS_LOST %s", name);
            break;
        case LV_EVENT_CLICKED:
            ESP_LOGI("T36D", "LVGL CLICKED %s", name);
            break;
        default:
            break;
    }
}
'@
    $Examples = $Examples.Replace($OldCb, $NewCb)
    Write-Utf8NoBom $ExamplesPath $Examples

    if (-not ([System.IO.File]::ReadAllText($LcdCPath)).Contains('#define LVGL_TASK_SLEEP 500')) { throw 'sleep=500 was not preserved' }
    Write-Host '[PASS] Test 36D diagnostics installed on Test 36C modern-I2C baseline'
    Write-Host '[PASS] Logging RAW PRESS/RELEASE + LVGL PRESSED/RELEASED/PRESS_LOST/CLICKED'

    idf.py build
    if ($LASTEXITCODE -ne 0) { throw 'idf.py build failed' }
    Write-Host '[PASS] Test 36D build complete'

    if ($Upload) {
        if (-not $UploadPort) { throw '-Upload requires -UploadPort' }
        $Ports = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
        if ($Ports -notcontains $UploadPort) { throw "Requested port $UploadPort not present. Available: $($Ports -join ', ')" }
        idf.py -p $UploadPort flash
        if ($LASTEXITCODE -ne 0) {
            Show-LatestIdfFlashLogs -BuildDir (Join-Path $Upstream 'build')
            throw "idf.py flash failed on $UploadPort"
        }
        Write-Host "[PASS] Test 36D flashed to $UploadPort"
    }
}
finally {
    git checkout -- . 2>$null
    $Managed = Join-Path $Upstream 'managed_components\espressif__esp_lvgl_port\src\lvgl9\esp_lvgl_port_touch.c'
    if (Test-Path $Managed) {
        # Managed component is outside upstream tracked source; restore it on next reconfigure/prepare.
        Remove-Item -Recurse -Force (Join-Path $Upstream 'managed_components\espressif__esp_lvgl_port') -ErrorAction SilentlyContinue
    }
    $Status = (git status --porcelain --untracked-files=no | Out-String).Trim()
    if ($Status) { Write-Warning "Tracked upstream tree is not clean after restore: $Status" }
    else { Write-Host '[PASS] Upstream tracked source restored after Test 36D' }
    Pop-Location
}
