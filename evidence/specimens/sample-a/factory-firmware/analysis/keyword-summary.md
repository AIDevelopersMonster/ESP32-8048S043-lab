# Factory firmware keyword summary

This report is generated from the local `strings.txt` output. Raw strings are not published by default.

## High-confidence application leads

| Offset | String |
|---:|---|
| 0x00010120 | `LVGL Widgets Demo` |
| 0x000103B5 | `LVGL v8` |
| 0x0002A164 | `esp_lcd_new_rgb_panel(_panel_config, &_panel_handle)` |
| 0x0002A199 | `<WINDOWS_PATH>\Arduino_GFX-master\src\databus\Arduino_ESP32RGBPanel.cpp` |
| 0x0002A201 | `esp_lcd_panel_reset(_panel_handle)` |
| 0x0002A224 | `esp_lcd_panel_init(_panel_handle)` |
| 0x0002A284 | `uint16_t* Arduino_ESP32RGBPanel::getFrameBuffer(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, int32_t)` |
| 0x0002A4F5 | `<WINDOWS_PATH>\2.0.3\cores\esp32\esp32-hal-uart.c` |
| 0x00031CDB | `esp_lcd_panel_init` |
| 0x00031CEE | `esp_lcd_panel_reset` |
| 0x00032037 | `esp_lcd_new_rgb_panel` |

## Grouped keyword hits

### application_identity

Matches: `5`

| Offset | String |
|---:|---|
| 0x00010120 | `LVGL Widgets Demo` |
| 0x000103B5 | `LVGL v8` |
| 0x0002A199 | `<WINDOWS_PATH>\Arduino_GFX-master\src\databus\Arduino_ESP32RGBPanel.cpp` |
| 0x0002A284 | `uint16_t* Arduino_ESP32RGBPanel::getFrameBuffer(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, int32_t)` |
| 0x0002A4F5 | `<WINDOWS_PATH>\2.0.3\cores\esp32\esp32-hal-uart.c` |

### display_rgb_panel

Matches: `18`

| Offset | String |
|---:|---|
| 0x0002A164 | `esp_lcd_new_rgb_panel(_panel_config, &_panel_handle)` |
| 0x0002A199 | `<WINDOWS_PATH>\Arduino_GFX-master\src\databus\Arduino_ESP32RGBPanel.cpp` |
| 0x0002A201 | `esp_lcd_panel_reset(_panel_handle)` |
| 0x0002A224 | `esp_lcd_panel_init(_panel_handle)` |
| 0x0002A284 | `uint16_t* Arduino_ESP32RGBPanel::getFrameBuffer(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, int32_t)` |
| 0x00031CA8 | `lcd_panel` |
| 0x00031CDB | `esp_lcd_panel_init` |
| 0x00031CEE | `esp_lcd_panel_reset` |
| 0x00031D5F | `/IDF/components/esp_lcd/src/esp_lcd_rgb_panel.c` |
| 0x00031D8F | `lcd_panel.rgb` |
| 0x00031E2D | `E (%u) %s: %s(%d): no mem for rgb panel` |
| 0x00031E56 | `E (%u) %s: %s(%d): no free rgb panel slot` |
| 0x00031EFC | `IDF/components/hal/esp32s3/include/hal/lcd_ll.h` |
| 0x00031FD7 | `lcd_rgb_panel_create_trans_link` |
| 0x00031FF7 | `lcd_ll_set_group_clock_src` |
| 0x00032012 | `rgb_panel_init` |
| 0x00032021 | `rgb_panel_draw_bitmap` |
| 0x00032037 | `esp_lcd_new_rgb_panel` |

### touch_i2c

Matches: `39`

| Offset | String |
|---:|---|
| 0x0002A457 | `Bi2c_slave_task` |
| 0x0002B1CA | `E (%u) %s: i2c command link allocation error: the buffer provided is too small.` |
| 0x0002B21B | `E (%u) %s: i2c command link malloc error` |
| 0x0002B256 | `/IDF/components/driver/i2c.c` |
| 0x0002B289 | `E (%u) %s: %s(%d): i2c number error` |
| 0x0002B2AE | `E (%u) %s: %s(%d): i2c driver install error` |
| 0x0002B2DB | `E (%u) %s: %s(%d): i2c timing value error` |
| 0x0002B306 | `E (%u) %s: %s(%d): i2c null address error` |
| 0x0002B331 | `E (%u) %s: %s(%d): i2c buffer size too small for slave mode` |
| 0x0002B36E | `E (%u) %s: i2c driver malloc error` |
| 0x0002B392 | `E (%u) %s: i2c ringbuffer error` |
| 0x0002B3B3 | `E (%u) %s: i2c semaphore error` |
| 0x0002B3D3 | `E (%u) %s: i2c driver install error` |
| 0x0002B3F8 | `E (%u) %s: %s(%d): sda gpio number error` |
| 0x0002B422 | `E (%u) %s: %s(%d): scl gpio number error` |
| 0x0002B44C | `E (%u) %s: %s(%d): this i2c pin does not support internal pull-up` |
| 0x0002B48F | `E (%u) %s: %s(%d): scl and sda gpio numbers are the same` |
| 0x0002B4C9 | `E (%u) %s: %s(%d): i2c mode error` |
| 0x0002B4EC | `E (%u) %s: %s(%d): i2c clock choice is invalid, please check flag and frequency` |
| 0x0002B53D | `E (%u) %s: %s(%d): i2c command link error` |
| 0x0002B568 | `E (%u) %s: %s(%d): i2c ack type error` |
| 0x0002B58F | `E (%u) %s: %s(%d): i2c data read length error` |
| 0x0002B5BE | `E (%u) %s: %s(%d): i2c driver not installed` |
| 0x0002B649 | `i2c_master_cmd_begin` |
| 0x0002B65E | `i2c_master_read` |
| 0x0002B66E | `i2c_master_read_byte` |
| 0x0002B683 | `i2c_master_write_byte` |
| 0x0002B699 | `i2c_master_write` |
| 0x0002B6AA | `i2c_master_stop` |
| 0x0002B6BA | `i2c_cmd_link_append` |
| 0x0002B6CE | `i2c_master_start` |
| 0x0002B6DF | `i2c_set_pin` |
| 0x0002B6EB | `i2c_isr_register` |
| 0x0002B6FC | `i2c_set_timeout` |
| 0x0002B70C | `i2c_param_config` |
| 0x0002B71D | `i2c_reset_rx_fifo` |
| 0x0002B72F | `i2c_reset_tx_fifo` |
| 0x0002B741 | `i2c_driver_delete` |
| 0x0002B753 | `i2c_driver_install` |

### uart_usb

Matches: `56`

| Offset | String |
|---:|---|
| 0x0002A467 | `uart_event_task` |
| 0x0002A4D9 | `uart_flush_input(uart->num)` |
| 0x0002A4F5 | `<WINDOWS_PATH>\2.0.3\cores\esp32\esp32-hal-uart.c` |
| 0x0002A561 | `uart_driver_install(uart_nr, rx_buffer_size, tx_buffer_size, 20, &(uart->uart_event_queue), 0)` |
| 0x0002A5C0 | `uart_param_config(uart_nr, &uart_config)` |
| 0x0002A5E9 | `uart_set_pin(uart_nr, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)` |
| 0x0002A635 | `uart_set_line_inverse(uart_nr, UART_SIGNAL_TXD_INV \| UART_SIGNAL_RXD_INV)` |
| 0x0002A67F | `uartFlushTxOnly` |
| 0x0002A68F | `uartBegin` |
| 0x0002B822 | `E (%u) %s: %s(%d): uart_num error` |
| 0x0002B8DF | `E (%u) %s: %s(%d): uart driver error` |
| 0x0002BA05 | `E (%u) %s: %s(%d): uart size error` |
| 0x0002BA29 | `E (%u) %s: %s(%d): uart data null` |
| 0x0002BA92 | `rtc_enabled & RTC_ENABLED(uart_num)` |
| 0x0002BAB6 | `/IDF/components/driver/uart.c` |
| 0x0002BAD4 | `E (%u) %s: %s(%d): uart rx buffer length error` |
| 0x0002BB04 | `E (%u) %s: %s(%d): uart tx buffer length error` |
| 0x0002BB34 | `E (%u) %s: UART driver malloc error` |
| 0x0002BB59 | `E (%u) %s: UART driver already installed` |
| 0x0002BBBE | `uart_set_rx_timeout` |
| 0x0002BBE2 | `uart_driver_delete` |
| 0x0002BBF5 | `uart_driver_install` |
| 0x0002BC09 | `uart_flush_input` |
| 0x0002BC1A | `uart_get_buffered_data_len` |
| 0x0002BC35 | `uart_reenable_intr_mask` |
| 0x0002BC4D | `uart_read_bytes` |
| 0x0002BC5D | `uart_write_bytes` |
| 0x0002BC6E | `uart_wait_tx_done` |
| 0x0002BC80 | `uart_intr_config` |
| 0x0002BC91 | `uart_param_config` |
| 0x0002BCA3 | `uart_set_pin` |
| 0x0002BCB0 | `uart_isr_register` |
| 0x0002BCC2 | `uart_enable_tx_intr` |
| 0x0002BCD6 | `uart_pattern_queue_reset` |
| 0x0002BCEF | `uart_disable_intr_mask` |
| 0x0002BD06 | `uart_set_line_inverse` |
| 0x0002BD1C | `uart_get_baudrate` |
| 0x0002BD2E | `uart_set_baudrate` |
| 0x0002BD40 | `uart_get_parity` |
| 0x0002BD50 | `uart_set_parity` |
| 0x0002BD60 | `uart_get_stop_bits` |
| 0x0002BD73 | `uart_set_stop_bits` |
| 0x0002BD86 | `uart_get_word_length` |
| 0x0002BD9B | `uart_set_word_length` |
| 0x0002DD94 | `/dev/uart` |
| 0x0002DE78 | `/IDF/components/vfs/vfs_usb_serial_jtag.c` |
| 0x0002DEA2 | `usb_serial_jtag_return_char` |
| 0x0002DF77 | `/IDF/components/vfs/vfs_uart.c` |
| 0x0002DFC5 | `uart_write` |
| 0x0002DFD0 | `uart_return_char` |
| 0x0002DFE1 | `uart_read` |
| 0x0002DFEB | `uart_close` |
| 0x0002DFF6 | `uart_fstat` |
| 0x0002E001 | `uart_fcntl` |
| 0x0002E00C | `uart_fsync` |
| 0x00053AD7 | `USBQ9RQ6` |

### gpio_backlight_pwm

Matches: `29`

| Offset | String |
|---:|---|
| 0x0002AFD5 | `GPIO number error` |
| 0x0002AFE7 | `GPIO output gpio_num error` |
| 0x0002B002 | `GPIO interrupt type error` |
| 0x0002B01C | `GPIO pull mode error` |
| 0x0002B031 | `E (%u) %s: GPIO_PIN mask error ` |
| 0x0002B06B | `/IDF/components/driver/gpio.c` |
| 0x0002B089 | `gpio_config` |
| 0x0002B095 | `gpio_od_disable` |
| 0x0002B0A5 | `gpio_od_enable` |
| 0x0002B0B4 | `gpio_output_disable` |
| 0x0002B0C8 | `gpio_output_enable` |
| 0x0002B0DB | `gpio_input_disable` |
| 0x0002B0EE | `gpio_input_enable` |
| 0x0002B100 | `gpio_set_direction` |
| 0x0002B113 | `gpio_set_pull_mode` |
| 0x0002B126 | `gpio_set_level` |
| 0x0002B135 | `gpio_intr_disable` |
| 0x0002B147 | `gpio_intr_enable_on_core` |
| 0x0002B160 | `gpio_intr_enable` |
| 0x0002B171 | `gpio_set_intr_type` |
| 0x0002B184 | `gpio_pulldown_dis` |
| 0x0002B196 | `gpio_pulldown_en` |
| 0x0002B1A7 | `gpio_pullup_dis` |
| 0x0002B1B7 | `gpio_pullup_en` |
| 0x0002B3F8 | `E (%u) %s: %s(%d): sda gpio number error` |
| 0x0002B422 | `E (%u) %s: %s(%d): scl gpio number error` |
| 0x0002B48F | `E (%u) %s: %s(%d): scl and sda gpio numbers are the same` |
| 0x0002B812 | `rtc_gpio_deinit` |
| 0x00031FAD | `E (%u) %s: %s(%d): configure GPIO failed` |

### ota_storage_fs

Matches: `59`

| Offset | String |
|---:|---|
| 0x0000802C | `otadata` |
| 0x0000808C | `spiffs` |
| 0x0002AED0 | `prvCheckItemFitsDefault` |
| 0x0002B3F8 | `E (%u) %s: %s(%d): sda gpio number error` |
| 0x0002B48F | `E (%u) %s: %s(%d): scl and sda gpio numbers are the same` |
| 0x0002C72E | `/IDF/components/nvs_flash/src/compressed_enum_table.hpp` |
| 0x0002C782 | `/IDF/components/nvs_flash/src/nvs_types.hpp` |
| 0x0002C84F | `void nvs::Item::getValue(T&) [with T = unsigned char]` |
| 0x0002C8DC | `/IDF/components/nvs_flash/src/nvs_page.hpp` |
| 0x0002C92A | `/IDF/components/nvs_flash/src/nvs_page.cpp` |
| 0x0002C9D1 | `esp_err_t nvs::Page::alterEntryRangeState(size_t, size_t, nvs::Page::EntryState)` |
| 0x0002CA22 | `void CompressedEnumTable<Tenum, Nbits, Nitems>::set(size_t, Tenum) [with Tenum = nvs::Page::EntryState; unsigned int Nbits = 2; unsigned int Nitems = 126; size_t = unsigned int]` |
| 0x0002CAD4 | `esp_err_t nvs::Page::alterEntryState(size_t, nvs::Page::EntryState)` |
| 0x0002CB18 | `esp_err_t nvs::Page::initialize()` |
| 0x0002CB3A | `esp_err_t nvs::Page::mLoadEntryTable()` |
| 0x0002CB61 | `esp_err_t nvs::Page::copyItems(nvs::Page&)` |
| 0x0002CB8C | `void nvs::Page::updateFirstUsedEntry(size_t, size_t)` |
| 0x0002CBC1 | `Tenum CompressedEnumTable<Tenum, Nbits, Nitems>::get(size_t) const [with Tenum = nvs::Page::EntryState; unsigned int Nbits = 2; unsigned int Nitems = 126; size_t = unsigned int]` |
| 0x0002CC73 | `uint32_t nvs::Page::getEntryAddress(size_t) const` |
| 0x0002CCCE | `IDF/components/nvs_flash/src/nvs_pagemanager.cpp` |
| 0x0002CCFF | `esp_err_t nvs::PageManager::load(nvs::Partition*, uint32_t, uint32_t)` |
| 0x0002EFF0 | `ESP_ERR_NVS_BASE` |
| 0x0002F001 | `ESP_ERR_NVS_NOT_INITIALIZED` |
| 0x0002F01D | `ESP_ERR_NVS_NOT_FOUND` |
| 0x0002F033 | `ESP_ERR_NVS_TYPE_MISMATCH` |
| 0x0002F04D | `ESP_ERR_NVS_READ_ONLY` |
| 0x0002F063 | `ESP_ERR_NVS_NOT_ENOUGH_SPACE` |
| 0x0002F080 | `ESP_ERR_NVS_INVALID_NAME` |
| 0x0002F099 | `ESP_ERR_NVS_INVALID_HANDLE` |
| 0x0002F0B4 | `ESP_ERR_NVS_REMOVE_FAILED` |
| 0x0002F0CE | `ESP_ERR_NVS_KEY_TOO_LONG` |
| 0x0002F0E7 | `ESP_ERR_NVS_PAGE_FULL` |
| 0x0002F0FD | `ESP_ERR_NVS_INVALID_STATE` |
| 0x0002F117 | `ESP_ERR_NVS_INVALID_LENGTH` |
| 0x0002F132 | `ESP_ERR_NVS_NO_FREE_PAGES` |
| 0x0002F14C | `ESP_ERR_NVS_VALUE_TOO_LONG` |
| 0x0002F167 | `ESP_ERR_NVS_PART_NOT_FOUND` |
| 0x0002F182 | `ESP_ERR_NVS_NEW_VERSION_FOUND` |
| 0x0002F1A0 | `ESP_ERR_NVS_XTS_ENCR_FAILED` |
| 0x0002F1BC | `ESP_ERR_NVS_XTS_DECR_FAILED` |
| 0x0002F1D8 | `ESP_ERR_NVS_XTS_CFG_FAILED` |
| 0x0002F1F3 | `ESP_ERR_NVS_XTS_CFG_NOT_FOUND` |
| 0x0002F211 | `ESP_ERR_NVS_ENCR_NOT_SUPPORTED` |
| 0x0002F230 | `ESP_ERR_NVS_KEYS_NOT_INITIALIZED` |
| 0x0002F251 | `ESP_ERR_NVS_CORRUPT_KEY_PART` |
| 0x0002F26E | `ESP_ERR_NVS_CONTENT_DIFFERS` |
| 0x0002F28A | `ESP_ERR_NVS_WRONG_ENCRYPTION` |
| 0x0002F347 | `ESP_ERR_OTA_BASE` |
| 0x0002F358 | `ESP_ERR_OTA_PARTITION_CONFLICT` |
| 0x0002F377 | `ESP_ERR_OTA_SELECT_INFO_INVALID` |
| 0x0002F397 | `ESP_ERR_OTA_VALIDATE_FAILED` |
| 0x0002F3B3 | `ESP_ERR_OTA_SMALL_SEC_VER` |
| 0x0002F3CD | `ESP_ERR_OTA_ROLLBACK_FAILED` |
| 0x0002F3E9 | `ESP_ERR_OTA_ROLLBACK_INVALID_STATE` |
| 0x0002F590 | `ESP_ERR_WIFI_NVS` |
| 0x0003020A | `ESP_ERR_HTTPS_OTA_BASE` |
| 0x00030221 | `ESP_ERR_HTTPS_OTA_IN_PROGRESS` |
| 0x0003227B | `/IDF/components/app_update/esp_ota_ops.c` |
| 0x000322AF | `esp_ota_get_running_partition` |

### factory_test_words

Matches: `69`

| Offset | String |
|---:|---|
| 0x00000048 | `Assert failed in %s, %s:%d (%s)` |
| 0x00010132 | `LVGL disp_draw_buf allocate failed!` |
| 0x0001034C | `Password` |
| 0x0002C339 | `E (%u) %s: failed to get chip size` |
| 0x0002C487 | `E (%u) %s: Detected size(%dk) smaller than the size in the binary image header(%dk). Probe failed.` |
| 0x0002CD4D | `E (%u) %s: Failed to allocate task args!` |
| 0x0002CD77 | `E (%u) %s: Failed to allocate pthread data!` |
| 0x0002CDA4 | `E (%u) %s: Failed to create task!` |
| 0x0002CDC7 | `false && "Failed to lock threads list!"` |
| 0x0002CE30 | `false && "Failed to release mutex!"` |
| 0x0002CE54 | `false && "Failed to unlock mutex!"` |
| 0x0002CF1D | `Stack smashing protect failure!` |
| 0x0002D44A | `vfs_err == ESP_OK && "Failed to register vfs console"` |
| 0x0002D4B3 | `err == ESP_OK && "Failed to init pthread module!"` |
| 0x0002EE62 | `E (%u) %s: SPI RAM enabled but initialization failed. Bailing out.` |
| 0x0002EEF3 | `ESP_FAIL` |
| 0x0002F0B4 | `ESP_ERR_NVS_REMOVE_FAILED` |
| 0x0002F1A0 | `ESP_ERR_NVS_XTS_ENCR_FAILED` |
| 0x0002F1BC | `ESP_ERR_NVS_XTS_DECR_FAILED` |
| 0x0002F1D8 | `ESP_ERR_NVS_XTS_CFG_FAILED` |
| 0x0002F397 | `ESP_ERR_OTA_VALIDATE_FAILED` |
| 0x0002F3CD | `ESP_ERR_OTA_ROLLBACK_FAILED` |
| 0x0002F4C0 | `ESP_ERR_IMAGE_FLASH_FAIL` |
| 0x0002F5C4 | `ESP_ERR_WIFI_PASSWORD` |
| 0x0002F5EF | `ESP_ERR_WIFI_WAKE_FAIL` |
| 0x0002F7AC | `ESP_ERR_DPP_FAILURE` |
| 0x0002F7C0 | `ESP_ERR_DPP_TX_FAILURE` |
| 0x0002F909 | `ESP_ERR_MESH_QUEUE_FAIL` |
| 0x0002FACC | `ESP_ERR_ESP_NETIF_DHCPC_START_FAILED` |
| 0x0002FB7B | `ESP_ERR_ESP_NETIF_DRIVER_ATTACH_FAILED` |
| 0x0002FBA2 | `ESP_ERR_ESP_NETIF_INIT_FAILED` |
| 0x0002FBE5 | `ESP_ERR_ESP_NETIF_MLD6_FAILED` |
| 0x0002FC03 | `ESP_ERR_ESP_NETIF_IP6_ADDR_FAILED` |
| 0x0002FC38 | `ESP_ERR_FLASH_OP_FAIL` |
| 0x0002FE46 | `ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST` |
| 0x0002FE6D | `ESP_ERR_ESP_TLS_SOCKET_SETOPT_FAILED` |
| 0x0002FEB5 | `ESP_ERR_ESP_TLS_SE_FAILED` |
| 0x0002FF0D | `ESP_ERR_MBEDTLS_CTR_DRBG_SEED_FAILED` |
| 0x0002FF32 | `ESP_ERR_MBEDTLS_SSL_SET_HOSTNAME_FAILED` |
| 0x0002FF5A | `ESP_ERR_MBEDTLS_SSL_CONFIG_DEFAULTS_FAILED` |
| 0x0002FF85 | `ESP_ERR_MBEDTLS_SSL_CONF_ALPN_PROTOCOLS_FAILED` |
| 0x0002FFB4 | `ESP_ERR_MBEDTLS_X509_CRT_PARSE_FAILED` |
| 0x0002FFDA | `ESP_ERR_MBEDTLS_SSL_CONF_OWN_CERT_FAILED` |
| 0x00030003 | `ESP_ERR_MBEDTLS_SSL_SETUP_FAILED` |
| 0x00030024 | `ESP_ERR_MBEDTLS_SSL_WRITE_FAILED` |
| 0x00030045 | `ESP_ERR_MBEDTLS_PK_PARSE_KEY_FAILED` |
| 0x00030069 | `ESP_ERR_MBEDTLS_SSL_HANDSHAKE_FAILED` |
| 0x0003008E | `ESP_ERR_MBEDTLS_SSL_CONF_PSK_FAILED` |
| 0x000300B2 | `ESP_ERR_MBEDTLS_SSL_TICKET_SETUP_FAILED` |
| 0x000300DA | `ESP_ERR_WOLFSSL_SSL_SET_HOSTNAME_FAILED` |
| 0x00030102 | `ESP_ERR_WOLFSSL_SSL_CONF_ALPN_PROTOCOLS_FAILED` |
| 0x00030131 | `ESP_ERR_WOLFSSL_CERT_VERIFY_SETUP_FAILED` |
| 0x0003015A | `ESP_ERR_WOLFSSL_KEY_VERIFY_SETUP_FAILED` |
| 0x00030182 | `ESP_ERR_WOLFSSL_SSL_HANDSHAKE_FAILED` |
| 0x000301A7 | `ESP_ERR_WOLFSSL_CTX_SETUP_FAILED` |
| 0x000301C8 | `ESP_ERR_WOLFSSL_SSL_SETUP_FAILED` |
| 0x000301E9 | `ESP_ERR_WOLFSSL_SSL_WRITE_FAILED` |
| 0x00030373 | `ESP_ERR_HW_CRYPTO_DS_HMAC_FAIL` |
| 0x00030BC5 | `E (%u) %s: esp_intr_alloc failed (0x%x)` |
| 0x00030BEE | `E (%u) %s: esp_intr_enable failed (0x%x)` |
| 0x00031EAD | `E (%u) %s: %s(%d): create done sem failed` |
| 0x00031F2C | `E (%u) %s: %s(%d): install interrupt failed` |
| 0x00031F59 | `E (%u) %s: %s(%d): alloc DMA channel failed` |
| 0x00031F86 | `E (%u) %s: %s(%d): install DMA failed` |
| 0x00031FAD | `E (%u) %s: %s(%d): configure GPIO failed` |
| 0x00032254 | `phys_offs != SPI_FLASH_CACHE2PHYS_FAIL` |
| 0x0003B412 | `%s failed: esp_err_t 0x%x` |
| 0x0003BDB8 | `assert failed: ` |
| 0x0003BFE3 | `E (%u) %s: configure host io mode failed - unsupported` |

## Interpretation boundary

String hits are static evidence only. They can identify likely frameworks, libraries and app labels, but they do not prove runtime behavior, pin mapping, touch controller identity or a factory-test entry path.

Raw paths and host/user-specific fragments are sanitized before publishing. Review the output before committing it.
