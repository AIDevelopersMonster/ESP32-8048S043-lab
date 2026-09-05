# Project: KONTAKTS / ESP32-8048S043 Lab
# Application: App 03 - Live Dashboard
# Programmer: Sol
# Engineer: Alex Malachevsky

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_lcd
        esp_driver_i2c
        esp_driver_gpio
        esp_driver_tsens
        esp_timer
        lvgl__lvgl
        espressif__esp_lcd_touch
        espressif__esp_lcd_touch_gt911
)
