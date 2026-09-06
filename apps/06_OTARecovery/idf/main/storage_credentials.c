#include "storage_credentials.h"

#include "nvs.h"
#include "nvs_flash.h"

#define NVS_NAMESPACE "platform"
#define NVS_KEY_SSID "wifi_ssid"
#define NVS_KEY_PASS "wifi_pass"

esp_err_t storage_credentials_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

bool storage_credentials_load(
    char *ssid,
    size_t ssid_len,
    char *password,
    size_t password_len
)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }

    size_t stored_ssid_len = ssid_len;
    size_t stored_password_len = password_len;
    esp_err_t ssid_err = nvs_get_str(handle, NVS_KEY_SSID, ssid, &stored_ssid_len);
    esp_err_t pass_err = nvs_get_str(handle, NVS_KEY_PASS, password, &stored_password_len);
    nvs_close(handle);

    if (ssid_err != ESP_OK || pass_err != ESP_OK || ssid[0] == '\0') {
        ssid[0] = '\0';
        password[0] = '\0';
        return false;
    }

    return true;
}

esp_err_t storage_credentials_save(const char *ssid, const char *password)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(handle, NVS_KEY_SSID, ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, NVS_KEY_PASS, password);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);
    return err;
}

esp_err_t storage_credentials_clear(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    esp_err_t ssid_err = nvs_erase_key(handle, NVS_KEY_SSID);
    if (ssid_err == ESP_ERR_NVS_NOT_FOUND) {
        ssid_err = ESP_OK;
    }

    esp_err_t pass_err = nvs_erase_key(handle, NVS_KEY_PASS);
    if (pass_err == ESP_ERR_NVS_NOT_FOUND) {
        pass_err = ESP_OK;
    }

    if (ssid_err == ESP_OK && pass_err == ESP_OK) {
        err = nvs_commit(handle);
    } else {
        err = ssid_err != ESP_OK ? ssid_err : pass_err;
    }

    nvs_close(handle);
    return err;
}
