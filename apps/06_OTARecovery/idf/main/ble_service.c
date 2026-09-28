#include "ble_service.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "store/config/ble_store_config.h"

#include "command_service.h"

#define TAG "APP16_BLE"
#define BLE_DEVICE_NAME "KONTAKTS-8048"
#define BLE_COMMAND_MAX 128
#define BLE_RESPONSE_MAX 1024
#define BLE_QUEUE_LEN 4

typedef struct {
    char command[BLE_COMMAND_MAX];
} ble_command_job_t;

static QueueHandle_t s_queue;
static SemaphoreHandle_t s_response_lock;
static char s_response[BLE_RESPONSE_MAX] = "OK BLE READY";
static bool s_ready;
static uint8_t s_addr_type;

static const ble_uuid16_t s_service_uuid = BLE_UUID16_INIT(0xFFF0);
static const ble_uuid16_t s_command_uuid = BLE_UUID16_INIT(0xFFF1);
static const ble_uuid16_t s_response_uuid = BLE_UUID16_INIT(0xFFF2);

static void set_response(const char *text)
{
    if (!s_response_lock) return;
    xSemaphoreTake(s_response_lock, portMAX_DELAY);
    strlcpy(s_response, text ? text : "", sizeof(s_response));
    xSemaphoreGive(s_response_lock);
}

static int gatt_access(uint16_t conn_handle,
                       uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt,
                       void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    const uintptr_t which = (uintptr_t)arg;

    if (which == 1 && ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        const uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len == 0 || len >= BLE_COMMAND_MAX) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;

        ble_command_job_t job = {0};
        if (os_mbuf_copydata(ctxt->om, 0, len, job.command) != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        job.command[len] = '\0';

        if (!s_queue || xQueueSend(s_queue, &job, 0) != pdTRUE) {
            set_response("ERR BLE BUSY");
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        set_response("OK BLE COMMAND QUEUED");
        return 0;
    }

    if (which == 2 && ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        char copy[BLE_RESPONSE_MAX];
        xSemaphoreTake(s_response_lock, portMAX_DELAY);
        strlcpy(copy, s_response, sizeof(copy));
        xSemaphoreGive(s_response_lock);
        return os_mbuf_append(ctxt->om, copy, strlen(copy)) == 0
                   ? 0
                   : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def s_gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &s_service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &s_command_uuid.u,
                .access_cb = gatt_access,
                .arg = (void *)1,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = &s_response_uuid.u,
                .access_cb = gatt_access,
                .arg = (void *)2,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {0}
        },
    },
    {0}
};

static void advertise(void);

static int gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Phone connected handle=%u", event->connect.conn_handle);
            } else {
                ESP_LOGW(TAG, "BLE connect failed status=%d", event->connect.status);
                advertise();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Phone disconnected reason=%d", event->disconnect.reason);
            advertise();
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            advertise();
            return 0;

        default:
            return 0;
    }
}

static void advertise(void)
{
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)BLE_DEVICE_NAME;
    fields.name_len = strlen(BLE_DEVICE_NAME);
    fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_set_fields rc=%d", rc);
        return;
    }

    struct ble_gap_adv_params params = {0};
    params.conn_mode = BLE_GAP_CONN_MODE_UND;
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(s_addr_type, NULL, BLE_HS_FOREVER, &params, gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_start rc=%d", rc);
        return;
    }
    ESP_LOGI(TAG, "Advertising as %s", BLE_DEVICE_NAME);
}

static void on_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_hs_util_ensure_addr rc=%d", rc);
        return;
    }

    rc = ble_hs_id_infer_auto(0, &s_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_hs_id_infer_auto rc=%d", rc);
        return;
    }

    s_ready = true;
    advertise();
}

static void on_reset(int reason)
{
    s_ready = false;
    ESP_LOGE(TAG, "NimBLE reset reason=%d", reason);
}

static void host_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void command_task(void *arg)
{
    (void)arg;
    ble_command_job_t job;
    char response[BLE_RESPONSE_MAX];

    for (;;) {
        if (xQueueReceive(s_queue, &job, portMAX_DELAY) != pdTRUE) continue;

        ESP_LOGI(TAG, "CMD RX: %s", job.command);
        esp_err_t err = command_service_execute(job.command, response, sizeof(response));
        set_response(response);
        ESP_LOGI(TAG, "CMD %s: %s", err == ESP_OK ? "OK" : "ERR", response);
    }
}

esp_err_t ble_service_init(void)
{
    s_response_lock = xSemaphoreCreateMutex();
    s_queue = xQueueCreate(BLE_QUEUE_LEN, sizeof(ble_command_job_t));
    if (!s_response_lock || !s_queue) return ESP_ERR_NO_MEM;

    BaseType_t task_ok = xTaskCreate(command_task, "ble_cmd", 4096, NULL, 5, NULL);
    if (task_ok != pdPASS) return ESP_ERR_NO_MEM;

    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed: %s", esp_err_to_name(err));
        return err;
    }

    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    int rc = ble_gatts_count_cfg(s_gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg rc=%d", rc);
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(s_gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs rc=%d", rc);
        return ESP_FAIL;
    }

    rc = ble_svc_gap_device_name_set(BLE_DEVICE_NAME);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_svc_gap_device_name_set rc=%d", rc);
        return ESP_FAIL;
    }

    ble_store_config_init();
    nimble_port_freertos_init(host_task);

    ESP_LOGI(TAG, "BLE transport initialized; service=0xFFF0 command=0xFFF1 response=0xFFF2");
    return ESP_OK;
}

bool ble_service_is_ready(void)
{
    return s_ready;
}
