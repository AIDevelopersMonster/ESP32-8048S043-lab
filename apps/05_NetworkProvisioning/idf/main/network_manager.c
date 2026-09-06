#include "network_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "storage_credentials.h"

#define TAG "APP05_NET"
#define STA_CONNECT_TIMEOUT_MS 20000
#define STA_RETRY_LIMIT 5
#define AP_CHANNEL 1
#define AP_MAX_CONNECTIONS 4

#define BIT_STA_GOT_IP BIT0
#define BIT_STA_FAILED BIT1

static EventGroupHandle_t s_events;
static network_state_t s_state = NETWORK_STATE_BOOT;
static int s_retry_count;
static char s_sta_ip[16] = "0.0.0.0";
static char s_ap_ssid[33];

static void build_ap_ssid(void)
{
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP));
    snprintf(s_ap_ssid, sizeof(s_ap_ssid), "KONTAKTS-%02X%02X%02X", mac[3], mac[4], mac[5]);
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        strlcpy(s_sta_ip, "0.0.0.0", sizeof(s_sta_ip));

        if (s_state == NETWORK_STATE_STA_CONNECTING && s_retry_count < STA_RETRY_LIMIT) {
            s_retry_count++;
            ESP_LOGW(TAG, "STA disconnected; retry %d/%d", s_retry_count, STA_RETRY_LIMIT);
            esp_wifi_connect();
        } else if (s_state == NETWORK_STATE_STA_ONLINE) {
            s_state = NETWORK_STATE_STA_CONNECTING;
            s_retry_count = 1;
            ESP_LOGW(TAG, "STA link lost; reconnecting");
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_events, BIT_STA_FAILED);
        }
    }

    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        snprintf(s_sta_ip, sizeof(s_sta_ip), IPSTR, IP2STR(&event->ip_info.ip));
        s_state = NETWORK_STATE_STA_ONLINE;
        s_retry_count = 0;
        xEventGroupClearBits(s_events, BIT_STA_FAILED);
        xEventGroupSetBits(s_events, BIT_STA_GOT_IP);
        ESP_LOGI(TAG, "STA online ip=%s", s_sta_ip);
    }
}

static void initial_network_task(void *arg)
{
    char ssid[33] = {0};
    char password[65] = {0};

    if (!storage_credentials_load(ssid, sizeof(ssid), password, sizeof(password))) {
        ESP_LOGI(TAG, "No saved Wi-Fi credentials");
        ESP_ERROR_CHECK(network_manager_enter_ap_setup());
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Saved Wi-Fi credentials found for ssid=%s (password hidden)", ssid);
    ESP_ERROR_CHECK(network_manager_connect(ssid, password));

    EventBits_t bits = xEventGroupWaitBits(
        s_events,
        BIT_STA_GOT_IP | BIT_STA_FAILED,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(STA_CONNECT_TIMEOUT_MS)
    );

    if (!(bits & BIT_STA_GOT_IP)) {
        ESP_LOGW(TAG, "Saved network unavailable within bounded window; entering AP setup");
        ESP_ERROR_CHECK(network_manager_enter_ap_setup());
    }

    vTaskDelete(NULL);
}

esp_err_t network_manager_init(void)
{
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif_init failed");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "event loop failed");

    s_events = xEventGroupCreate();
    if (!s_events) {
        return ESP_ERR_NO_MEM;
    }

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_cfg), TAG, "esp_wifi_init failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL), TAG, "wifi handler failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL), TAG, "ip handler failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "wifi storage failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "wifi mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");

    build_ap_ssid();
    return ESP_OK;
}

esp_err_t network_manager_begin(void)
{
    BaseType_t ok = xTaskCreate(initial_network_task, "network_init", 6144, NULL, 5, NULL);
    return ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t network_manager_connect(const char *ssid, const char *password)
{
    wifi_config_t cfg = {0};
    strlcpy((char *)cfg.sta.ssid, ssid, sizeof(cfg.sta.ssid));
    strlcpy((char *)cfg.sta.password, password, sizeof(cfg.sta.password));
    cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "set APSTA failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &cfg), TAG, "set STA config failed");

    xEventGroupClearBits(s_events, BIT_STA_GOT_IP | BIT_STA_FAILED);
    s_retry_count = 0;
    s_state = NETWORK_STATE_STA_CONNECTING;
    ESP_LOGI(TAG, "STA connecting ssid=%s", ssid);
    return esp_wifi_connect();
}

esp_err_t network_manager_enter_ap_setup(void)
{
    wifi_config_t cfg = {0};
    strlcpy((char *)cfg.ap.ssid, s_ap_ssid, sizeof(cfg.ap.ssid));
    cfg.ap.ssid_len = strlen(s_ap_ssid);
    cfg.ap.channel = AP_CHANNEL;
    cfg.ap.max_connection = AP_MAX_CONNECTIONS;
    cfg.ap.authmode = WIFI_AUTH_OPEN;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "set APSTA failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &cfg), TAG, "set AP config failed");

    s_state = NETWORK_STATE_AP_SETUP;
    ESP_LOGI(TAG, "AP setup active ssid=%s", s_ap_ssid);
    ESP_LOGI(TAG, "Open http://192.168.4.1/");
    return ESP_OK;
}

esp_err_t network_manager_disconnect_sta(void)
{
    return esp_wifi_disconnect();
}

esp_err_t network_manager_scan_json(char **json_out)
{
    if (!json_out) {
        return ESP_ERR_INVALID_ARG;
    }
    *json_out = NULL;

    uint16_t count = 0;
    wifi_scan_config_t scan_cfg = {0};
    ESP_RETURN_ON_ERROR(esp_wifi_scan_start(&scan_cfg, true), TAG, "scan start failed");
    ESP_RETURN_ON_ERROR(esp_wifi_scan_get_ap_num(&count), TAG, "scan count failed");
    if (count > 20) {
        count = 20;
    }

    wifi_ap_record_t *records = calloc(count ? count : 1, sizeof(wifi_ap_record_t));
    if (!records) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_wifi_scan_get_ap_records(&count, records);
    if (err != ESP_OK) {
        free(records);
        return err;
    }

    size_t capacity = 32 + (size_t)count * 128;
    char *json = calloc(1, capacity);
    if (!json) {
        free(records);
        return ESP_ERR_NO_MEM;
    }

    size_t used = 0;
    used += snprintf(json + used, capacity - used, "[");
    for (uint16_t i = 0; i < count; i++) {
        used += snprintf(
            json + used,
            capacity - used,
            "%s{\"ssid\":\"%s\",\"rssi\":%d}",
            i ? "," : "",
            (char *)records[i].ssid,
            records[i].rssi
        );
    }
    snprintf(json + used, capacity - used, "]");

    free(records);
    *json_out = json;
    return ESP_OK;
}

network_state_t network_manager_state(void)
{
    return s_state;
}

const char *network_manager_state_name(void)
{
    switch (s_state) {
        case NETWORK_STATE_STA_CONNECTING: return "STA_CONNECTING";
        case NETWORK_STATE_STA_ONLINE: return "STA_ONLINE";
        case NETWORK_STATE_AP_SETUP: return "AP_SETUP";
        default: return "BOOT";
    }
}

const char *network_manager_sta_ip(void)
{
    return s_sta_ip;
}

const char *network_manager_ap_ssid(void)
{
    return s_ap_ssid;
}
