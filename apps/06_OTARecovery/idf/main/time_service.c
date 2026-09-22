#include "time_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define TAG "APP07_TIME"
#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE_POSIX "EET-2EEST,M3.5.0/3,M10.5.0/4"
#define TIME_SYNC_TIMEOUT_MS 12000
#define TIME_SYNC_TASK_STACK 4096

static SemaphoreHandle_t s_lock;
static time_service_status_t s_status;

static const char *weekday_name(int wday)
{
    static const char *names[] = {
        "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"
    };
    return (wday >= 0 && wday < 7) ? names[wday] : "--";
}

static void set_state_locked(const char *state)
{
    strlcpy(s_status.state, state ? state : "UNKNOWN", sizeof(s_status.state));
}

static void sync_task(void *arg)
{
    (void)arg;
    esp_err_t err = esp_netif_sntp_start();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NTP sync requested from %s", NTP_SERVER);
        err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(TIME_SYNC_TIMEOUT_MS));
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_status.syncing = false;
    if (err == ESP_OK) {
        s_status.synchronized = true;
        s_status.last_sync_epoch = time(NULL);
        set_state_locked("SYNCED");
    } else {
        set_state_locked("SYNC FAILED");
    }
    xSemaphoreGive(s_lock);

    if (err == ESP_OK) {
        struct tm now = {0};
        time_t epoch = time(NULL);
        localtime_r(&epoch, &now);
        ESP_LOGI(TAG, "NTP SYNC PASS %04d-%02d-%02d %02d:%02d:%02d %s",
                 now.tm_year + 1900, now.tm_mon + 1, now.tm_mday,
                 now.tm_hour, now.tm_min, now.tm_sec, weekday_name(now.tm_wday));
    } else {
        ESP_LOGW(TAG, "NTP sync failed: %s", esp_err_to_name(err));
    }
    vTaskDelete(NULL);
}

esp_err_t time_service_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    memset(&s_status, 0, sizeof(s_status));
    strlcpy(s_status.timezone, "Europe/Helsinki", sizeof(s_status.timezone));
    set_state_locked("WAITING FOR WIFI");

    setenv("TZ", TIMEZONE_POSIX, 1);
    tzset();

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER);
    config.start = false;
    esp_err_t err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SNTP init failed: %s", esp_err_to_name(err));
        return err;
    }

    s_status.ntp_initialized = true;
    ESP_LOGI(TAG, "Time service ready server=%s timezone=%s", NTP_SERVER, s_status.timezone);
    return ESP_OK;
}

esp_err_t time_service_request_sync(void)
{
    if (!s_lock || !s_status.ntp_initialized) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_status.syncing) {
        xSemaphoreGive(s_lock);
        return ESP_ERR_INVALID_STATE;
    }
    s_status.syncing = true;
    set_state_locked("SYNCING");
    xSemaphoreGive(s_lock);

    BaseType_t ok = xTaskCreate(sync_task, "ntp_sync", TIME_SYNC_TASK_STACK, NULL, 5, NULL);
    if (ok != pdPASS) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_status.syncing = false;
        set_state_locked("NO MEMORY");
        xSemaphoreGive(s_lock);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

bool time_service_get_local_time(struct tm *out)
{
    if (!out) return false;
    time_t now = time(NULL);
    localtime_r(&now, out);

    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool valid = s_status.synchronized;
    xSemaphoreGive(s_lock);
    return valid;
}

void time_service_get_status(time_service_status_t *out)
{
    if (!out || !s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_status;
    xSemaphoreGive(s_lock);
}

void time_service_format_binding(const char *binding, char *out, size_t out_len)
{
    if (!binding || !out || out_len == 0) return;
    struct tm now = {0};
    bool valid = time_service_get_local_time(&now);
    time_service_status_t status = {0};
    time_service_get_status(&status);

    if (strcmp(binding, "time.clock") == 0) {
        if (valid) snprintf(out, out_len, "%02d:%02d:%02d", now.tm_hour, now.tm_min, now.tm_sec);
        else strlcpy(out, "--:--:--", out_len);
    } else if (strcmp(binding, "time.date") == 0) {
        if (valid) snprintf(out, out_len, "%02d.%02d.%04d", now.tm_mday, now.tm_mon + 1, now.tm_year + 1900);
        else strlcpy(out, "--.--.----", out_len);
    } else if (strcmp(binding, "time.year") == 0) {
        if (valid) snprintf(out, out_len, "%04d", now.tm_year + 1900);
        else strlcpy(out, "----", out_len);
    } else if (strcmp(binding, "time.weekday") == 0) {
        strlcpy(out, valid ? weekday_name(now.tm_wday) : "WAITING", out_len);
    } else if (strcmp(binding, "time.sync_state") == 0) {
        strlcpy(out, status.state, out_len);
    } else if (strcmp(binding, "time.last_sync") == 0) {
        if (status.last_sync_epoch > 0) {
            struct tm synced = {0};
            localtime_r(&status.last_sync_epoch, &synced);
            snprintf(out, out_len, "%02d.%02d %02d:%02d", synced.tm_mday, synced.tm_mon + 1,
                     synced.tm_hour, synced.tm_min);
        } else {
            strlcpy(out, "never", out_len);
        }
    } else {
        strlcpy(out, "unsupported", out_len);
    }
}
