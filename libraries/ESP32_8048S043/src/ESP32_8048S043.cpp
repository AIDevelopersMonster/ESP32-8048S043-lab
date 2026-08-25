#include "ESP32_8048S043.h"

#include <esp_heap_caps.h>
#include <esp_ota_ops.h>
#include <esp_system.h>

namespace {

#define ESP32_8048S043_STRINGIFY_IMPL(x) #x
#define ESP32_8048S043_STRINGIFY(x) ESP32_8048S043_STRINGIFY_IMPL(x)

const char *resetReasonName(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON: return "POWERON";
        case ESP_RST_EXT: return "EXT";
        case ESP_RST_SW: return "SW";
        case ESP_RST_PANIC: return "PANIC";
        case ESP_RST_INT_WDT: return "INT_WDT";
        case ESP_RST_TASK_WDT: return "TASK_WDT";
        case ESP_RST_WDT: return "WDT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_BROWNOUT: return "BROWNOUT";
        case ESP_RST_SDIO: return "SDIO";
        default: return "UNKNOWN";
    }
}

const char *flashModeName(FlashMode_t mode) {
    switch (mode) {
        case FM_QIO: return "QIO";
        case FM_QOUT: return "QOUT";
        case FM_DIO: return "DIO";
        case FM_DOUT: return "DOUT";
        case FM_FAST_READ: return "FAST_READ";
        case FM_SLOW_READ: return "SLOW_READ";
        default: return "UNKNOWN";
    }
}

const char *passFail(bool value) {
    return value ? "PASS" : "FAIL";
}

const char *definedText(bool value) {
    return value ? "defined" : "not defined";
}

void printSize(Stream &out, const char *label, uint32_t bytes) {
    out.printf("%-28s: %lu bytes / %lu KB / %lu MB\n",
               label,
               static_cast<unsigned long>(bytes),
               static_cast<unsigned long>(bytes / 1024UL),
               static_cast<unsigned long>(bytes / (1024UL * 1024UL)));
}

void printEfuseMac(Stream &out) {
    const uint64_t mac = ESP.getEfuseMac();
    const uint32_t macLow = static_cast<uint32_t>(mac);
    const uint16_t macHigh = static_cast<uint16_t>(mac >> 32);

    out.printf("%-28s: 0x%04X%08lX\n",
               "eFuse MAC raw",
               macHigh,
               static_cast<unsigned long>(macLow));

    out.printf("%-28s: %02X:%02X:%02X:%02X:%02X:%02X\n",
               "eFuse MAC bytes",
               static_cast<unsigned int>((mac >> 40) & 0xFF),
               static_cast<unsigned int>((mac >> 32) & 0xFF),
               static_cast<unsigned int>((mac >> 24) & 0xFF),
               static_cast<unsigned int>((mac >> 16) & 0xFF),
               static_cast<unsigned int>((mac >> 8) & 0xFF),
               static_cast<unsigned int>(mac & 0xFF));
}

void printPartition(Stream &out, const char *title, const esp_partition_t *part) {
    if (!part) {
        out.printf("%-28s: not available\n", title);
        return;
    }

    out.printf("%-28s: label=%s type=0x%02X subtype=0x%02X address=0x%06lX size=%lu\n",
               title,
               part->label,
               static_cast<unsigned int>(part->type),
               static_cast<unsigned int>(part->subtype),
               static_cast<unsigned long>(part->address),
               static_cast<unsigned long>(part->size));
}

void printDivider(Stream &out) {
    out.println("------------------------------------------------------------");
}

void printMacroText(Stream &out, const char *label, const char *value) {
    out.printf("%-28s: %s\n", label, value);
}

void printBuildProfile(Stream &out) {
    out.println("[ARDUINO BUILD PROFILE]");

#ifdef ARDUINO_BOARD
    printMacroText(out, "ARDUINO_BOARD", ESP32_8048S043_STRINGIFY(ARDUINO_BOARD));
#else
    printMacroText(out, "ARDUINO_BOARD", "not defined");
#endif

#ifdef ARDUINO_VARIANT
    printMacroText(out, "ARDUINO_VARIANT", ESP32_8048S043_STRINGIFY(ARDUINO_VARIANT));
#else
    printMacroText(out, "ARDUINO_VARIANT", "not defined");
#endif

#ifdef ARDUINO_ARCH_ESP32
    printMacroText(out, "ARDUINO_ARCH_ESP32", "defined");
#else
    printMacroText(out, "ARDUINO_ARCH_ESP32", "not defined");
#endif

#ifdef ESP_ARDUINO_VERSION_MAJOR
    out.printf("%-28s: %d.%d.%d\n",
               "Arduino-ESP32 version",
               ESP_ARDUINO_VERSION_MAJOR,
               ESP_ARDUINO_VERSION_MINOR,
               ESP_ARDUINO_VERSION_PATCH);
#else
    printMacroText(out, "Arduino-ESP32 version", "not exposed by macros");
#endif

#ifdef CONFIG_IDF_TARGET
    printMacroText(out, "CONFIG_IDF_TARGET", ESP32_8048S043_STRINGIFY(CONFIG_IDF_TARGET));
#else
    printMacroText(out, "CONFIG_IDF_TARGET", "not defined");
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3
    printMacroText(out, "CONFIG_IDF_TARGET_ESP32S3", ESP32_8048S043_STRINGIFY(CONFIG_IDF_TARGET_ESP32S3));
#else
    printMacroText(out, "CONFIG_IDF_TARGET_ESP32S3", "not defined");
#endif

#ifdef ARDUINO_USB_MODE
    printMacroText(out, "ARDUINO_USB_MODE", ESP32_8048S043_STRINGIFY(ARDUINO_USB_MODE));
#else
    printMacroText(out, "ARDUINO_USB_MODE", "not defined");
#endif

#ifdef ARDUINO_USB_CDC_ON_BOOT
    printMacroText(out, "ARDUINO_USB_CDC_ON_BOOT", ESP32_8048S043_STRINGIFY(ARDUINO_USB_CDC_ON_BOOT));
#else
    printMacroText(out, "ARDUINO_USB_CDC_ON_BOOT", "not defined");
#endif

#ifdef BOARD_HAS_PSRAM
    printMacroText(out, "BOARD_HAS_PSRAM", "defined");
#else
    printMacroText(out, "BOARD_HAS_PSRAM", "not defined");
#endif

#ifdef CONFIG_SPIRAM
    printMacroText(out, "CONFIG_SPIRAM", ESP32_8048S043_STRINGIFY(CONFIG_SPIRAM));
#else
    printMacroText(out, "CONFIG_SPIRAM", "not defined");
#endif

#ifdef CONFIG_SPIRAM_BOOT_INIT
    printMacroText(out, "CONFIG_SPIRAM_BOOT_INIT", ESP32_8048S043_STRINGIFY(CONFIG_SPIRAM_BOOT_INIT));
#else
    printMacroText(out, "CONFIG_SPIRAM_BOOT_INIT", "not defined");
#endif

#ifdef CONFIG_SPIRAM_USE_MALLOC
    printMacroText(out, "CONFIG_SPIRAM_USE_MALLOC", ESP32_8048S043_STRINGIFY(CONFIG_SPIRAM_USE_MALLOC));
#else
    printMacroText(out, "CONFIG_SPIRAM_USE_MALLOC", "not defined");
#endif

#ifdef CONFIG_SPIRAM_MODE_OCT
    printMacroText(out, "CONFIG_SPIRAM_MODE_OCT", ESP32_8048S043_STRINGIFY(CONFIG_SPIRAM_MODE_OCT));
#else
    printMacroText(out, "CONFIG_SPIRAM_MODE_OCT", "not defined");
#endif

#ifdef CONFIG_SPIRAM_MODE_QUAD
    printMacroText(out, "CONFIG_SPIRAM_MODE_QUAD", ESP32_8048S043_STRINGIFY(CONFIG_SPIRAM_MODE_QUAD));
#else
    printMacroText(out, "CONFIG_SPIRAM_MODE_QUAD", "not defined");
#endif

    printDivider(out);
}

void printPsramDiagnostic(Stream &out) {
    const uint32_t psramSize = ESP.getPsramSize();
    const bool found = psramFound();

    out.printf("%-28s: %s\n", "psramFound()", found ? "true" : "false");
    out.printf("%-28s: %s\n", "PSRAM runtime status", psramSize > 0 ? "DETECTED" : "NOT DETECTED");

    printSize(out, "IDF SPIRAM total", static_cast<uint32_t>(heap_caps_get_total_size(MALLOC_CAP_SPIRAM)));
    printSize(out, "IDF SPIRAM free", static_cast<uint32_t>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
    printSize(out, "IDF SPIRAM largest block", static_cast<uint32_t>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)));

    if (psramSize == 0) {
        out.println("PSRAM warning               : expected about 8 MB for N16R8-class board");
        out.println("PSRAM likely cause          : current Arduino build/profile did not enable OPI PSRAM");
        out.println("PSRAM next action           : compare [ARDUINO BUILD PROFILE] with Tools menu");
        out.println("PSRAM known-good reference  : ESP32S3 Dev Module + Flash 16MB + PSRAM OPI PSRAM");
    }
}

void printAcceptanceCheck(Stream &out) {
    const uint32_t flashSize = ESP.getFlashChipSize();
    const uint32_t psramSize = ESP.getPsramSize();
    const esp_partition_t *running = esp_ota_get_running_partition();

    const bool chipOk = String(ESP.getChipModel()) == "ESP32-S3";
    const bool flashOk = flashSize >= 15UL * 1024UL * 1024UL;
    const bool psramOk = psramSize >= 7UL * 1024UL * 1024UL;
    const bool partitionOk = running && running->size >= 3UL * 1024UL * 1024UL;

    out.println("[ACCEPTANCE CHECK]");
    out.printf("%-28s: %s\n", "Chip is ESP32-S3", passFail(chipOk));
    out.printf("%-28s: %s\n", "Flash is about 16 MB", passFail(flashOk));
    out.printf("%-28s: %s\n", "PSRAM is about 8 MB", passFail(psramOk));
    out.printf("%-28s: %s\n", "App partition about 3 MB", passFail(partitionOk));
    out.printf("%-28s: %s\n", "Overall BoardInfo", (chipOk && flashOk && psramOk && partitionOk) ? "PASS CANDIDATE" : "PROFILE / CONFIG ISSUE");

    if (!psramOk) {
        out.println("Acceptance note             : do not continue to LVGL/Web/OTA memory-heavy tests until PSRAM is detected.");
    }

    printDivider(out);
}

}  // namespace

bool ESP32_8048S043::begin() {
    return true;
}

void ESP32_8048S043::printBoardInfo(Stream &out) const {
    using namespace esp32_8048s043::pins;

    out.println("============================================================");
    out.println("ESP32-8048S043 Lab / 01_BoardInfo / Extended runtime report");
    out.println("Profile intent: ESP32-8048S043 / ESP32-S3 / RGB800x480 / GT911 / N16R8");
    out.println("Runtime truth : compile-time macros + ESP/IDF values below");
    printDivider(out);

    out.println("[BUILD / RUNTIME]");
    out.printf("%-28s: %s %s\n", "Sketch build", __DATE__, __TIME__);
    out.printf("%-28s: %s\n", "ESP-IDF SDK", ESP.getSdkVersion());
    out.printf("%-28s: %d (%s)\n",
               "Reset reason",
               static_cast<int>(esp_reset_reason()),
               resetReasonName(esp_reset_reason()));
    printDivider(out);

    printBuildProfile(out);

    out.println("[CHIP]");
    out.printf("%-28s: %s\n", "Chip model", ESP.getChipModel());
    out.printf("%-28s: %u\n", "Chip revision", ESP.getChipRevision());
    out.printf("%-28s: %u\n", "Chip cores", ESP.getChipCores());
    out.printf("%-28s: %u MHz\n", "CPU frequency", ESP.getCpuFreqMHz());
    printEfuseMac(out);
    printDivider(out);

    out.println("[MEMORY]");
    printSize(out, "Heap size", ESP.getHeapSize());
    printSize(out, "Free heap", ESP.getFreeHeap());
    printSize(out, "Min free heap", ESP.getMinFreeHeap());
    printSize(out, "Max alloc heap", ESP.getMaxAllocHeap());
    printSize(out, "PSRAM size", ESP.getPsramSize());
    printSize(out, "Free PSRAM", ESP.getFreePsram());
    printSize(out, "Max alloc PSRAM", ESP.getMaxAllocPsram());
    printPsramDiagnostic(out);
    printDivider(out);

    out.println("[FLASH / SKETCH]");
    printSize(out, "Flash chip size", ESP.getFlashChipSize());
    out.printf("%-28s: %lu Hz\n", "Flash chip speed", static_cast<unsigned long>(ESP.getFlashChipSpeed()));
    out.printf("%-28s: %s (%d)\n", "Flash chip mode", flashModeName(ESP.getFlashChipMode()), static_cast<int>(ESP.getFlashChipMode()));
    printSize(out, "Sketch size", ESP.getSketchSize());
    printSize(out, "Free sketch space", ESP.getFreeSketchSpace());
    out.printf("%-28s: %s\n", "Sketch MD5", ESP.getSketchMD5().c_str());
    printDivider(out);

    out.println("[PARTITIONS]");
    printPartition(out, "Running app", esp_ota_get_running_partition());
    printPartition(out, "Boot app", esp_ota_get_boot_partition());
    printDivider(out);

    out.println("[SOURCE-BACKED DISPLAY / TOUCH / SD PROFILE]");
    out.printf("%-28s: %dx%d RGB/DPI\n", "Display", LCD_WIDTH, LCD_HEIGHT);
    out.printf("%-28s: DE=%d VSYNC=%d HSYNC=%d PCLK=%d BL=%d\n",
               "RGB control",
               RGB_DE, RGB_VSYNC, RGB_HSYNC, RGB_PCLK, BACKLIGHT);
    out.printf("%-28s: %d,%d,%d,%d,%d\n", "RGB R0..R4", RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4);
    out.printf("%-28s: %d,%d,%d,%d,%d,%d\n", "RGB G0..G5", RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5);
    out.printf("%-28s: %d,%d,%d,%d,%d\n", "RGB B0..B4", RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4);
    out.printf("%-28s: SDA=%d SCL=%d RST=%d INT=%d ADDR=0x%02X/0x%02X\n",
               "GT911 touch",
               TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT, TOUCH_GT911_ADDR, TOUCH_GT911_ADDR_ALT);
    out.printf("%-28s: CS=%d MOSI=%d CLK=%d MISO=%d\n",
               "microSD SPI",
               SD_CS, SD_MOSI, SD_CLK, SD_MISO);
    printDivider(out);

    out.println("[EXPECTED SAMPLE A BASELINE]");
    out.println("Chip                    : ESP32-S3");
    out.println("Flash                   : about 16 MB / 16777216 bytes");
    out.println("PSRAM                   : about 8 MB / 8388608 bytes with OPI PSRAM enabled");
    out.println("USB bridge              : CH340C USB-UART, Arduino upload through selected COM port");
    out.println("Display                 : 800x480 RGB panel, separate display tests validate output");
    out.println("Touch                   : GT911, separate touch tests validate polling/coordinates");
    printDivider(out);

    printAcceptanceCheck(out);

    out.println("============================================================");
}
