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
    "#include \"sd_manager.h\"\n#include \"sd_launcher.h\""
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
    "static void sd_dashboard_cb(lv_event_t *e)\n{\n    if (lv_event_get_code(e) == LV_EVENT_CLICKED)\n        sd_run_widget(\"widgets/youtube/dashboard.json\", \"Dashboard\");\n}\nstatic void sd_views_cb(lv_event_t *e)\n{\n    if (lv_event_get_code(e) == LV_EVENT_CLICKED)\n        sd_run_widget(\"widgets/youtube/views.json\", \"Views\");\n}\nstatic void sd_subscribers_cb(lv_event_t *e)\n{\n    if (lv_event_get_code(e) == LV_EVENT_CLICKED)\n        sd_run_widget(\"widgets/youtube/subscribers.json\", \"Subscribers\");\n}\n"
    "static void sd_catalog_run_cb(const sd_manager_entry_t *entry, void *user_data)\n{\n    (void)user_data;\n    if (!entry) return;\n    sd_run_widget(entry->relative_path, entry->entry_name);\n}\n"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "    s_sd_mount_button = make_button(s_sd_panel, 24, 238, 214, 52, \"MOUNT / RESCAN\", sd_mount_cb);\n    s_sd_dashboard_button = make_button(s_sd_panel, 252, 238, 214, 52, \"DASHBOARD\", sd_dashboard_cb);\n    s_sd_views_button = make_button(s_sd_panel, 480, 238, 214, 52, \"VIEWS\", sd_views_cb);\n    s_sd_subscribers_button = make_button(s_sd_panel, 24, 304, 214, 52, \"SUBSCRIBERS\", sd_subscribers_cb);\n    make_label(s_sd_panel, 252, 314, \"SD UPDATE: manifest contract only; flash writes disabled in this test.\", &lv_font_montserrat_18, 0x8B949E);"
    "    s_sd_mount_button = make_button(s_sd_panel, 24, 238, 214, 52, \"MOUNT / RESCAN\", sd_mount_cb);\n    s_sd_launcher = sd_launcher_create(s_sd_panel, 252, 230, 442, 142, sd_catalog_run_cb, NULL);\n    make_label(s_sd_panel, 24, 304, \"Applications are discovered from package.json. Add apps by copying folders to /widgets.\", &lv_font_montserrat_18, 0x8B949E);"
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
    "            s_sd_packages_scanned = true;"
    "            if (s_sd_launcher) sd_launcher_refresh(s_sd_launcher);\n            s_sd_packages_scanned = true;"
    DISPLAY_OTA_TEXT
    "${DISPLAY_OTA_TEXT}"
)
string(REPLACE
    "        set_button_enabled(s_sd_dashboard_button, false);\n        set_button_enabled(s_sd_views_button, false);\n        set_button_enabled(s_sd_subscribers_button, false);\n"
    ""
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
