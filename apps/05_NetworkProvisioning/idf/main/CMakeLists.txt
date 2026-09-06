idf_component_register(
    SRCS
        "main.c"
        "storage_credentials.c"
        "network_manager.c"
        "web_setup.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        esp_http_server
)
