#include "help_web.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "esp_log.h"

#include "sd_manager.h"

#define TAG "APP09_HELP_WEB"
#define HELP_IO_CHUNK 2048
#define HELP_MANIFEST_MAX 8192
#define HELP_NAME_MAX 63

static void set_html_headers(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(req, "Referrer-Policy", "no-referrer");
    httpd_resp_set_hdr(req, "Content-Security-Policy",
                       "default-src 'none'; style-src 'self'; img-src 'self' data: https:; "
                       "font-src 'self'; script-src 'none'; connect-src 'none'; frame-src 'none'; "
                       "object-src 'none'; form-action 'none'; base-uri 'none'");
}

static bool safe_package_name(const char *name)
{
    if (!name || !name[0]) return false;
    size_t len = strnlen(name, HELP_NAME_MAX + 1);
    if (len == 0 || len > HELP_NAME_MAX) return false;
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p) {
        if (!(isalnum(*p) || *p == '-' || *p == '_' || *p == '.')) return false;
    }
    return true;
}

static bool query_value(httpd_req_t *req, const char *key, char *out, size_t out_len)
{
    size_t qlen = httpd_req_get_url_query_len(req);
    if (!qlen || qlen >= 192) return false;
    char query[192];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) return false;
    return httpd_query_key_value(query, key, out, out_len) == ESP_OK;
}

static bool ensure_sd(httpd_req_t *req)
{
    if (sd_manager_is_mounted()) return true;
    if (sd_manager_mount() == ESP_OK) return true;
    httpd_resp_set_status(req, "503 Service Unavailable");
    set_html_headers(req);
    httpd_resp_sendstr(req,
        "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>KONTAKTS Help</title></head><body><h1>SD Help unavailable</h1>"
        "<p>Insert the SD card and try again.</p><p><a href='/'>Back to platform home</a></p></body></html>");
    return false;
}

static esp_err_t send_file(httpd_req_t *req, const char *path, const char *content_type, bool html)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "help file not found");
        return ESP_OK;
    }
    if (html) set_html_headers(req);
    else {
        httpd_resp_set_type(req, content_type);
        httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
        httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    }
    char buf[HELP_IO_CHUNK];
    size_t got;
    esp_err_t err = ESP_OK;
    while ((got = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (httpd_resp_send_chunk(req, buf, got) != ESP_OK) { err = ESP_FAIL; break; }
    }
    fclose(f);
    if (err != ESP_OK) return err;
    return httpd_resp_send_chunk(req, NULL, 0);
}

static char *read_manifest(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size <= 0 || size > HELP_MANIFEST_MAX) { fclose(f); return NULL; }
    rewind(f);
    char *text = malloc((size_t)size + 1);
    if (!text) { fclose(f); return NULL; }
    size_t got = fread(text, 1, (size_t)size, f);
    fclose(f);
    if (got != (size_t)size) { free(text); return NULL; }
    text[got] = '\0';
    return text;
}

static bool manifest_help(const char *folder, char *display_name, size_t display_len)
{
    if (!safe_package_name(folder)) return false;
    char manifest[256];
    int n = snprintf(manifest, sizeof(manifest), "%s/%.63s/package.json", SD_MANAGER_WIDGET_ROOT, folder);
    if (n < 0 || (size_t)n >= sizeof(manifest)) return false;
    char *text = read_manifest(manifest);
    if (!text) return false;
    cJSON *root = cJSON_Parse(text);
    free(text);
    if (!root) return false;
    const cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    const cJSON *docs = cJSON_GetObjectItemCaseSensitive(root, "documentation");
    const cJSON *entry = cJSON_IsObject(docs) ? cJSON_GetObjectItemCaseSensitive(docs, "entry") : NULL;
    bool ok = cJSON_IsString(entry) && entry->valuestring && strcmp(entry->valuestring, "html/index.html") == 0;
    if (ok) {
        const char *shown = cJSON_IsString(name) && name->valuestring && name->valuestring[0] ? name->valuestring : folder;
        strlcpy(display_name, shown, display_len);
    }
    cJSON_Delete(root);
    return ok;
}

static esp_err_t send_escaped(httpd_req_t *req, const char *text)
{
    for (const unsigned char *p = (const unsigned char *)text; p && *p; ++p) {
        const char *replacement = NULL;
        switch (*p) {
            case '&': replacement = "&amp;"; break;
            case '<': replacement = "&lt;"; break;
            case '>': replacement = "&gt;"; break;
            case '"': replacement = "&quot;"; break;
            case '\'': replacement = "&#39;"; break;
            default: break;
        }
        if (replacement) {
            if (httpd_resp_sendstr_chunk(req, replacement) != ESP_OK) return ESP_FAIL;
        } else {
            char one[2] = {(char)*p, 0};
            if (httpd_resp_sendstr_chunk(req, one) != ESP_OK) return ESP_FAIL;
        }
    }
    return ESP_OK;
}

static esp_err_t help_root_get(httpd_req_t *req)
{
    if (!ensure_sd(req)) return ESP_OK;
    set_html_headers(req);
    httpd_resp_sendstr_chunk(req,
        "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>KONTAKTS Help</title><link rel='stylesheet' href='/help/style.css'></head><body>"
        "<header><div class='eyebrow'>ESP32-8048S043 / KONTAKTS</div><h1>Help Center</h1>"
        "<div class='topnav'><a href='/'>Platform Home</a><a href='#applications'>Project Pages</a>"
        "<a href='https://github.com/AIDevelopersMonster/ESP32-8048S043-lab'>GitHub Project</a></div>"
        "<p>System documentation and help carried by applications on the SD card.</p></header>"
        "<main><h2>System</h2><div class='grid'>"
        "<a class='card' href='/help/system?doc=user'><b>User Guide</b><span>Setup, Wi-Fi, SD apps, OTA and recovery</span></a>"
        "<a class='card' href='/help/system?doc=application-programmer'><b>Application Programmer</b><span>Packages, JSON widgets, services and bindings</span></a>"
        "<a class='card' href='/help/system?doc=system-programmer'><b>System Programmer</b><span>Platform architecture, boot, HTTP and runtime internals</span></a>"
        "<a class='card' href='/help/system?doc=hardware'><b>Hardware Reference</b><span>Board, GPIO, connectors, display, touch, SD and datasheets</span></a>"
        "</div><h2 id='applications'>Project / Application Pages</h2><div class='grid'>");

    DIR *dir = opendir(SD_MANAGER_WIDGET_ROOT);
    size_t app_count = 0;
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.' || !safe_package_name(entry->d_name)) continue;
            char display[96] = {0};
            if (!manifest_help(entry->d_name, display, sizeof(display))) continue;
            char doc_path[256];
            int path_n = snprintf(doc_path, sizeof(doc_path), "%s/%.63s/html/index.html", SD_MANAGER_WIDGET_ROOT, entry->d_name);
            if (path_n < 0 || (size_t)path_n >= sizeof(doc_path)) continue;
            struct stat st;
            if (stat(doc_path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
            char link[160];
            int link_n = snprintf(link, sizeof(link), "<a class='card' href='/help/app?name=%.63s'><b>", entry->d_name);
            if (link_n < 0 || (size_t)link_n >= sizeof(link)) continue;
            httpd_resp_sendstr_chunk(req, link);
            if (send_escaped(req, display) != ESP_OK) { closedir(dir); return ESP_FAIL; }
            httpd_resp_sendstr_chunk(req, "</b><span>Project page and application help from SD package</span></a>");
            app_count++;
        }
        closedir(dir);
    }
    if (!app_count) httpd_resp_sendstr_chunk(req, "<div class='note'>No application help packages found.</div>");
    httpd_resp_sendstr_chunk(req,
        "</div></main><footer><a href='/'>Platform Home</a> · Documentation is served from SD. HTML updates do not require reflashing.</footer>"
        "</body></html>");
    return httpd_resp_sendstr_chunk(req, NULL);
}

static esp_err_t help_system_get(httpd_req_t *req)
{
    if (!ensure_sd(req)) return ESP_OK;
    char doc[64] = {0};
    if (!query_value(req, "doc", doc, sizeof(doc))) strlcpy(doc, "index", sizeof(doc));
    const char *path = NULL;
    if (strcmp(doc, "index") == 0) path = SD_MANAGER_BASE_PATH "/wiki/system/index.html";
    else if (strcmp(doc, "user") == 0) path = SD_MANAGER_BASE_PATH "/wiki/system/user/index.html";
    else if (strcmp(doc, "application-programmer") == 0) path = SD_MANAGER_BASE_PATH "/wiki/system/application-programmer/index.html";
    else if (strcmp(doc, "system-programmer") == 0) path = SD_MANAGER_BASE_PATH "/wiki/system/system-programmer/index.html";
    else if (strcmp(doc, "hardware") == 0) path = SD_MANAGER_BASE_PATH "/wiki/system/hardware/index.html";
    else { httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "unknown system help page"); return ESP_OK; }
    return send_file(req, path, "text/html; charset=utf-8", true);
}

static esp_err_t help_app_get(httpd_req_t *req)
{
    if (!ensure_sd(req)) return ESP_OK;
    char name[HELP_NAME_MAX + 1] = {0};
    if (!query_value(req, "name", name, sizeof(name)) || !safe_package_name(name)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid application name"); return ESP_OK;
    }
    char display[96] = {0};
    if (!manifest_help(name, display, sizeof(display))) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "application does not declare HTML help"); return ESP_OK;
    }
    char path[256];
    int n = snprintf(path, sizeof(path), "%s/%.63s/html/index.html", SD_MANAGER_WIDGET_ROOT, name);
    if (n < 0 || (size_t)n >= sizeof(path)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "application help path too long"); return ESP_OK;
    }
    return send_file(req, path, "text/html; charset=utf-8", true);
}

static esp_err_t help_style_get(httpd_req_t *req)
{
    if (!ensure_sd(req)) return ESP_OK;
    const char *path = SD_MANAGER_BASE_PATH "/wiki/assets/style.css";
    FILE *probe = fopen(path, "rb");
    if (probe) { fclose(probe); return send_file(req, path, "text/css; charset=utf-8", false); }
    httpd_resp_set_type(req, "text/css; charset=utf-8");
    return httpd_resp_sendstr(req, "body{font-family:sans-serif;max-width:900px;margin:auto;padding:20px}a{color:#0969da}");
}

esp_err_t help_web_register(httpd_handle_t server)
{
    if (!server) return ESP_ERR_INVALID_ARG;
    const httpd_uri_t handlers[] = {
        {.uri = "/help", .method = HTTP_GET, .handler = help_root_get},
        {.uri = "/help/system", .method = HTTP_GET, .handler = help_system_get},
        {.uri = "/help/app", .method = HTTP_GET, .handler = help_app_get},
        {.uri = "/help/style.css", .method = HTTP_GET, .handler = help_style_get},
    };
    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); ++i) {
        esp_err_t err = httpd_register_uri_handler(server, &handlers[i]);
        if (err != ESP_OK) return err;
    }
    ESP_LOGI(TAG, "SD browser help registered at /help");
    return ESP_OK;
}
