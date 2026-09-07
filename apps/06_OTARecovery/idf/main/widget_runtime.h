#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#define WIDGET_MAX_JSON_BYTES 32768
#define WIDGET_MAX_OBJECTS 24
#define WIDGET_MAX_BOUND_LABELS 8

typedef enum {
    WIDGET_OBJECT_LABEL = 0,
    WIDGET_OBJECT_BAR,
    WIDGET_OBJECT_BUTTON,
} widget_object_type_t;

typedef struct {
    widget_object_type_t type;
    int x;
    int y;
    int w;
    int h;
    int value;
    uint32_t color;
    char text[161];
    char binding[33];
    char prefix[65];
    char suffix[65];
    char action[25];
} widget_object_t;

typedef struct {
    bool valid;
    char id[49];
    char name[65];
    char version[25];
    uint32_t background;
    size_t object_count;
    widget_object_t objects[WIDGET_MAX_OBJECTS];
} widget_model_t;

typedef struct {
    bool installed;
    uint32_t generation;
    size_t file_size;
    char id[49];
    char name[65];
    char version[25];
    char status[96];
} widget_info_t;

esp_err_t widget_runtime_init(void);
esp_err_t widget_runtime_install_json(const char *json, size_t len, char *reason, size_t reason_len);
esp_err_t widget_runtime_delete(void);
void widget_runtime_get_info(widget_info_t *out);
void widget_runtime_get_model(widget_model_t *out);
uint32_t widget_runtime_generation(void);
