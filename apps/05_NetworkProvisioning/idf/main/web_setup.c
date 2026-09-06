#include "web_setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "network_manager.h"
#include "storage_credentials.h"

#define TAG "APP05_WEB"

static httpd_handle_t s_httpd;

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
    if (!p) {
        return false;
    }

    p += strlen(needle);
    const char *end = strchr(p, '&');
    size_t n = end ? (size_t)(end - p) : strlen(p);
    char encoded[128];
    if (n >= sizeof(encoded)) {
        n = sizeof(encoded) - 1;
    }

    memcpy(encoded, p, n);
    encoded[n] = '\0';
    url_decode(out, out_len, encoded);
    return true;
}

static esp_err_t root_get(httpd_req_t *req)
{
    char html[2300];
    int n = snprintf(
        html,
        sizeof(html),
        "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>KONTAKTS Setup</title><style>body{font-family:sans-serif;max-width:720px;margin:30px auto;padding:0 18px;background:#101418;color:#eef}input,select,button{font-size:18px;padding:10px;margin:6px 0;width:100%%;box-sizing:border-box}button{cursor:pointer}.card{background:#182027;padding:20px;border-radius:18px;margin:14px 0}</style></head><body>"
        "<h1>KONTAKTS Network Setup</h1><div class='card'><b>State:</b> %s<br><b>STA IP:</b> %s<br><b>Setup AP:</b> %s</div>"
        "<div class='card'><form method='post' action='/save'><label>Wi-Fi network</label><select id='ssid' name='ssid'><option>Scanning...</option></select><label>Password</label><input type='password' name='password' autocomplete='current-password'><button type='submit'>Save and connect</button></form></div>"
        "<div class='card'><form method='post' action='/clear'><button type='submit'>Clear saved Wi-Fi and use setup AP</button></form></div>"
        "<script>fetch('/scan').then(r=>r.json()).then(a=>{let s=document.getElementById('ssid');s.innerHTML='';a.forEach(x=>{let o=document.createElement('option');o.value=x.ssid;o.textContent=x.ssid+' ('+x.rssi+' dBm)';s.appendChild(o)});if(!a.length)s.innerHTML='<option value=\"\">No networks found</option>'}).catch(()=>{});</script>"
        "</body></html>",
        network_manager_state_name(),
        network_manager_sta_ip(),
        network_manager_ap_ssid()
    );

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, n);
}

static esp_err_t favicon_get(httpd_req_t *req)
{
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_get(httpd_req_t *req)
{
    char json[192];
    int n = snprintf(
        json,
        sizeof(json),
        "{\"state\":\"%s\",\"ip\":\"%s\",\"ap\":\"%s\"}",
        network_manager_state_name(),
        network_manager_sta_ip(),
        network_manager_ap_ssid()
    );
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, n);
}

static esp_err_t scan_get(httpd_req_t *req)
{
    char *json = NULL;
    esp_err_t err = network_manager_scan_json(&json);
    if (err == ESP_ERR_INVALID_STATE) {
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_sendstr(req, "scan available only in AP setup mode");
        return err;
    }
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "scan failed");
        return err;
    }

    httpd_resp_set_type(req, "application/json");
    esp_err_t send_err = httpd_resp_sendstr(req, json);
    free(json);
    return send_err;
}

static esp_err_t save_post(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len >= 256) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
        return ESP_FAIL;
    }

    char body[256];
    int got = httpd_req_recv(req, body, req->content_len);
    if (got <= 0) {
        return ESP_FAIL;
    }
    body[got] = '\0';

    char ssid[33] = {0};
    char password[65] = {0};
    if (!form_value(body, "ssid", ssid, sizeof(ssid)) || ssid[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
        return ESP_FAIL;
    }
    form_value(body, "password", password, sizeof(password));

    ESP_RETURN_ON_ERROR(storage_credentials_save(ssid, password), TAG, "save credentials failed");
    ESP_LOGI(TAG, "Credentials saved for ssid=%s (password hidden)", ssid);
    ESP_RETURN_ON_ERROR(network_manager_connect(ssid, password), TAG, "STA connect failed");

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_sendstr(req, "Saved. Connecting...");
}

static esp_err_t clear_post(httpd_req_t *req)
{
    ESP_RETURN_ON_ERROR(storage_credentials_clear(), TAG, "clear credentials failed");
    ESP_LOGI(TAG, "Saved Wi-Fi credentials cleared");
    network_manager_disconnect_sta();
    ESP_RETURN_ON_ERROR(network_manager_enter_ap_setup(), TAG, "AP setup failed");

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_sendstr(req, "Cleared");
}

esp_err_t web_setup_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.stack_size = 8192;
    ESP_RETURN_ON_ERROR(httpd_start(&s_httpd, &config), TAG, "httpd_start failed");

    const httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get};
    const httpd_uri_t favicon = {.uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_get};
    const httpd_uri_t status = {.uri = "/status", .method = HTTP_GET, .handler = status_get};
    const httpd_uri_t scan = {.uri = "/scan", .method = HTTP_GET, .handler = scan_get};
    const httpd_uri_t save = {.uri = "/save", .method = HTTP_POST, .handler = save_post};
    const httpd_uri_t clear = {.uri = "/clear", .method = HTTP_POST, .handler = clear_post};

    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_httpd, &root), TAG, "root handler failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_httpd, &favicon), TAG, "favicon handler failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_httpd, &status), TAG, "status handler failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_httpd, &scan), TAG, "scan handler failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_httpd, &save), TAG, "save handler failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_httpd, &clear), TAG, "clear handler failed");

    ESP_LOGI(TAG, "HTTP setup server started");
    return ESP_OK;
}
