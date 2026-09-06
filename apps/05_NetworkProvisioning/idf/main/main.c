#include "esp_log.h"

#include "network_manager.h"
#include "storage_credentials.h"
#include "web_setup.h"

#define TAG "APP05"

void app_main(void)
{
    ESP_LOGI(TAG, "KONTAKTS App05 network provisioning start");

    ESP_ERROR_CHECK(storage_credentials_init());
    ESP_ERROR_CHECK(network_manager_init());
    ESP_ERROR_CHECK(web_setup_start());
    ESP_ERROR_CHECK(network_manager_begin());

    ESP_LOGI(TAG, "APP05:NETWORK:STARTED");
}
