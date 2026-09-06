#include "esp_log.h"

#include "network_manager.h"
#include "ota_manager.h"
#include "storage_credentials.h"
#include "web_setup.h"

#define TAG "APP06"

void app_main(void)
{
    ESP_LOGI(TAG, "KONTAKTS App06 GitHub OTA / rollback / recovery start");

    ESP_ERROR_CHECK(storage_credentials_init());
    ESP_ERROR_CHECK(ota_manager_init());
    ESP_ERROR_CHECK(network_manager_init());
    ESP_ERROR_CHECK(web_setup_start());
    ESP_ERROR_CHECK(network_manager_begin());

    ota_status_t status;
    ota_manager_get_status(&status);
    ESP_LOGI(TAG,
             "APP06:OTA:READY version=%s running=%s image_state=%s",
             status.current_version,
             status.running_partition,
             status.image_state);
}
