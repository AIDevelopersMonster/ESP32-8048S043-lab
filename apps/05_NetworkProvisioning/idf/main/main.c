#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "nvs.h"

#define TAG "APP05"
#define NVS_NAMESPACE "platform"
#define NVS_KEY_SSID "wifi_ssid"
#define NVS_KEY_PASS "wifi_pass"
#define STA_CONNECT_TIMEOUT_MS 20000
#define STA_RETRY_LIMIT 5
#define AP_CHANNEL 1
#define AP_MAX_CONNECTIONS 4

#define BIT_STA_GOT_IP BIT0
#define BIT_STA_FAILED BIT1

typedef enum {
    NET_BOOT = 0,
    NET_STA_CONNECTING,
    NET_STA_ONLINE,
    NET_AP_SETUP,
} network_state_t;

static EventGroupHandle_t s_events;
static esp_netif_t *s_sta_netif;
static esp_netif_t *s_ap_netif;
static httpd_handle_t s_httpd;
static network_state_t s_state = NET_BOOT;
static int s_retry_count = 0;
static char s_sta_ip[16] = "0.0.0.0";
static char s_ap_ssid[33] = {0};

static const char *state_name(network_state_t state)
{
    switch (state) {
        case NET_STA_CONNECTING: return "STA_CONNECTING";
        case NET_STA_ONLINE: return "STA_ONLINE";
        case NET_AP_SETUP: return "AP_SETUP";
        default: return "BOOT";
    }
}

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static bool load_credentials(char *ssid, size_t ssid_len, char *password, size_t password_len)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
        return false;
    }

    size_t s_len = ssid_len;
    size_t p_len = password_len;
    esp_err_t a = nvs_get_str(h, NVS_KEY_SSID, ssid, &s_len);
    esp_err_t b = nvs_get_str(h, NVS_KEY_PASS, password, &p_len);
    nvs_close(h);

    if (a != ESP_OK || b != ESP_OK || ssid[0] == '\0') {
        ssid[0] = '\0';
        password[0] = '\0';
        return false;
    }
    return true;
}

static esp_err_t save_credentials(const char *ssid, const char *password)
{
    nvs_handle_t h;
    ESP_RETURN_ON_ERROR(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h), TAG, "nvs_open failed");
    esp_err_t err = nvs_set_str(h, NVS_KEY_SSID, ssid);
    if (err == ESP_OK) err = nvs_set_str(h, NVS_KEY_PASS, password);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

static esp_err_t clear_credentials(void)
{
    nvs_handle_t h;
    ESP_RETURN_ON_ERROR(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h), TAG, "nvs_open failed");
    esp_err_t err = nvs_erase_key(h, NVS_KEY_SSID);
    if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
    esp_err_t err2 = nvs_erase_key(h, NVS_KEY_PASS);
    if (err2 == ESP_ERR_NVS_NOT_FOUND) err2 = ESP_OK;
    if (err == ESP_OK && err2 == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err != ESP_OK ? err : err2;
}

static void build_ap_ssid(void)
{
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP));
    snprintf(s_ap_ssid, sizeof(s_ap_ssid), "KONTAKTS-%02X%02X%02X", mac[3], mac[4], mac[5]);
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_sta_ip[0] = '\0';
        strlcpy(s_sta_ip, "0.0.0.0", sizeof(s_sta_ip));
        if (s_state == NET_STA_CONNECTING && s_retry_count < STA_RETRY_LIMIT) {
            s_retry_count++;
            ESP_LOGW(TAG, "STA disconnected; retry %d/%d", s_retry_count, STA_RETRY_LIMIT);
            esp_wifi_connect();
        } else if (s_state == NET_STA_ONLINE) {
            s_state = NET_STA_CONNECTING;
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
        s_state = NET_STA_ONLINE;
        s_retry_count = 0;
        xEventGroupClearBits(s_events, BIT_STA_FAILED);
        xEventGroupSetBits(s_events, BIT_STA_GOT_IP);
        ESP_LOGI(TAG, "STA online ip=%s", s_sta_ip);
    }
}

static esp_err_t start_ap(void)
{
    build_ap_ssid();

    wifi_config_t cfg = {0};
    strlcpy((char *)cfg.ap.ssid, s_ap_ssid, sizeof(cfg.ap.ssid));
    cfg.ap.ssid_len = strlen(s_ap_ssid);
    cfg.ap.channel = AP_CHANNEL;
    cfg.ap.max_connection = AP_MAX_CONNECTIONS;
    cfg.ap.authmode = WIFI_AUTH_OPEN;

    wifi_mode_t mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
    if (mode == WIFI_MODE_STA) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    } else if (mode != WIFI_MODE_AP && mode != WIFI_MODE_APSTA) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &cfg));
    s_state = NET_AP_SETUP;
    ESP_LOGI(TAG, "AP setup active ssid=%s", s_ap_ssid);
    ESP_LOGI(TAG, "Open http://192.168.4.1/");
    return ESP_OK;
}

static esp_err_t connect_sta(const char *ssid, const char *password)
{
    wifi_config_t cfg = {0};
    strlcpy((char *)cfg.sta.ssid, ssid, sizeof(cfg.sta.ssid));
    strlcpy((char *)cfg.sta.password, password, sizeof(cfg.sta.password));
    cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
    xEventGroupClearBits(s_events, BIT_STA_GOT_IP | BIT_STA_FAILED);
    s_retry_count = 0;
    s_state = NET_STA_CONNECTING;
    ESP_LOGI(TAG, "STA connecting ssid=%s", ssid);
    return esp_wifi_connect();
}

static void url_decode(char *dst, size_t dst_len, const char *src)
{
    size_t di = 0;
    for (size_t i = 0; src[i] && di + 1 < dst_len; i++) {
        if (src[i] == '+') {
            dst[di++] = ' ';
        } else if (src[i] == '%' && src[i + 1] && src[i + 2]) {
            char h[3] = {src[i + 1], src[i + 2], 0};
            dst[di++] = (char)strtol(h, NULL, 16);
            i += 2;
        } else {
            dst[di++] = src[i];
        }
    }
    dst[di] = '\0';
}

static bool form_value(const char *body, const char *key, char *out, size_t out_len)
{
    char needle[40];
    snprintf(needle, sizeof(needle), "%s=", key);
    const char *p = strstr(body, needle);
    if (!p) return false;
    p += strlen(needle);
    const char *end = strchr(p, '&');
    size_t n = end ? (size_t)(end - p) : strlen(p);
    char encoded[128];
    if (n >= sizeof(encoded)) n = sizeof(encoded) - 1;
    memcpy(encoded, p, n);
    encoded[n] = '\0';
    url_decode(out, out_len, encoded);
    return true;
}

static esp_err_t root_get(httpd_req_t *req)
{
    char html[2300];
    int n = snprintf(html, sizeof(html),
        "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>KONTAKTS Setup</title><style>body{font-family:sans-serif;max-width:720px;margin:30px auto;padding:0 18px;background:#101418;color:#eef}input,select,button{font-size:18px;padding:10px;margin:6px 0;width:100%%;box-sizing:border-box}button{cursor:pointer}.card{background:#182027;padding:20px;border-radius:18px;margin:14px 0}</style></head><body>"
        "<h1>KONTAKTS Network Setup</h1><div class='card'><b>State:</b> %s<br><b>STA IP:</b> %s<br><b>Setup AP:</b> %s</div>"
        "<div class='card'><form method='post' action='/save'><label>Wi-Fi network</label><select id='ssid' name='ssid'><option>Scanning...</option></select><label>Password</label><input type='password' name='password' autocomplete='current-password'><button type='submit'>Save and connect</button></form></div>"
        "<div class='card'><form method='post' action='/clear'><button type='submit'>Clear saved Wi-Fi and use setup AP</button></form></div>"
        "<script>fetch('/scan').then(r=>r.json()).then(a=>{let s=document.getElementById('ssid');s.innerHTML='';a.forEach(x=>{let o=document.createElement('option');o.value=x.ssid;o.textContent=x.ssid+' ('+x.rssi+' dBm)';s.appendChild(o)});if(!a.length)s.innerHTML='<option value=\"\">No networks found</option>'}).catch(()=>{});</script>"
        "</body></html>", state_name(s_state), s_sta_ip, s_ap_ssid);
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, n);
}

static esp_err_t status_get(httpd_req_t *req)
{
    char json[160];
    int n = snprintf(json, sizeof(json), "{\"state\":\"%s\",\"ip\":\"%s\",\"ap\":\"%s\"}", state_name(s_state), s_sta_ip, s_ap_ssid);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, n);
}

static esp_err_t scan_get(httpd_req_t *req)
{
    uint16_t count = 0;
    wifi_scan_config_t scan_cfg = {0};
    esp_err_t err = esp_wifi_scan_start(&scan_cfg, true);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "scan failed");
        return err;
    }
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&count));
    if (count > 20) count = 20;
    wifi_ap_record_t *records = calloc(count ? count : 1, sizeof(wifi_ap_record_t));
    if (!records) return ESP_ERR_NO_MEM;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&count, records));

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "[");
    for (uint16_t i = 0; i < count; i++) {
        char item[140];
        snprintf(item, sizeof(item), "%s{\"ssid\":\"%s\",\"rssi\":%d}", i ? "," : "", (char *)records[i].ssid, records[i].rssi);
        httpd_resp_sendstr_chunk(req, item);
    }
    httpd_resp_sendstr_chunk(req, "]");
    httpd_resp_sendstr_chunk(req, NULL);
    free(records);
    return ESP_OK;
}

static esp_err_t save_post(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len >= 256) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
        return ESP_FAIL;
    }
    char body[256];
    int got = httpd_req_recv(req, body, req->content_len);
    if (got <= 0) return ESP_FAIL;
    body[got] = '\0';

    char ssid[33] = {0};
    char password[65] = {0};
    if (!form_value(body, "ssid", ssid, sizeof(ssid)) || ssid[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
        return ESP_FAIL;
    }
    form_value(body, "password", password, sizeof(password));

    ESP_RETURN_ON_ERROR(save_credentials(ssid, password), TAG, "save credentials failed");
    ESP_LOGI(TAG, "Credentials saved for ssid=%s (password hidden)", ssid);
    ESP_ERROR_CHECK(connect_sta(ssid, password));

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_sendstr(req, "Saved. Connecting...");
}

static esp_err_t clear_post(httpd_req_t *req)
{
    ESP_RETURN_ON_ERROR(clear_credentials(), TAG, "clear credentials failed");
    ESP_LOGI(TAG, "Saved Wi-Fi credentials cleared");
    esp_wifi_disconnect();
    ESP_ERROR_CHECK(start_ap());
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_sendstr(req, "Cleared");
}

static void start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    ESP_ERROR_CHECK(httpd_start(&s_httpd, &config));

    const httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get};
    const httpd_uri_t status = {.uri = "/status", .method = HTTP_GET, .handler = status_get};
    const httpd_uri_t scan = {.uri = "/scan", .method = HTTP_GET, .handler = scan_get};
    const httpd_uri_t save = {.uri = "/save", .method = HTTP_POST, .handler = save_post};
    const httpd_uri_t clear = {.uri = "/clear", .method = HTTP_POST, .handler = clear_post};
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &status));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &scan));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &save));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &clear));
    ESP_LOGI(TAG, "HTTP setup server started");
}

static void initial_network_task(void *arg)
{
    char ssid[33] = {0};
    char password[65] = {0};

    if (!load_credentials(ssid, sizeof(ssid), password, sizeof(password))) {
        ESP_LOGI(TAG, "No saved Wi-Fi credentials");
        ESP_ERROR_CHECK(start_ap());
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Saved Wi-Fi credentials found for ssid=%s (password hidden)", ssid);
    ESP_ERROR_CHECK(connect_sta(ssid, password));

    EventBits_t bits = xEventGroupWaitBits(s_events, BIT_STA_GOT_IP | BIT_STA_FAILED, pdTRUE, pdFALSE, pdMS_TO_TICKS(STA_CONNECT_TIMEOUT_MS));
    if (!(bits & BIT_STA_GOT_IP)) {
        ESP_LOGW(TAG, "Saved network unavailable within bounded window; entering AP setup");
        ESP_ERROR_CHECK(start_ap());
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "KONTAKTS App05 network provisioning start");
    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    s_events = xEventGroupCreate();
    if (!s_events) abort();

    s_sta_netif = esp_netif_create_default_wifi_sta();
    s_ap_netif = esp_netif_create_default_wifi_ap();
    (void)s_sta_netif;
    (void)s_ap_netif;

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_start());

    build_ap_ssid();
    start_http_server();
    xTaskCreate(initial_network_task, "network_init", 6144, NULL, 5, NULL);

    ESP_LOGI(TAG, "APP05:NETWORK:STARTED");
}
