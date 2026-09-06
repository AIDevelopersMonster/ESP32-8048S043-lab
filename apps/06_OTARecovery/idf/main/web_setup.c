#include "web_setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "network_manager.h"
#include "ota_manager.h"
#include "storage_credentials.h"

#define TAG "APP06_WEB"

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
    ota_status_t ota;
    ota_manager_get_status(&ota);

    const bool setup = network_manager_state() == NETWORK_STATE_AP_SETUP;
    const char *network_form = setup
        ? "<div class='card'><h2>Wi-Fi setup</h2><form method='post' action='/save'><label>Network</label><select id='ssid' name='ssid'><option>Scanning...</option></select><label>Password</label><input type='password' name='password'><button>Save and connect</button></form></div>"
        : "";
    const char *scan_script = setup
        ? "<script>fetch('/scan').then(r=>r.json()).then(a=>{let s=document.getElementById('ssid');s.innerHTML='';a.forEach(x=>{let o=document.createElement('option');o.value=x.ssid;o.textContent=x.ssid+' ('+x.rssi+' dBm)';s.appendChild(o)});if(!a.length)s.innerHTML='<option value=\"\">No networks found</option>'}).catch(()=>{});</script>"
        : "";

    char *html = calloc(1, 6000);
    if (!html) return ESP_ERR_NO_MEM;

    int n = snprintf(
        html, 6000,
        "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>KONTAKTS App06</title><style>body{font-family:sans-serif;max-width:760px;margin:24px auto;padding:0 16px;background:#101418;color:#eef}input,select,button{font-size:17px;padding:10px;margin:6px 0;width:100%%;box-sizing:border-box}button{cursor:pointer}.card{background:#182027;padding:18px;border-radius:16px;margin:12px 0}.warn{background:#402020}.ok{background:#18352a}code{word-break:break-all}</style></head><body>"
        "<h1>KONTAKTS App06</h1>"
        "<div class='card'><h2>Network</h2><b>State:</b> %s<br><b>STA IP:</b> %s<br><b>Setup AP:</b> %s</div>"
        "%s"
        "<div class='card'><h2>GitHub OTA</h2><div id='ota'>"
        "<b>State:</b> %s<br><b>Installed:</b> %s<br><b>Available:</b> %s<br><b>Running:</b> %s<br><b>Image:</b> %s<br><b>Progress:</b> %d%%<br><b>Message:</b> %s"
        "</div><p><code>%s</code></p>"
        "<form method='post' action='/ota/check'><button>CHECK GITHUB</button></form>"
        "<form method='post' action='/ota/install'><button>DOWNLOAD & INSTALL</button></form>"
        "<form method='post' action='/ota/confirm'><button>CONFIRM RUNNING IMAGE</button></form>"
        "<form method='post' action='/ota/rollback'><button>ROLLBACK PENDING CANDIDATE</button></form>"
        "<form method='post' action='/ota/recovery'><button>BOOT FACTORY RECOVERY</button></form></div>"
        "<div class='card'><form method='post' action='/clear'><button>Clear saved Wi-Fi and use setup AP</button></form></div>"
        "<script>setInterval(()=>fetch('/ota/status').then(r=>r.json()).then(x=>{document.getElementById('ota').innerHTML='<b>State:</b> '+x.state+'<br><b>Installed:</b> '+x.current_version+'<br><b>Available:</b> '+x.available_version+'<br><b>Running:</b> '+x.running_partition+'<br><b>Image:</b> '+x.image_state+'<br><b>Progress:</b> '+x.progress_percent+'%%<br><b>Message:</b> '+x.message}).catch(()=>{}),2000);</script>"
        "%s</body></html>",
        network_manager_state_name(), network_manager_sta_ip(), network_manager_ap_ssid(),
        network_form,
        ota.state, ota.current_version, ota.available_version[0] ? ota.available_version : "-",
        ota.running_partition, ota.image_state, ota.progress_percent, ota.message,
        ota_manager_manifest_url(), scan_script);

    httpd_resp_set_type(req, "text/html");
    esp_err_t err = httpd_resp_send(req, html, n);
    free(html);
    return err;
}

static esp_err_t favicon_get(httpd_req_t *req)
{
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_get(httpd_req_t *req)
{
    char json[192];
    int n = snprintf(json, sizeof(json),
                     "{\"state\":\"%s\",\"ip\":\"%s\",\"ap\":\"%s\"}",
                     network_manager_state_name(), network_manager_sta_ip(), network_manager_ap_ssid());
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, n);
}

static esp_err_t ota_status_get(httpd_req_t *req)
{
    ota_status_t s;
    ota_manager_get_status(&s);
    char json[768];
    int n = snprintf(json, sizeof(json),
                     "{\"state\":\"%s\",\"current_version\":\"%s\",\"available_version\":\"%s\",\"running_partition\":\"%s\",\"image_state\":\"%s\",\"progress_percent\":%d,\"busy\":%s,\"update_available\":%s,\"pending_verify\":%s,\"message\":\"%s\"}",
                     s.state, s.current_version, s.available_version, s.running_partition, s.image_state,
                     s.progress_percent, s.busy ? "true" : "false",
                     s.update_available ? "true" : "false",
                     s.pending_verify ? "true" : "false", s.message);
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
        return ESP_OK;
    }
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "scan failed");
        return ESP_OK;
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
        return ESP_OK;
    }
    char body[256];
    int got = httpd_req_recv(req, body, req->content_len);
    if (got <= 0) return ESP_FAIL;
    body[got] = '\0';

    char ssid[33] = {0};
    char password[65] = {0};
    if (!form_value(body, "ssid", ssid, sizeof(ssid)) || ssid[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
        return ESP_OK;
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
    network_manager_disconnect_sta();
    ESP_RETURN_ON_ERROR(network_manager_enter_ap_setup(), TAG, "AP setup failed");
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_sendstr(req, "Cleared");
}

static esp_err_t ota_action_response(httpd_req_t *req, esp_err_t err)
{
    if (err != ESP_OK) {
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "text/plain");
        char msg[96];
        snprintf(msg, sizeof(msg), "OTA action rejected: %s", esp_err_to_name(err));
        httpd_resp_sendstr(req, msg);
        return ESP_OK;
    }
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_sendstr(req, "OTA action accepted");
}

static esp_err_t ota_check_post(httpd_req_t *req) { return ota_action_response(req, ota_manager_start_check()); }
static esp_err_t ota_install_post(httpd_req_t *req) { return ota_action_response(req, ota_manager_start_install()); }
static esp_err_t ota_confirm_post(httpd_req_t *req) { return ota_action_response(req, ota_manager_confirm_running()); }
static esp_err_t ota_rollback_post(httpd_req_t *req) { return ota_action_response(req, ota_manager_start_rollback()); }
static esp_err_t ota_recovery_post(httpd_req_t *req) { return ota_action_response(req, ota_manager_start_recovery()); }

esp_err_t web_setup_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.stack_size = 8192;
    ESP_RETURN_ON_ERROR(httpd_start(&s_httpd, &config), TAG, "httpd_start failed");

    const httpd_uri_t handlers[] = {
        {.uri = "/", .method = HTTP_GET, .handler = root_get},
        {.uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_get},
        {.uri = "/status", .method = HTTP_GET, .handler = status_get},
        {.uri = "/scan", .method = HTTP_GET, .handler = scan_get},
        {.uri = "/save", .method = HTTP_POST, .handler = save_post},
        {.uri = "/clear", .method = HTTP_POST, .handler = clear_post},
        {.uri = "/ota/status", .method = HTTP_GET, .handler = ota_status_get},
        {.uri = "/ota/check", .method = HTTP_POST, .handler = ota_check_post},
        {.uri = "/ota/install", .method = HTTP_POST, .handler = ota_install_post},
        {.uri = "/ota/confirm", .method = HTTP_POST, .handler = ota_confirm_post},
        {.uri = "/ota/rollback", .method = HTTP_POST, .handler = ota_rollback_post},
        {.uri = "/ota/recovery", .method = HTTP_POST, .handler = ota_recovery_post},
    };

    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); ++i) {
        ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_httpd, &handlers[i]), TAG, "URI handler failed");
    }

    ESP_LOGI(TAG, "App06 HTTP control server started");
    return ESP_OK;
}
