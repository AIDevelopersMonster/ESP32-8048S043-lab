#include "esp_log.h"

#include "display_ota.h"
#include "network_manager.h"
#include "ota_manager.h"
#include "storage_credentials.h"
#include "storage_fs.h"
#include "time_service.h"
#include "web_setup.h"
#include "widget_runtime.h"
#include "youtube_service.h"

#define TAG "APP08"

void app_main(void)
{
    ESP_LOGI(TAG, "KONTAKTS platform shell + persistent network widget runtime start");

    ESP_ERROR_CHECK(storage_credentials_init());
    ESP_ERROR_CHECK(storage_fs_init());
    ESP_ERROR_CHECK(widget_runtime_init());
    ESP_ERROR_CHECK(ota_manager_init());

    /* Network stack must exist before SNTP and network-data services start. */
    ESP_ERROR_CHECK(network_manager_init());
    ESP_ERROR_CHECK(time_service_init());
    ESP_ERROR_CHECK(youtube_service_init());

    /* Firmware-resident recovery/status/OTA shell remains independent from widget files. */
    ESP_ERROR_CHECK(display_ota_start());

    ESP_ERROR_CHECK(web_setup_start());
    ESP_ERROR_CHECK(network_manager_begin());
    ESP_ERROR_CHECK(youtube_service_start());

    ota_status_t status;
    ota_manager_get_status(&status);
    widget_info_t widget;
    widget_runtime_get_info(&widget);
    ESP_LOGI(TAG,
             "PLATFORM:READY version=%s running=%s image_state=%s display=READY widget=%s youtube=READY",
             status.current_version,
             status.running_partition,
             status.image_state,
             widget.installed ? widget.id : "none");
}
