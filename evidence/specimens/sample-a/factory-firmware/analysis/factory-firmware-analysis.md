# Factory firmware first-pass analysis

Input: `evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin`

Size: 16777216 bytes / 0x1000000

SHA-256: `3007e5a223cd70dd9e53746c899ba25af24721c68f1cfc69ab8a8ce3d3e6eb4c`

## ESP image header candidates

| Offset | Segments | SPI mode | Size/freq | Entry |
|---:|---:|---:|---:|---:|
| 0x00000000 | 3 | 0x02 | 0x2F | 0x403B61D8 |
| 0x00010000 | 6 | 0x02 | 0x4F | 0x40376810 |

## Partition table candidates

### Candidate at 0x00008000

| # | Type | Subtype | Offset | Size | Label | Flags |
|---:|---|---|---:|---:|---|---:|
| 0 | data | nvs | 0x00009000 | 0x5000 | `nvs` | 0x00000000 |
| 1 | data | ota | 0x0000E000 | 0x2000 | `otadata` | 0x00000000 |
| 2 | app | ota_0 | 0x00010000 | 0x140000 | `app0` | 0x00000000 |
| 3 | app | ota_1 | 0x00150000 | 0x140000 | `app1` | 0x00000000 |
| 4 | data | spiffs | 0x00290000 | 0x170000 | `spiffs` | 0x00000000 |

## Keyword hits for possible factory tests

| Keyword | Offset | String |
|---|---:|---|
| `test` | 0x000315D8 | `xQueueGenericCreateStatic` |
| `test` | 0x00031B3E | `xTaskCreateStaticPinnedToCore` |
| `pass` | 0x0001034C | `Password` |
| `pass` | 0x0002F5C4 | `ESP_ERR_WIFI_PASSWORD` |
| `fail` | 0x00000048 | `Assert failed in %s, %s:%d (%s)` |
| `fail` | 0x00010132 | `LVGL disp_draw_buf allocate failed!` |
| `fail` | 0x0002C339 | `E (%u) %s: failed to get chip size` |
| `fail` | 0x0002C487 | `E (%u) %s: Detected size(%dk) smaller than the size in the binary image header(%dk). Probe failed.` |
| `fail` | 0x0002CD4D | `E (%u) %s: Failed to allocate task args!` |
| `fail` | 0x0002CD77 | `E (%u) %s: Failed to allocate pthread data!` |
| `fail` | 0x0002CDA4 | `E (%u) %s: Failed to create task!` |
| `fail` | 0x0002CDC7 | `false && "Failed to lock threads list!"` |
| `fail` | 0x0002CE30 | `false && "Failed to release mutex!"` |
| `fail` | 0x0002CE54 | `false && "Failed to unlock mutex!"` |
| `fail` | 0x0002CF1D | `Stack smashing protect failure!` |
| `fail` | 0x0002D44A | `vfs_err == ESP_OK && "Failed to register vfs console"` |
| `fail` | 0x0002D4B3 | `err == ESP_OK && "Failed to init pthread module!"` |
| `fail` | 0x0002EE62 | `E (%u) %s: SPI RAM enabled but initialization failed. Bailing out.` |
| `fail` | 0x0002EEF3 | `ESP_FAIL` |
| `fail` | 0x0002F0B4 | `ESP_ERR_NVS_REMOVE_FAILED` |
| `fail` | 0x0002F1A0 | `ESP_ERR_NVS_XTS_ENCR_FAILED` |
| `fail` | 0x0002F1BC | `ESP_ERR_NVS_XTS_DECR_FAILED` |
| `fail` | 0x0002F1D8 | `ESP_ERR_NVS_XTS_CFG_FAILED` |
| `fail` | 0x0002F397 | `ESP_ERR_OTA_VALIDATE_FAILED` |
| `fail` | 0x0002F3CD | `ESP_ERR_OTA_ROLLBACK_FAILED` |
| `fail` | 0x0002F4C0 | `ESP_ERR_IMAGE_FLASH_FAIL` |
| `fail` | 0x0002F5EF | `ESP_ERR_WIFI_WAKE_FAIL` |
| `fail` | 0x0002F7AC | `ESP_ERR_DPP_FAILURE` |
| `fail` | 0x0002F7C0 | `ESP_ERR_DPP_TX_FAILURE` |
| `fail` | 0x0002F909 | `ESP_ERR_MESH_QUEUE_FAIL` |
| `fail` | 0x0002FACC | `ESP_ERR_ESP_NETIF_DHCPC_START_FAILED` |
| `fail` | 0x0002FB7B | `ESP_ERR_ESP_NETIF_DRIVER_ATTACH_FAILED` |
| `fail` | 0x0002FBA2 | `ESP_ERR_ESP_NETIF_INIT_FAILED` |
| `fail` | 0x0002FBE5 | `ESP_ERR_ESP_NETIF_MLD6_FAILED` |
| `fail` | 0x0002FC03 | `ESP_ERR_ESP_NETIF_IP6_ADDR_FAILED` |
| `fail` | 0x0002FC38 | `ESP_ERR_FLASH_OP_FAIL` |
| `fail` | 0x0002FE46 | `ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST` |
| `fail` | 0x0002FE6D | `ESP_ERR_ESP_TLS_SOCKET_SETOPT_FAILED` |
| `fail` | 0x0002FEB5 | `ESP_ERR_ESP_TLS_SE_FAILED` |
| `fail` | 0x0002FF0D | `ESP_ERR_MBEDTLS_CTR_DRBG_SEED_FAILED` |
| `fail` | 0x0002FF32 | `ESP_ERR_MBEDTLS_SSL_SET_HOSTNAME_FAILED` |
| `fail` | 0x0002FF5A | `ESP_ERR_MBEDTLS_SSL_CONFIG_DEFAULTS_FAILED` |
| `fail` | 0x0002FF85 | `ESP_ERR_MBEDTLS_SSL_CONF_ALPN_PROTOCOLS_FAILED` |
| `fail` | 0x0002FFB4 | `ESP_ERR_MBEDTLS_X509_CRT_PARSE_FAILED` |
| `fail` | 0x0002FFDA | `ESP_ERR_MBEDTLS_SSL_CONF_OWN_CERT_FAILED` |
| `fail` | 0x00030003 | `ESP_ERR_MBEDTLS_SSL_SETUP_FAILED` |
| `fail` | 0x00030024 | `ESP_ERR_MBEDTLS_SSL_WRITE_FAILED` |
| `fail` | 0x00030045 | `ESP_ERR_MBEDTLS_PK_PARSE_KEY_FAILED` |
| `fail` | 0x00030069 | `ESP_ERR_MBEDTLS_SSL_HANDSHAKE_FAILED` |
| `fail` | 0x0003008E | `ESP_ERR_MBEDTLS_SSL_CONF_PSK_FAILED` |
| `fail` | 0x000300B2 | `ESP_ERR_MBEDTLS_SSL_TICKET_SETUP_FAILED` |
| `fail` | 0x000300DA | `ESP_ERR_WOLFSSL_SSL_SET_HOSTNAME_FAILED` |
| `fail` | 0x00030102 | `ESP_ERR_WOLFSSL_SSL_CONF_ALPN_PROTOCOLS_FAILED` |
| `fail` | 0x00030131 | `ESP_ERR_WOLFSSL_CERT_VERIFY_SETUP_FAILED` |
| `fail` | 0x0003015A | `ESP_ERR_WOLFSSL_KEY_VERIFY_SETUP_FAILED` |
| `fail` | 0x00030182 | `ESP_ERR_WOLFSSL_SSL_HANDSHAKE_FAILED` |
| `fail` | 0x000301A7 | `ESP_ERR_WOLFSSL_CTX_SETUP_FAILED` |
| `fail` | 0x000301C8 | `ESP_ERR_WOLFSSL_SSL_SETUP_FAILED` |
| `fail` | 0x000301E9 | `ESP_ERR_WOLFSSL_SSL_WRITE_FAILED` |
| `fail` | 0x00030373 | `ESP_ERR_HW_CRYPTO_DS_HMAC_FAIL` |
| `fail` | 0x00030BC5 | `E (%u) %s: esp_intr_alloc failed (0x%x)` |
| `fail` | 0x00030BEE | `E (%u) %s: esp_intr_enable failed (0x%x)` |
| `fail` | 0x00031EAD | `E (%u) %s: %s(%d): create done sem failed` |
| `fail` | 0x00031F2C | `E (%u) %s: %s(%d): install interrupt failed` |
| `fail` | 0x00031F59 | `E (%u) %s: %s(%d): alloc DMA channel failed` |
| `fail` | 0x00031F86 | `E (%u) %s: %s(%d): install DMA failed` |
| `fail` | 0x00031FAD | `E (%u) %s: %s(%d): configure GPIO failed` |
| `fail` | 0x00032254 | `phys_offs != SPI_FLASH_CACHE2PHYS_FAIL` |
| `fail` | 0x0003B412 | `%s failed: esp_err_t 0x%x` |
| `fail` | 0x0003BDB8 | `assert failed: ` |
| `fail` | 0x0003BFE3 | `E (%u) %s: configure host io mode failed - unsupported` |
| `lcd` | 0x0002A164 | `esp_lcd_new_rgb_panel(_panel_config, &_panel_handle)` |
| `lcd` | 0x0002A201 | `esp_lcd_panel_reset(_panel_handle)` |
| `lcd` | 0x0002A224 | `esp_lcd_panel_init(_panel_handle)` |
| `lcd` | 0x00031CA8 | `lcd_panel` |
| `lcd` | 0x00031CDB | `esp_lcd_panel_init` |
| `lcd` | 0x00031CEE | `esp_lcd_panel_reset` |
| `lcd` | 0x00031D5F | `/IDF/components/esp_lcd/src/esp_lcd_rgb_panel.c` |
| `lcd` | 0x00031D8F | `lcd_panel.rgb` |
| `lcd` | 0x00031EFC | `IDF/components/hal/esp32s3/include/hal/lcd_ll.h` |
| `lcd` | 0x00031FD7 | `lcd_rgb_panel_create_trans_link` |
| `lcd` | 0x00031FF7 | `lcd_ll_set_group_clock_src` |
| `lcd` | 0x00032037 | `esp_lcd_new_rgb_panel` |
| `rgb` | 0x0002A164 | `esp_lcd_new_rgb_panel(_panel_config, &_panel_handle)` |
| `rgb` | 0x0002A199 | `C:\Users\zhang'pei\Documents\Arduino\libraries\Arduino_GFX-master\src\databus\Arduino_ESP32RGBPanel.cpp` |
| `rgb` | 0x0002A284 | `uint16_t* Arduino_ESP32RGBPanel::getFrameBuffer(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, int32_t)` |
| `rgb` | 0x00031D5F | `/IDF/components/esp_lcd/src/esp_lcd_rgb_panel.c` |
| `rgb` | 0x00031D8F | `lcd_panel.rgb` |
| `rgb` | 0x00031E2D | `E (%u) %s: %s(%d): no mem for rgb panel` |
| `rgb` | 0x00031E56 | `E (%u) %s: %s(%d): no free rgb panel slot` |
| `rgb` | 0x00031FD7 | `lcd_rgb_panel_create_trans_link` |
| `rgb` | 0x00032012 | `rgb_panel_init` |
| `rgb` | 0x00032021 | `rgb_panel_draw_bitmap` |
| `rgb` | 0x00032037 | `esp_lcd_new_rgb_panel` |
| `i2c` | 0x0002A457 | `Bi2c_slave_task` |
| `i2c` | 0x0002B1CA | `E (%u) %s: i2c command link allocation error: the buffer provided is too small.` |
| `i2c` | 0x0002B21B | `E (%u) %s: i2c command link malloc error` |
| `i2c` | 0x0002B256 | `/IDF/components/driver/i2c.c` |
| `i2c` | 0x0002B289 | `E (%u) %s: %s(%d): i2c number error` |
| `i2c` | 0x0002B2AE | `E (%u) %s: %s(%d): i2c driver install error` |
| `i2c` | 0x0002B2DB | `E (%u) %s: %s(%d): i2c timing value error` |
| `i2c` | 0x0002B306 | `E (%u) %s: %s(%d): i2c null address error` |
| `i2c` | 0x0002B331 | `E (%u) %s: %s(%d): i2c buffer size too small for slave mode` |
| `i2c` | 0x0002B36E | `E (%u) %s: i2c driver malloc error` |
| `i2c` | 0x0002B392 | `E (%u) %s: i2c ringbuffer error` |
| `i2c` | 0x0002B3B3 | `E (%u) %s: i2c semaphore error` |
| `i2c` | 0x0002B3D3 | `E (%u) %s: i2c driver install error` |
| `i2c` | 0x0002B44C | `E (%u) %s: %s(%d): this i2c pin does not support internal pull-up` |
| `i2c` | 0x0002B4C9 | `E (%u) %s: %s(%d): i2c mode error` |
| `i2c` | 0x0002B4EC | `E (%u) %s: %s(%d): i2c clock choice is invalid, please check flag and frequency` |
| `i2c` | 0x0002B53D | `E (%u) %s: %s(%d): i2c command link error` |
| `i2c` | 0x0002B568 | `E (%u) %s: %s(%d): i2c ack type error` |
| `i2c` | 0x0002B58F | `E (%u) %s: %s(%d): i2c data read length error` |
| `i2c` | 0x0002B5BE | `E (%u) %s: %s(%d): i2c driver not installed` |
| `i2c` | 0x0002B649 | `i2c_master_cmd_begin` |
| `i2c` | 0x0002B65E | `i2c_master_read` |
| `i2c` | 0x0002B66E | `i2c_master_read_byte` |
| `i2c` | 0x0002B683 | `i2c_master_write_byte` |
| `i2c` | 0x0002B699 | `i2c_master_write` |
| `i2c` | 0x0002B6AA | `i2c_master_stop` |
| `i2c` | 0x0002B6BA | `i2c_cmd_link_append` |
| `i2c` | 0x0002B6CE | `i2c_master_start` |
| `i2c` | 0x0002B6DF | `i2c_set_pin` |
| `i2c` | 0x0002B6EB | `i2c_isr_register` |
| `i2c` | 0x0002B6FC | `i2c_set_timeout` |
| `i2c` | 0x0002B70C | `i2c_param_config` |
| `i2c` | 0x0002B71D | `i2c_reset_rx_fifo` |
| `i2c` | 0x0002B72F | `i2c_reset_tx_fifo` |
| `i2c` | 0x0002B741 | `i2c_driver_delete` |
| `i2c` | 0x0002B753 | `i2c_driver_install` |
| `sd` | 0x0002AED0 | `prvCheckItemFitsDefault` |
| `sd` | 0x0002B3F8 | `E (%u) %s: %s(%d): sda gpio number error` |
| `sd` | 0x0002B48F | `E (%u) %s: %s(%d): scl and sda gpio numbers are the same` |
| `tf` | 0x0002A284 | `uint16_t* Arduino_ESP32RGBPanel::getFrameBuffer(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, int32_t)` |
| `tf` | 0x0002A67F | `uartFlushTxOnly` |
| `tf` | 0x0002AEC1 | `prvGetFreeSize` |
| `tf` | 0x0002BDB0 | `s_platform.groups[group_id]` |
| `tf` | 0x0002DB41 | `InstFetchPrivilege` |
| `tf` | 0x00052FCA | `'rAtF` |
| `tf` | 0x000581B5 | `300tF` |
| `wifi` | 0x0002F4EF | `ESP_ERR_WIFI_BASE` |
| `wifi` | 0x0002F501 | `ESP_ERR_WIFI_NOT_INIT` |
| `wifi` | 0x0002F517 | `ESP_ERR_WIFI_NOT_STARTED` |
| `wifi` | 0x0002F530 | `ESP_ERR_WIFI_NOT_STOPPED` |
| `wifi` | 0x0002F549 | `ESP_ERR_WIFI_IF` |
| `wifi` | 0x0002F559 | `ESP_ERR_WIFI_MODE` |
| `wifi` | 0x0002F56B | `ESP_ERR_WIFI_STATE` |
| `wifi` | 0x0002F57E | `ESP_ERR_WIFI_CONN` |
| `wifi` | 0x0002F590 | `ESP_ERR_WIFI_NVS` |
| `wifi` | 0x0002F5A1 | `ESP_ERR_WIFI_MAC` |
| `wifi` | 0x0002F5B2 | `ESP_ERR_WIFI_SSID` |
| `wifi` | 0x0002F5C4 | `ESP_ERR_WIFI_PASSWORD` |
| `wifi` | 0x0002F5DA | `ESP_ERR_WIFI_TIMEOUT` |
| `wifi` | 0x0002F5EF | `ESP_ERR_WIFI_WAKE_FAIL` |
| `wifi` | 0x0002F606 | `ESP_ERR_WIFI_WOULD_BLOCK` |
| `wifi` | 0x0002F61F | `ESP_ERR_WIFI_NOT_CONNECT` |
| `wifi` | 0x0002F638 | `ESP_ERR_WIFI_POST` |
| `wifi` | 0x0002F64A | `ESP_ERR_WIFI_INIT_STATE` |
| `wifi` | 0x0002F662 | `ESP_ERR_WIFI_STOP_STATE` |
| `wifi` | 0x0002F67A | `ESP_ERR_WIFI_NOT_ASSOC` |
| `wifi` | 0x0002F691 | `ESP_ERR_WIFI_TX_DISALLOW` |
| `wifi` | 0x0002F6AA | `ESP_ERR_WIFI_REGISTRAR` |
| `wifi` | 0x0002F6C1 | `ESP_ERR_WIFI_WPS_TYPE` |
| `wifi` | 0x0002F6D7 | `ESP_ERR_WIFI_WPS_SM` |
| `wifi` | 0x0002F802 | `ESP_ERR_MESH_WIFI_NOT_START` |
| `ble` | 0x00010241 | `Tablet: %u` |
| `ble` | 0x0001043C | `Tablet: ` |
| `ble` | 0x0002B095 | `gpio_od_disable` |
| `ble` | 0x0002B0A5 | `gpio_od_enable` |
| `ble` | 0x0002B0B4 | `gpio_output_disable` |
| `ble` | 0x0002B0C8 | `gpio_output_enable` |
| `ble` | 0x0002B0DB | `gpio_input_disable` |
| `ble` | 0x0002B0EE | `gpio_input_enable` |
| `ble` | 0x0002B135 | `gpio_intr_disable` |
| `ble` | 0x0002B147 | `gpio_intr_enable_on_core` |
| `ble` | 0x0002B160 | `gpio_intr_enable` |
| `ble` | 0x0002B7BA | `periph_module_disable` |
| `ble` | 0x0002B7D0 | `periph_module_enable` |
| `ble` | 0x0002BA92 | `rtc_enabled & RTC_ENABLED(uart_num)` |
| `ble` | 0x0002BBD2 | `rtc_clk_disable` |
| `ble` | 0x0002BC35 | `uart_reenable_intr_mask` |
| `ble` | 0x0002BCC2 | `uart_enable_tx_intr` |
| `ble` | 0x0002BCEF | `uart_disable_intr_mask` |
| `ble` | 0x0002C0AB | `E (%u) %s: No MD5 found in partition table` |
| `ble` | 0x0002C0D7 | `E (%u) %s: Partition table MD5 mismatch` |
| `ble` | 0x0002C593 | `esp_task_stack_is_sane_cache_disabled()` |
| `ble` | 0x0002C6A4 | `spi_flash_enable_interrupts_caches_and_other_cpu` |
| `ble` | 0x0002C6D5 | `spi_flash_disable_interrupts_caches_and_other_cpu` |
| `ble` | 0x0002C72E | `/IDF/components/nvs_flash/src/compressed_enum_table.hpp` |
| `ble` | 0x0002C7AE | `void CompressedEnumTable<Tenum, Nbits, Nitems>::set(size_t, Tenum) [with Tenum = bool; unsigned int Nbits = 1; unsigned int Nitems = 256; size_t = unsigned int]` |
| `ble` | 0x0002CA22 | `void CompressedEnumTable<Tenum, Nbits, Nitems>::set(size_t, Tenum) [with Tenum = nvs::Page::EntryState; unsigned int Nbits = 2; unsigned int Nitems = 126; size_t = unsigned int]` |
| `ble` | 0x0002CB3A | `esp_err_t nvs::Page::mLoadEntryTable()` |
| `ble` | 0x0002CBC1 | `Tenum CompressedEnumTable<Tenum, Nbits, Nitems>::get(size_t) const [with Tenum = nvs::Page::EntryState; unsigned int Nbits = 2; unsigned int Nitems = 126; size_t = unsigned int]` |
| `ble` | 0x0002D19E | `E (%u) %s: Check that CONFIG_FREERTOS_UNICORE is enabled in menuconfig` |
| `ble` | 0x0002D996 | `Double exception` |
| `ble` | 0x0002DA14 | `Cache disabled but cached memory region accessed` |
| `ble` | 0x0002EDBB | `esp_intr_disable` |
| `ble` | 0x0002EDDA | `is_vect_desc_usable` |
| `ble` | 0x0002EE62 | `E (%u) %s: SPI RAM enabled but initialization failed. Bailing out.` |
| `ble` | 0x00030BEE | `E (%u) %s: esp_intr_enable failed (0x%x)` |
| `ble` | 0x000314CE | `uxQueueSpacesAvailable` |
| `ble` | 0x0003BB95 | `search_suitable_block` |
| `ble` | 0x0003BDA4 | `<cached disabled>` |
| `usb` | 0x0002DE78 | `/IDF/components/vfs/vfs_usb_serial_jtag.c` |
| `usb` | 0x0002DEA2 | `usb_serial_jtag_return_char` |
| `usb` | 0x00053AD7 | `USBQ9RQ6` |
| `gpio` | 0x0002AFD5 | `GPIO number error` |
| `gpio` | 0x0002AFE7 | `GPIO output gpio_num error` |
| `gpio` | 0x0002B002 | `GPIO interrupt type error` |
| `gpio` | 0x0002B01C | `GPIO pull mode error` |
| `gpio` | 0x0002B031 | `E (%u) %s: GPIO_PIN mask error ` |
| `gpio` | 0x0002B06B | `/IDF/components/driver/gpio.c` |
| `gpio` | 0x0002B089 | `gpio_config` |
| `gpio` | 0x0002B095 | `gpio_od_disable` |
| `gpio` | 0x0002B0A5 | `gpio_od_enable` |
| `gpio` | 0x0002B0B4 | `gpio_output_disable` |
| `gpio` | 0x0002B0C8 | `gpio_output_enable` |
| `gpio` | 0x0002B0DB | `gpio_input_disable` |
| `gpio` | 0x0002B0EE | `gpio_input_enable` |
| `gpio` | 0x0002B100 | `gpio_set_direction` |
| `gpio` | 0x0002B113 | `gpio_set_pull_mode` |
| `gpio` | 0x0002B126 | `gpio_set_level` |
| `gpio` | 0x0002B135 | `gpio_intr_disable` |
| `gpio` | 0x0002B147 | `gpio_intr_enable_on_core` |
| `gpio` | 0x0002B160 | `gpio_intr_enable` |
| `gpio` | 0x0002B171 | `gpio_set_intr_type` |
| `gpio` | 0x0002B184 | `gpio_pulldown_dis` |
| `gpio` | 0x0002B196 | `gpio_pulldown_en` |
| `gpio` | 0x0002B1A7 | `gpio_pullup_dis` |
| `gpio` | 0x0002B1B7 | `gpio_pullup_en` |
| `gpio` | 0x0002B3F8 | `E (%u) %s: %s(%d): sda gpio number error` |
| `gpio` | 0x0002B422 | `E (%u) %s: %s(%d): scl gpio number error` |
| `gpio` | 0x0002B48F | `E (%u) %s: %s(%d): scl and sda gpio numbers are the same` |
| `gpio` | 0x0002B812 | `rtc_gpio_deinit` |
| `gpio` | 0x00031FAD | `E (%u) %s: %s(%d): configure GPIO failed` |
| `psram` | 0x0002B61B | `E (%u) %s: Using buffer allocated from psram` |
| `psram` | 0x0002BFD8 | `E (%u) %s: %s(%d): invalid psram alignment: %zu` |
| `flash` | 0x0000009B | `/IDF/components/bootloader_support/src/bootloader_flash.c` |
| `flash` | 0x000000F7 | `bootloader_flash_read_sfdp` |
| `flash` | 0x00000112 | `bootloader_flash_execute_command_common` |
| `flash` | 0x0002C12A | `/IDF/components/spi_flash/partition.c` |
| `flash` | 0x0002C21B | `s_mmap_page_refcnt[i] == 0 \|\| (entry_pro == SOC_MMU_PAGE_IN_FLASH(pages[pageno]) )` |
| `flash` | 0x0002C26E | `/IDF/components/spi_flash/flash_mmap.c` |
| `flash` | 0x0002C2E1 | `spi_flash_munmap` |
| `flash` | 0x0002C2F2 | `spi_flash_mmap_pages` |
| `flash` | 0x0002C307 | `E (%u) %s: unexpected spi flash error code: 0x%x` |
| `flash` | 0x0002C36E | `/IDF/components/spi_flash/esp_flash_api.c` |
| `flash` | 0x0002C3AC | `E (%u) %s: flash encrypted write address must be 16 bytes aligned` |
| `flash` | 0x0002C3EF | `E (%u) %s: flash encrypted write length must be multiple of 16` |
| `flash` | 0x0002C43C | `esp_flash_write_encrypted` |
| `flash` | 0x0002C456 | `esp_flash_write` |
| `flash` | 0x0002C466 | `esp_flash_erase_region` |
| `flash` | 0x0002C47D | `spi_flash` |
| `flash` | 0x0002C4EB | `spi_flash` |
| `flash` | 0x0002C506 | `/IDF/components/spi_flash/spi_flash_os_func_app.c` |
| `flash` | 0x0002C538 | `spi1_flash_os_check_yield` |
| `flash` | 0x0002C552 | `s_flash_op_mutex != NULL` |
| `flash` | 0x0002C56B | `/IDF/components/spi_flash/cache_utils.c` |
| `flash` | 0x0002C5BB | `s_flash_op_cpu == -1` |
| `flash` | 0x0002C5E1 | `esp_ipc_call(other_cpuid, &spi_flash_op_block_func, (void *) other_cpuid)` |
| `flash` | 0x0002C645 | `cpuid == s_flash_op_cpu` |
| `flash` | 0x0002C6A4 | `spi_flash_enable_interrupts_caches_and_other_cpu` |
| `flash` | 0x0002C6D5 | `spi_flash_disable_interrupts_caches_and_other_cpu` |
| `flash` | 0x0002C707 | `spi_flash_init_lock` |
| `flash` | 0x0002C72E | `/IDF/components/nvs_flash/src/compressed_enum_table.hpp` |
| `flash` | 0x0002C782 | `/IDF/components/nvs_flash/src/nvs_types.hpp` |
| `flash` | 0x0002C8DC | `/IDF/components/nvs_flash/src/nvs_page.hpp` |
| `flash` | 0x0002C92A | `/IDF/components/nvs_flash/src/nvs_page.cpp` |
| `flash` | 0x0002CCCE | `IDF/components/nvs_flash/src/nvs_pagemanager.cpp` |
| `flash` | 0x0002D4E5 | `flash_ret == ESP_OK` |
| `flash` | 0x0002D680 | `Write back error occurred while dcache tries to write back to flash` |
| `flash` | 0x0002F4C0 | `ESP_ERR_IMAGE_FLASH_FAIL` |
| `flash` | 0x0002FC25 | `ESP_ERR_FLASH_BASE` |
| `flash` | 0x0002FC38 | `ESP_ERR_FLASH_OP_FAIL` |
| `flash` | 0x0002FC4E | `ESP_ERR_FLASH_OP_TIMEOUT` |
| `flash` | 0x0002FC67 | `ESP_ERR_FLASH_NOT_INITIALISED` |
| `flash` | 0x0002FC85 | `ESP_ERR_FLASH_UNSUPPORTED_HOST` |
| `flash` | 0x0002FCA4 | `ESP_ERR_FLASH_UNSUPPORTED_CHIP` |
| `flash` | 0x0002FCC3 | `ESP_ERR_FLASH_PROTECTED` |
| `flash` | 0x00032254 | `phys_offs != SPI_FLASH_CACHE2PHYS_FAIL` |
| `flash` | 0x000322DC | `/IDF/components/bootloader_support/src/bootloader_flash.c` |
| `flash` | 0x00032338 | `bootloader_flash_execute_command_common` |
| `flash` | 0x0003BDEA | `/IDF/components/spi_flash/memspi_host_driver.c` |
| `flash` | 0x0003C07A | `/IDF/components/spi_flash/spi_flash_chip_generic.c` |
| `flash` | 0x0003C0AD | `E (%u) %s: The flash you use doesn't support auto suspend, only 'XMC' is supported` |
| `flash` | 0x0003C101 | `spi_flash_chip_generic_get_write_protect` |
| `flash` | 0x0003C2C5 | `(io_mode == SPI_FLASH_OPI_STR) \|\| (io_mode == SPI_FLASH_OPI_DTR)` |
| `flash` | 0x0003C306 | `/IDF/components/spi_flash/spi_flash_chip_mxic_opi.c` |
| `flash` | 0x0003C36D | `spi_flash_chip_mxic_opi_get_write_protect` |
| `flash` | 0x0003C397 | `spi_flash_chip_mxic_opi_get_data_length_zoom` |
| `flash` | 0x0003C588 | `/IDF/components/spi_flash/esp32s3/spi_timing_config.c` |
| `flash` | 0x0003C5BE | `spi_timing_config_set_flash_clock` |
| `lvgl` | 0x00010120 | `LVGL Widgets Demo` |
| `lvgl` | 0x00010132 | `LVGL disp_draw_buf allocate failed!` |
| `lvgl` | 0x000103B5 | `LVGL v8` |
| `ota` | 0x0000802C | `otadata` |
| `ota` | 0x0002F347 | `ESP_ERR_OTA_BASE` |
| `ota` | 0x0002F358 | `ESP_ERR_OTA_PARTITION_CONFLICT` |
| `ota` | 0x0002F377 | `ESP_ERR_OTA_SELECT_INFO_INVALID` |
| `ota` | 0x0002F397 | `ESP_ERR_OTA_VALIDATE_FAILED` |

Truncated: 72 additional hits. See `strings.txt`.

## Entropy map, 64 KiB blocks

| Offset | Entropy | Note |
|---:|---:|---|
| 0x00000000 | 2.274 | mixed |
| 0x00010000 | 5.168 | mixed |
| 0x00020000 | 5.913 | mixed |
| 0x00030000 | 5.214 | mixed |
| 0x00040000 | 7.281 | mixed |
| 0x00050000 | 7.311 | mixed |
| 0x00060000 | 7.242 | mixed |
| 0x00070000 | 7.128 | mixed |
| 0x00080000 | 7.152 | mixed |
| 0x00090000 | 3.884 | mixed |
| 0x000A0000 | -0.000 | erased |
| 0x000B0000 | -0.000 | erased |
| 0x000C0000 | -0.000 | erased |
| 0x000D0000 | -0.000 | erased |
| 0x000E0000 | -0.000 | erased |
| 0x000F0000 | -0.000 | erased |
| 0x00100000 | -0.000 | erased |
| 0x00110000 | -0.000 | erased |
| 0x00120000 | -0.000 | erased |
| 0x00130000 | -0.000 | erased |
| 0x00140000 | -0.000 | erased |
| 0x00150000 | -0.000 | erased |
| 0x00160000 | -0.000 | erased |
| 0x00170000 | -0.000 | erased |
| 0x00180000 | -0.000 | erased |
| 0x00190000 | -0.000 | erased |
| 0x001A0000 | -0.000 | erased |
| 0x001B0000 | -0.000 | erased |
| 0x001C0000 | -0.000 | erased |
| 0x001D0000 | -0.000 | erased |
| 0x001E0000 | -0.000 | erased |
| 0x001F0000 | -0.000 | erased |
| 0x00200000 | -0.000 | erased |
| 0x00210000 | -0.000 | erased |
| 0x00220000 | -0.000 | erased |
| 0x00230000 | -0.000 | erased |
| 0x00240000 | -0.000 | erased |
| 0x00250000 | -0.000 | erased |
| 0x00260000 | -0.000 | erased |
| 0x00270000 | -0.000 | erased |
| 0x00280000 | -0.000 | erased |
| 0x00290000 | -0.000 | erased |
| 0x002A0000 | -0.000 | erased |
| 0x002B0000 | -0.000 | erased |
| 0x002C0000 | -0.000 | erased |
| 0x002D0000 | -0.000 | erased |
| 0x002E0000 | -0.000 | erased |
| 0x002F0000 | -0.000 | erased |
| 0x00300000 | -0.000 | erased |
| 0x00310000 | -0.000 | erased |
| 0x00320000 | -0.000 | erased |
| 0x00330000 | -0.000 | erased |
| 0x00340000 | -0.000 | erased |
| 0x00350000 | -0.000 | erased |
| 0x00360000 | -0.000 | erased |
| 0x00370000 | -0.000 | erased |
| 0x00380000 | -0.000 | erased |
| 0x00390000 | -0.000 | erased |
| 0x003A0000 | -0.000 | erased |
| 0x003B0000 | -0.000 | erased |
| 0x003C0000 | -0.000 | erased |
| 0x003D0000 | -0.000 | erased |
| 0x003E0000 | -0.000 | erased |
| 0x003F0000 | -0.000 | erased |
| 0x00400000 | -0.000 | erased |
| 0x00410000 | -0.000 | erased |
| 0x00420000 | -0.000 | erased |
| 0x00430000 | -0.000 | erased |
| 0x00440000 | -0.000 | erased |
| 0x00450000 | -0.000 | erased |
| 0x00460000 | -0.000 | erased |
| 0x00470000 | -0.000 | erased |
| 0x00480000 | -0.000 | erased |
| 0x00490000 | -0.000 | erased |
| 0x004A0000 | -0.000 | erased |
| 0x004B0000 | -0.000 | erased |
| 0x004C0000 | -0.000 | erased |
| 0x004D0000 | -0.000 | erased |
| 0x004E0000 | -0.000 | erased |
| 0x004F0000 | -0.000 | erased |
| 0x00500000 | -0.000 | erased |
| 0x00510000 | -0.000 | erased |
| 0x00520000 | -0.000 | erased |
| 0x00530000 | -0.000 | erased |
| 0x00540000 | -0.000 | erased |
| 0x00550000 | -0.000 | erased |
| 0x00560000 | -0.000 | erased |
| 0x00570000 | -0.000 | erased |
| 0x00580000 | -0.000 | erased |
| 0x00590000 | -0.000 | erased |
| 0x005A0000 | -0.000 | erased |
| 0x005B0000 | -0.000 | erased |
| 0x005C0000 | -0.000 | erased |
| 0x005D0000 | -0.000 | erased |
| 0x005E0000 | -0.000 | erased |
| 0x005F0000 | -0.000 | erased |
| 0x00600000 | -0.000 | erased |
| 0x00610000 | -0.000 | erased |
| 0x00620000 | -0.000 | erased |
| 0x00630000 | -0.000 | erased |
| 0x00640000 | -0.000 | erased |
| 0x00650000 | -0.000 | erased |
| 0x00660000 | -0.000 | erased |
| 0x00670000 | -0.000 | erased |
| 0x00680000 | -0.000 | erased |
| 0x00690000 | -0.000 | erased |
| 0x006A0000 | -0.000 | erased |
| 0x006B0000 | -0.000 | erased |
| 0x006C0000 | -0.000 | erased |
| 0x006D0000 | -0.000 | erased |
| 0x006E0000 | -0.000 | erased |
| 0x006F0000 | -0.000 | erased |
| 0x00700000 | -0.000 | erased |
| 0x00710000 | -0.000 | erased |
| 0x00720000 | -0.000 | erased |
| 0x00730000 | -0.000 | erased |
| 0x00740000 | -0.000 | erased |
| 0x00750000 | -0.000 | erased |
| 0x00760000 | -0.000 | erased |
| 0x00770000 | -0.000 | erased |
| 0x00780000 | -0.000 | erased |
| 0x00790000 | -0.000 | erased |
| 0x007A0000 | -0.000 | erased |
| 0x007B0000 | -0.000 | erased |
| 0x007C0000 | -0.000 | erased |
| 0x007D0000 | -0.000 | erased |
| 0x007E0000 | -0.000 | erased |
| 0x007F0000 | -0.000 | erased |
| 0x00800000 | -0.000 | erased |
| 0x00810000 | -0.000 | erased |
| 0x00820000 | -0.000 | erased |
| 0x00830000 | -0.000 | erased |
| 0x00840000 | -0.000 | erased |
| 0x00850000 | -0.000 | erased |
| 0x00860000 | -0.000 | erased |
| 0x00870000 | -0.000 | erased |
| 0x00880000 | -0.000 | erased |
| 0x00890000 | -0.000 | erased |
| 0x008A0000 | -0.000 | erased |
| 0x008B0000 | -0.000 | erased |
| 0x008C0000 | -0.000 | erased |
| 0x008D0000 | -0.000 | erased |
| 0x008E0000 | -0.000 | erased |
| 0x008F0000 | -0.000 | erased |
| 0x00900000 | -0.000 | erased |
| 0x00910000 | -0.000 | erased |
| 0x00920000 | -0.000 | erased |
| 0x00930000 | -0.000 | erased |
| 0x00940000 | -0.000 | erased |
| 0x00950000 | -0.000 | erased |
| 0x00960000 | -0.000 | erased |
| 0x00970000 | -0.000 | erased |
| 0x00980000 | -0.000 | erased |
| 0x00990000 | -0.000 | erased |
| 0x009A0000 | -0.000 | erased |
| 0x009B0000 | -0.000 | erased |
| 0x009C0000 | -0.000 | erased |
| 0x009D0000 | -0.000 | erased |
| 0x009E0000 | -0.000 | erased |
| 0x009F0000 | -0.000 | erased |
| 0x00A00000 | -0.000 | erased |
| 0x00A10000 | -0.000 | erased |
| 0x00A20000 | -0.000 | erased |
| 0x00A30000 | -0.000 | erased |
| 0x00A40000 | -0.000 | erased |
| 0x00A50000 | -0.000 | erased |
| 0x00A60000 | -0.000 | erased |
| 0x00A70000 | -0.000 | erased |
| 0x00A80000 | -0.000 | erased |
| 0x00A90000 | -0.000 | erased |
| 0x00AA0000 | -0.000 | erased |
| 0x00AB0000 | -0.000 | erased |
| 0x00AC0000 | -0.000 | erased |
| 0x00AD0000 | -0.000 | erased |
| 0x00AE0000 | -0.000 | erased |
| 0x00AF0000 | -0.000 | erased |
| 0x00B00000 | -0.000 | erased |
| 0x00B10000 | -0.000 | erased |
| 0x00B20000 | -0.000 | erased |
| 0x00B30000 | -0.000 | erased |
| 0x00B40000 | -0.000 | erased |
| 0x00B50000 | -0.000 | erased |
| 0x00B60000 | -0.000 | erased |
| 0x00B70000 | -0.000 | erased |
| 0x00B80000 | -0.000 | erased |
| 0x00B90000 | -0.000 | erased |
| 0x00BA0000 | -0.000 | erased |
| 0x00BB0000 | -0.000 | erased |
| 0x00BC0000 | -0.000 | erased |
| 0x00BD0000 | -0.000 | erased |
| 0x00BE0000 | -0.000 | erased |
| 0x00BF0000 | -0.000 | erased |
| 0x00C00000 | -0.000 | erased |
| 0x00C10000 | -0.000 | erased |
| 0x00C20000 | -0.000 | erased |
| 0x00C30000 | -0.000 | erased |
| 0x00C40000 | -0.000 | erased |
| 0x00C50000 | -0.000 | erased |
| 0x00C60000 | -0.000 | erased |
| 0x00C70000 | -0.000 | erased |
| 0x00C80000 | -0.000 | erased |
| 0x00C90000 | -0.000 | erased |
| 0x00CA0000 | -0.000 | erased |
| 0x00CB0000 | -0.000 | erased |
| 0x00CC0000 | -0.000 | erased |
| 0x00CD0000 | -0.000 | erased |
| 0x00CE0000 | -0.000 | erased |
| 0x00CF0000 | -0.000 | erased |
| 0x00D00000 | -0.000 | erased |
| 0x00D10000 | -0.000 | erased |
| 0x00D20000 | -0.000 | erased |
| 0x00D30000 | -0.000 | erased |
| 0x00D40000 | -0.000 | erased |
| 0x00D50000 | -0.000 | erased |
| 0x00D60000 | -0.000 | erased |
| 0x00D70000 | -0.000 | erased |
| 0x00D80000 | -0.000 | erased |
| 0x00D90000 | -0.000 | erased |
| 0x00DA0000 | -0.000 | erased |
| 0x00DB0000 | -0.000 | erased |
| 0x00DC0000 | -0.000 | erased |
| 0x00DD0000 | -0.000 | erased |
| 0x00DE0000 | -0.000 | erased |
| 0x00DF0000 | -0.000 | erased |
| 0x00E00000 | -0.000 | erased |
| 0x00E10000 | -0.000 | erased |
| 0x00E20000 | -0.000 | erased |
| 0x00E30000 | -0.000 | erased |
| 0x00E40000 | -0.000 | erased |
| 0x00E50000 | -0.000 | erased |
| 0x00E60000 | -0.000 | erased |
| 0x00E70000 | -0.000 | erased |
| 0x00E80000 | -0.000 | erased |
| 0x00E90000 | -0.000 | erased |
| 0x00EA0000 | -0.000 | erased |
| 0x00EB0000 | -0.000 | erased |
| 0x00EC0000 | -0.000 | erased |
| 0x00ED0000 | -0.000 | erased |
| 0x00EE0000 | -0.000 | erased |
| 0x00EF0000 | -0.000 | erased |
| 0x00F00000 | -0.000 | erased |
| 0x00F10000 | -0.000 | erased |
| 0x00F20000 | -0.000 | erased |
| 0x00F30000 | -0.000 | erased |
| 0x00F40000 | -0.000 | erased |
| 0x00F50000 | -0.000 | erased |
| 0x00F60000 | -0.000 | erased |
| 0x00F70000 | -0.000 | erased |
| 0x00F80000 | -0.000 | erased |
| 0x00F90000 | -0.000 | erased |
| 0x00FA0000 | -0.000 | erased |
| 0x00FB0000 | -0.000 | erased |
| 0x00FC0000 | -0.000 | erased |
| 0x00FD0000 | -0.000 | erased |
| 0x00FE0000 | -0.000 | erased |
| 0x00FF0000 | -0.000 | erased |

## Interpretation boundary

This is a first-pass static scan only. It may identify strings, partitions and likely test labels, but it does not prove runtime behavior. Factory-test claims require a named specimen, logs/video and a reproducible entry path.
