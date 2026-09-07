#include "storage_web.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "storage_fs.h"

#define TAG "APP07_STORAGE_WEB"
#define STORAGE_UPLOAD_MAX_BYTES (512 * 1024)
#define STORAGE_IO_CHUNK 4096
#define STORAGE_TEMP_FILE STORAGE_FS_BASE "/.upload.tmp"

static bool protected_name(const char *name)
{
    return strcmp(name, "widget.json") == 0 ||
           strcmp(name, "widget.tmp") == 0 ||
           strcmp(name, "widget.bak") == 0 ||
           strcmp(name, ".upload.tmp") == 0;
}

static bool valid_name(const char *name)
{
    if (!name || !name[0] || strlen(name) > 63 || protected_name(name)) return false;
    if (name[0] == '.') return false;
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p) {
        if (!(isalnum(*p) || *p == '.' || *p == '_' || *p == '-')) return false;
    }
    return true;
}

static bool query_name(httpd_req_t *req, char *out, size_t out_len)
{
    size_t qlen = httpd_req_get_url_query_len(req);
    if (qlen == 0 || qlen >= 160) return false;
    char query[160];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) return false;
    if (httpd_query_key_value(query, "name", out, out_len) != ESP_OK) return false;
    return valid_name(out);
}

static void make_path(char *out, size_t out_len, const char *name)
{
    snprintf(out, out_len, STORAGE_FS_BASE "/%s", name);
}

static esp_err_t list_get(httpd_req_t *req)
{
    size_t total = 0, used = 0;
    storage_fs_info(&total, &used);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "{\"total\":");
    char num[64];
    snprintf(num, sizeof(num), "%u,\"used\":%u,\"files\":[",
             (unsigned)total, (unsigned)used);
    httpd_resp_sendstr_chunk(req, num);

    DIR *dir = opendir(STORAGE_FS_BASE);
    bool first = true;
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            const char *name = entry->d_name;
            if (!name || !name[0] || strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

            char path[128];
            make_path(path, sizeof(path), name);
            struct stat st = {0};
            if (stat(path, &st) != 0) continue;

            char item[256];
            snprintf(item, sizeof(item),
                     "%s{\"name\":\"%s\",\"size\":%u,\"protected\":%s}",
                     first ? "" : ",", name, (unsigned)st.st_size,
                     protected_name(name) ? "true" : "false");
            httpd_resp_sendstr_chunk(req, item);
            first = false;
        }
        closedir(dir);
    }

    httpd_resp_sendstr_chunk(req, "]}");
    return httpd_resp_sendstr_chunk(req, NULL);
}

static esp_err_t upload_post(httpd_req_t *req)
{
    char name[64];
    if (!query_name(req, name, sizeof(name))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid file name");
        return ESP_OK;
    }
    if (req->content_len <= 0 || req->content_len > STORAGE_UPLOAD_MAX_BYTES) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "file must be 1..524288 bytes");
        return ESP_OK;
    }

    char target[128];
    make_path(target, sizeof(target), name);
    unlink(STORAGE_TEMP_FILE);
    FILE *f = fopen(STORAGE_TEMP_FILE, "wb");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "cannot create temp file");
        return ESP_OK;
    }

    char *buf = heap_caps_malloc(STORAGE_IO_CHUNK, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) buf = malloc(STORAGE_IO_CHUNK);
    if (!buf) {
        fclose(f);
        unlink(STORAGE_TEMP_FILE);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no memory");
        return ESP_OK;
    }

    int remaining = req->content_len;
    esp_err_t result = ESP_OK;
    while (remaining > 0) {
        int want = remaining > STORAGE_IO_CHUNK ? STORAGE_IO_CHUNK : remaining;
        int got = httpd_req_recv(req, buf, want);
        if (got <= 0 || fwrite(buf, 1, (size_t)got, f) != (size_t)got) {
            result = ESP_FAIL;
            break;
        }
        remaining -= got;
    }
    free(buf);

    if (fflush(f) != 0 || fsync(fileno(f)) != 0) result = ESP_FAIL;
    fclose(f);

    if (result != ESP_OK) {
        unlink(STORAGE_TEMP_FILE);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "upload failed");
        return ESP_OK;
    }

    unlink(target);
    if (rename(STORAGE_TEMP_FILE, target) != 0) {
        unlink(STORAGE_TEMP_FILE);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "activate failed");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "FILE UPLOAD PASS name=%s bytes=%d", name, req->content_len);
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_sendstr(req, "UPLOAD PASS");
}

static esp_err_t download_get(httpd_req_t *req)
{
    char name[64];
    if (!query_name(req, name, sizeof(name))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid file name");
        return ESP_OK;
    }

    char path[128];
    make_path(path, sizeof(path), name);
    FILE *f = fopen(path, "rb");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "file not found");
        return ESP_OK;
    }

    char disposition[128];
    snprintf(disposition, sizeof(disposition), "attachment; filename=\"%s\"", name);
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Content-Disposition", disposition);

    char *buf = heap_caps_malloc(STORAGE_IO_CHUNK, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) buf = malloc(STORAGE_IO_CHUNK);
    if (!buf) {
        fclose(f);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no memory");
        return ESP_OK;
    }

    size_t got;
    esp_err_t err = ESP_OK;
    while ((got = fread(buf, 1, STORAGE_IO_CHUNK, f)) > 0) {
        if (httpd_resp_send_chunk(req, buf, got) != ESP_OK) {
            err = ESP_FAIL;
            break;
        }
    }
    free(buf);
    fclose(f);
    if (err != ESP_OK) return err;
    return httpd_resp_send_chunk(req, NULL, 0);
}

static esp_err_t delete_post(httpd_req_t *req)
{
    char name[64];
    if (!query_name(req, name, sizeof(name))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid or protected file name");
        return ESP_OK;
    }

    char path[128];
    make_path(path, sizeof(path), name);
    if (unlink(path) != 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "file not found");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "FILE DELETE PASS name=%s", name);
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_sendstr(req, "DELETE PASS");
}

esp_err_t storage_web_register(httpd_handle_t server)
{
    if (!server) return ESP_ERR_INVALID_ARG;

    const httpd_uri_t handlers[] = {
        {.uri="/storage/list", .method=HTTP_GET, .handler=list_get},
        {.uri="/storage/upload", .method=HTTP_POST, .handler=upload_post},
        {.uri="/storage/download", .method=HTTP_GET, .handler=download_get},
        {.uri="/storage/delete", .method=HTTP_POST, .handler=delete_post},
    };

    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); ++i) {
        esp_err_t err = httpd_register_uri_handler(server, &handlers[i]);
        if (err != ESP_OK) return err;
    }
    ESP_LOGI(TAG, "Web Storage Manager registered");
    return ESP_OK;
}
