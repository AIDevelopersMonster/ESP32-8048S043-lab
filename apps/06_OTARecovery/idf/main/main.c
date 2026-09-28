#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display_ota.h"
#include "ble_service.h"
#include "network_manager.h"
#include "modbus_service.h"
#include "ota_manager.h"
#include "sd_manager.h"
#include "serial_service.h"
#include "storage_credentials.h"
#include "storage_fs.h"
#include "time_service.h"
#include "web_setup.h"
#include "widget_runtime.h"
#include "youtube_service.h"

#define TAG "APP09"

void app_main(void)
{
    ESP_LOGI(TAG, "KONTAKTS platform shell + persistent network widget runtime + optional SD start");

    ESP_ERROR_CHECK(storage_credentials_init());
    ESP_ERROR_CHECK(storage_fs_init());
    ESP_ERROR_CHECK(widget_runtime_init());
    ESP_ERROR_CHECK(serial_service_init());
    ESP_ERROR_CHECK(modbus_service_init());
    ESP_ERROR_CHECK(ota_manager_init());

    /* SD is optional. Mount failure must never block boot/recovery/widget restore. */
    esp_err_t sd_err = sd_manager_init();
    if (sd_err != ESP_OK) {
        ESP_LOGW(TAG, "Optional SD unavailable: %s", esp_err_to_name(sd_err));
    }

    /* Network stack must exist before SNTP and network-data services start. */
    ESP_ERROR_CHECK(network_manager_init());
    ESP_ERROR_CHECK(time_service_init());
    ESP_ERROR_CHECK(youtube_service_init());

    /* Firmware-resident SYS/recovery shell remains independent from SD/widget files. */
    ESP_ERROR_CHECK(display_ota_start());

    /*
     * The RGB panel and LVGL need a sizeable contiguous block of internal DMA RAM.
     * Reserve that known-good display baseline before the optional BLE transport
     * starts allocating host/controller memory.
     */
    for (int i = 0; i < 300 && !display_ota_is_ready(); ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (display_ota_is_ready()) {
        esp_err_t ble_err = ble_service_init();
        if (ble_err != ESP_OK) {
            ESP_LOGW(TAG, "Optional BLE transport unavailable: %s", esp_err_to_name(ble_err));
        }
    } else {
        ESP_LOGW(TAG, "BLE transport deferred because display did not become ready");
    }

    ESP_ERROR_CHECK(web_setup_start());
    ESP_ERROR_CHECK(network_manager_begin());
    ESP_ERROR_CHECK(youtube_service_start());

    ota_status_t status;
    ota_manager_get_status(&status);
    widget_info_t widget;
    widget_runtime_get_info(&widget);
    sd_manager_status_t sd;
    sd_manager_get_status(&sd);
    ESP_LOGI(TAG,
             "PLATFORM:READY version=%s running=%s image_state=%s display=READY widget=%s youtube=READY sd=%s",
             status.current_version,
             status.running_partition,
             status.image_state,
             widget.installed ? widget.id : "none",
             sd.state);
}
