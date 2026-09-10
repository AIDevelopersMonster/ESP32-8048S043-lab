# App09 links the SD/FAT/SPI stack into the same platform image. On the physical
# ESP32-S3 target the original 32-line LVGL partial draw buffer (51.2 KiB of
# contiguous internal RAM) no longer fits after RGB panel + touch init.
# Generate the display source with an 8-line (12.8 KiB) partial buffer while
# preserving the proven App08 renderer and all 800x480 framebuffer settings.
set(DISPLAY_OTA_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/display_ota_app09.c")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/display_ota.c")
file(READ "${CMAKE_CURRENT_LIST_DIR}/display_ota.c" DISPLAY_OTA_TEXT)
string(REPLACE
    "#define LVGL_BUF_LINES 32"
    "#define LVGL_BUF_LINES 8"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)

# App09 canonical SD architecture: firmware owns only SYS | SD | WIDGET.
# Applications are discovered from /sd/widgets/*/package.json and rendered by
# the generic SD launcher. No application name or entrypoint is hard-coded here.
string(REPLACE
    "#include \"sd_manager.h\""
    "#include \"sd_manager.h\"\n#include \"sd_launcher.h\"\n#include \"metric_carousel.h\""
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "static lv_obj_t *s_sd_mount_button, *s_sd_dashboard_button, *s_sd_views_button, *s_sd_subscribers_button;"
    "static lv_obj_t *s_sd_mount_button;\nstatic sd_launcher_view_t *s_sd_launcher;"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "static sevenseg_clock_view_t *s_clocks[WIDGET_MAX_CLOCKS];\nstatic size_t s_clock_count;"
    "static sevenseg_clock_view_t *s_clocks[WIDGET_MAX_CLOCKS];\nstatic size_t s_clock_count;\nstatic metric_carousel_view_t *s_metric_carousels[WIDGET_MAX_METRIC_CAROUSELS];\nstatic size_t s_metric_carousel_count;"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "static void render_widget(void);"
    "static void render_widget(void);\nstatic void metric_value_cb(const char *binding, char *out, size_t out_len, void *user_data);"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "static void sd_dashboard_cb(lv_event_t *e)\n{\n    if (lv_event_get_code(e) == LV_EVENT_CLICKED)\n        sd_run_widget(\"widgets/youtube/dashboard.json\", \"Dashboard\");\n}\nstatic void sd_views_cb(lv_event_t *e)\n{\n    if (lv_event_get_code(e) == LV_EVENT_CLICKED)\n        sd_run_widget(\"widgets/youtube/views.json\", \"Views\");\n}\nstatic void sd_subscribers_cb(lv_event_t *e)\n{\n    if (lv_event_get_code(e) == LV_EVENT_CLICKED)\n        sd_run_widget(\"widgets/youtube/subscribers.json\", \"Subscribers\");\n}\n"
    "static void sd_catalog_run_cb(const sd_manager_entry_t *entry, void *user_data)\n{\n    (void)user_data;\n    if (!entry) return;\n    sd_run_widget(entry->relative_path, entry->entry_name);\n}\n"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)

# Product-style SD screen: no raw manifest JSON in the user surface. Keep
# application launchers on the right and reserve the left column for card and
# explicit offline update information.
string(REPLACE
    "    s_sd_packages = make_label(s_sd_panel, 24, 152, \"Packages: not scanned\", &lv_font_montserrat_18, 0xC9D1D9);"
    "    s_sd_packages = make_label(s_sd_panel, 24, 152, \"Applications: waiting for scan\", &lv_font_montserrat_18, 0xC9D1D9);"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "    lv_obj_set_width(s_sd_message, 714); lv_label_set_long_mode(s_sd_message, LV_LABEL_LONG_DOT);"
    "    lv_obj_set_width(s_sd_message, 210); lv_label_set_long_mode(s_sd_message, LV_LABEL_LONG_WRAP);"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "    s_sd_mount_button = make_button(s_sd_panel, 24, 238, 214, 52, \"MOUNT / RESCAN\", sd_mount_cb);\n    s_sd_dashboard_button = make_button(s_sd_panel, 252, 238, 214, 52, \"DASHBOARD\", sd_dashboard_cb);\n    s_sd_views_button = make_button(s_sd_panel, 480, 238, 214, 52, \"VIEWS\", sd_views_cb);\n    s_sd_subscribers_button = make_button(s_sd_panel, 24, 304, 214, 52, \"SUBSCRIBERS\", sd_subscribers_cb);\n    make_label(s_sd_panel, 252, 314, \"SD UPDATE: manifest contract only; flash writes disabled in this test.\", &lv_font_montserrat_18, 0x8B949E);"
    "    s_sd_mount_button = make_button(s_sd_panel, 24, 238, 214, 52, \"MOUNT / RESCAN\", sd_mount_cb);\n    s_sd_launcher = sd_launcher_create(s_sd_panel, 252, 190, 442, 182, sd_catalog_run_cb, NULL);\n    make_label(s_sd_panel, 24, 304, \"SYSTEM UPDATE\", &lv_font_montserrat_18, 0x58A6FF);\n    make_label(s_sd_panel, 24, 332, \"/sd/UPDATE  |  explicit install only\", &lv_font_montserrat_18, 0x8B949E);"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "        set_button_enabled(s_sd_dashboard_button, true);\n        set_button_enabled(s_sd_views_button, true);\n        set_button_enabled(s_sd_subscribers_button, true);\n"
    ""
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "        if (!s_sd_packages_scanned) {\n            char *packages = NULL;\n            esp_err_t err = sd_manager_packages_json(&packages);\n            if (err == ESP_OK && packages) {\n                lv_label_set_text_fmt(s_sd_packages, \"Packages: %s\", packages);\n                free(packages);\n            } else {\n                lv_label_set_text_fmt(s_sd_packages, \"Packages scan failed: %s\", esp_err_to_name(err));\n            }\n            s_sd_packages_scanned = true;\n        }"
    "        if (!s_sd_packages_scanned) {\n            size_t count = sd_manager_entry_count();\n            lv_label_set_text_fmt(s_sd_packages, \"Applications: %u launcher(s) discovered\", (unsigned)count);\n            if (s_sd_launcher) sd_launcher_refresh(s_sd_launcher);\n            s_sd_packages_scanned = true;\n        }"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "        lv_label_set_text(s_sd_packages, \"Packages: unavailable until SD is mounted\");"
    "        lv_label_set_text(s_sd_packages, \"Applications: mount SD to scan\");"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "        set_button_enabled(s_sd_dashboard_button, false);\n        set_button_enabled(s_sd_views_button, false);\n        set_button_enabled(s_sd_subscribers_button, false);\n"
    ""
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)

# Generic metric carousel capability. This is intentionally service-agnostic:
# any SD widget may cycle allowed bindings without adding application-specific
# firmware buttons or code paths.
string(REPLACE
    "    clear_clock_views();\n    clear_chart_views();\n    lv_obj_clean(s_widget_content); s_bound_count = 0;"
    "    clear_clock_views();\n    clear_chart_views();\n    for (size_t i = 0; i < s_metric_carousel_count; ++i) { metric_carousel_destroy(s_metric_carousels[i]); s_metric_carousels[i] = NULL; }\n    s_metric_carousel_count = 0;\n    lv_obj_clean(s_widget_content); s_bound_count = 0;"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "        } else if (o->type == WIDGET_OBJECT_CHART) {\n            create_chart_view(o);\n        }"
    "        } else if (o->type == WIDGET_OBJECT_CHART) {\n            create_chart_view(o);\n        } else if (o->type == WIDGET_OBJECT_METRIC_CAROUSEL && s_metric_carousel_count < WIDGET_MAX_METRIC_CAROUSELS) {\n            metric_carousel_view_t *carousel = metric_carousel_create(s_widget_content, o->x, o->y, o->w, o->h, o->text, o->interval_ms, o->carousel_items, o->carousel_item_count, metric_value_cb, NULL);\n            if (carousel) s_metric_carousels[s_metric_carousel_count++] = carousel;\n        }"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "static void refresh_bindings(void)"
    "static void metric_value_cb(const char *binding, char *out, size_t out_len, void *user_data)\n{\n    (void)user_data;\n    binding_value(binding, out, out_len);\n}\n\nstatic void refresh_bindings(void)"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "    for (size_t i = 0; i < s_clock_count; ++i) sevenseg_clock_update(s_clocks[i]);\n    refresh_charts();"
    "    for (size_t i = 0; i < s_clock_count; ++i) sevenseg_clock_update(s_clocks[i]);\n    for (size_t i = 0; i < s_metric_carousel_count; ++i) metric_carousel_update(s_metric_carousels[i]);\n    refresh_charts();"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)

file(WRITE "${DISPLAY_OTA_SOURCE}" "${DISPLAY_OTA_TEXT}")

idf_component_register(
    SRCS
        "main.c"
        "storage_credentials.c"
        "storage_fs.c"
        "storage_web.c"
        "sd_manager.c"
        "sd_launcher.c"
        "metric_carousel.c"
        "widget_runtime.c"
        "network_manager.c"
        "time_service.c"
        "youtube_service.c"
        "youtube_web.c"
        "sevenseg_clock.c"
        "web_setup.c"
        "ota_manager.c"
        "${DISPLAY_OTA_SOURCE}"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        spiffs
        fatfs
        sdmmc
        esp_driver_spi
        esp_wifi
        esp_event
        esp_netif
        esp_http_server
        esp_http_client
        app_update
        esp_partition
        mbedtls
        json
        esp_lcd
        esp_driver_i2c
        esp_driver_gpio
        esp_timer
        lvgl__lvgl
        espressif__esp_lcd_touch
        espressif__esp_lcd_touch_gt911
)
