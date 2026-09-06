# Project: KONTAKTS / ESP32-8048S043 Lab
# Application: App 04 - Persistent Storage / Partial Resource Loading

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        spiffs
)

spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)
