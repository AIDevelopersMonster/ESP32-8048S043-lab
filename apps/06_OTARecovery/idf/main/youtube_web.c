#include "youtube_web.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "youtube_service.h"

#define TAG "APP08_YT_WEB"
#define YT_PAGE_BYTES 12288

static void url_decode(char *dst, size_t dst_len, const char *src)
{
    size_t di = 0;
    for (size_t i = 0; src && src[i] && di + 1 < dst_len; ++i) {
        if (src[i] == '+') dst[di++] = ' ';
        else if (src[i] == '%' && src[i + 1] && src[i + 2]) {
            char h[3] = {src[i + 1], src[i + 2], 0};
            dst[di++] = (char)strtol(h, NULL, 16);
            i += 2;
        } else dst[di++] = src[i];
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
    char encoded[160];
    if (n >= sizeof(encoded)) n = sizeof(encoded) - 1;
    memcpy(encoded, p, n);
    encoded[n] = '\0';
    url_decode(out, out_len, encoded);
    return true;
}

static esp_err_t redirect_youtube(httpd_req_t *req)
{
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/youtube");
    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t youtube_get(httpd_req_t *req)
{
    youtube_status_t s;
    youtube_service_get_status(&s);

    char *html = heap_caps_calloc(1, YT_PAGE_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!html) html = calloc(1, YT_PAGE_BYTES);
    if (!html) {
        ESP_LOGE(TAG, "YouTube page allocation failed internal=%u largest_internal=%u psram=%u",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no memory for YouTube page");
        return ESP_OK;
    }

    int n = snprintf(html, YT_PAGE_BYTES,
        "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>KONTAKTS YouTube</title><style>body{font-family:sans-serif;max-width:820px;margin:20px auto;padding:0 14px;background:#101418;color:#eef}"
        ".card{background:#182027;padding:16px;border-radius:14px;margin:12px 0}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}.metric{background:#111820;padding:14px;border-radius:10px;text-align:center}.big{font-size:30px;font-weight:700}"
        "input,button{font-size:16px;padding:10px;margin:5px 0;width:100%%;box-sizing:border-box}button{cursor:pointer}.row{display:grid;grid-template-columns:repeat(5,1fr);gap:8px}.muted{color:#9aa7b2}@media(max-width:650px){.grid,.row{grid-template-columns:1fr 1fr}}</style></head><body>"
        "<h1>YouTube Dashboard</h1><div class='card'><b>Channel ID:</b> %s<br><b>Configured:</b> %s<br><b>State:</b> %s<br><b>Message:</b> %s<br><b>History:</b> %u days</div>"
        "<div class='grid'><div class='metric'>SUBSCRIBERS<div class='big'>%llu</div></div><div class='metric'>VIEWS<div class='big'>%llu</div></div><div class='metric'>VIDEOS<div class='big'>%llu</div></div></div>"
        "<div class='card'><b>Selected period:</b> %s<br><b>Views delta:</b> +%llu<br><b>Subscribers delta:</b> %+lld</div>"
        "<div class='card'><h2>Period</h2><div class='row'>"
        "<form method='post' action='/youtube/period'><input type='hidden' name='period' value='7'><button>7D</button></form>"
        "<form method='post' action='/youtube/period'><input type='hidden' name='period' value='30'><button>30D</button></form>"
        "<form method='post' action='/youtube/period'><input type='hidden' name='period' value='90'><button>90D</button></form>"
        "<form method='post' action='/youtube/period'><input type='hidden' name='period' value='0'><button>ALL</button></form>"
        "<form method='post' action='/youtube/refresh'><button>REFRESH</button></form></div></div>"
        "<div class='card'><h2>YouTube API settings</h2><p class='muted'>The API key is stored in NVS and is never displayed back by this page.</p>"
        "<form method='post' action='/youtube/save'><label>Channel ID</label><input name='channel' value='%s' required maxlength='63'>"
        "<label>API key</label><input name='api_key' type='password' placeholder='Paste AIza... key' required maxlength='95'><button>SAVE & TEST</button></form></div>"
        "<p><a href='/' style='color:#58a6ff'>Back to KONTAKTS Platform</a></p></body></html>",
        s.channel_id[0] ? s.channel_id : "-", s.configured ? "yes" : "no", s.state, s.message,
        (unsigned)s.history_count, (unsigned long long)s.subscribers, (unsigned long long)s.views,
        (unsigned long long)s.videos,
        s.period == YOUTUBE_PERIOD_ALL ? "ALL" : (s.period == YOUTUBE_PERIOD_7D ? "7D" : (s.period == YOUTUBE_PERIOD_90D ? "90D" : "30D")),
        (unsigned long long)s.period_views_delta, (long long)s.period_subscribers_delta,
        s.channel_id[0] ? s.channel_id : "UCplLC3QnAagQq2hw1G3RLvQ");

    if (n < 0 || n >= YT_PAGE_BYTES) {
        ESP_LOGE(TAG, "YouTube page render overflow n=%d cap=%d", n, YT_PAGE_BYTES);
        free(html);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "YouTube page render failed");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "YouTube page GET bytes=%d internal=%u largest_internal=%u psram=%u",
             n,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    httpd_resp_set_type(req, "text/html");
    esp_err_t err = httpd_resp_send(req, html, n);
    free(html);
    return err;
}

static esp_err_t youtube_save_post(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len >= 320) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
        return ESP_OK;
    }
    char body[320];
    int got = httpd_req_recv(req, body, req->content_len);
    if (got <= 0) return ESP_FAIL;
    body[got] = '\0';
    char channel[64] = {0};
    char api_key[96] = {0};
    if (!form_value(body, "channel", channel, sizeof(channel)) ||
        !form_value(body, "api_key", api_key, sizeof(api_key))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "channel and api_key required");
        return ESP_OK;
    }
    esp_err_t err = youtube_service_save_config(channel, api_key);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid YouTube configuration");
        return ESP_OK;
    }
    ESP_LOGI(TAG, "YouTube configuration saved for channel %s", channel);
    return redirect_youtube(req);
}

static esp_err_t youtube_refresh_post(httpd_req_t *req)
{
    esp_err_t err = youtube_service_request_refresh();
    if (err != ESP_OK) {
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "text/plain");
        return httpd_resp_sendstr(req, "YouTube is not configured");
    }
    return redirect_youtube(req);
}

static esp_err_t youtube_period_post(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len >= 64) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid period");
        return ESP_OK;
    }
    char body[64];
    int got = httpd_req_recv(req, body, req->content_len);
    if (got <= 0) return ESP_FAIL;
    body[got] = '\0';
    char value[16] = {0};
    if (!form_value(body, "period", value, sizeof(value))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "period required");
        return ESP_OK;
    }
    int p = atoi(value);
    youtube_period_t period = p == 7 ? YOUTUBE_PERIOD_7D : p == 90 ? YOUTUBE_PERIOD_90D : p == 0 ? YOUTUBE_PERIOD_ALL : YOUTUBE_PERIOD_30D;
    youtube_service_set_period(period);
    return redirect_youtube(req);
}

esp_err_t youtube_web_register(httpd_handle_t server)
{
    if (!server) return ESP_ERR_INVALID_ARG;
    const httpd_uri_t handlers[] = {
        {.uri = "/youtube", .method = HTTP_GET, .handler = youtube_get},
        {.uri = "/youtube/save", .method = HTTP_POST, .handler = youtube_save_post},
        {.uri = "/youtube/refresh", .method = HTTP_POST, .handler = youtube_refresh_post},
        {.uri = "/youtube/period", .method = HTTP_POST, .handler = youtube_period_post},
    };
    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); ++i) {
        esp_err_t err = httpd_register_uri_handler(server, &handlers[i]);
        if (err != ESP_OK) return err;
    }
    ESP_LOGI(TAG, "YouTube web settings registered");
    return ESP_OK;
}
