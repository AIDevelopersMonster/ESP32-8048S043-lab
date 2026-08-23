#include "ESP32_8048S043.h"

#include <esp_ota_ops.h>
#include <esp_system.h>

namespace {

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

void printSize(Stream &out, const char *label, uint32_t bytes) {
    out.printf("%-24s: %lu bytes / %lu KB / %lu MB\n",
               label,
               static_cast<unsigned long>(bytes),
               static_cast<unsigned long>(bytes / 1024UL),
               static_cast<unsigned long>(bytes / (1024UL * 1024UL)));
}

void printEfuseMac(Stream &out) {
    const uint64_t mac = ESP.getEfuseMac();
    const uint32_t macLow = static_cast<uint32_t>(mac);
    const uint16_t macHigh = static_cast<uint16_t>(mac >> 32);

    out.printf("%-24s: 0x%04X%08lX\n",
               "eFuse MAC raw",
               macHigh,
               static_cast<unsigned long>(macLow));

    out.printf("%-24s: %02X:%02X:%02X:%02X:%02X:%02X\n",
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
        out.printf("%-24s: not available\n", title);
        return;
    }

    out.printf("%-24s: label=%s type=0x%02X subtype=0x%02X address=0x%06lX size=%lu\n",
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

void printPsramDiagnostic(Stream &out) {
    const uint32_t psramSize = ESP.getPsramSize();
    out.printf("%-24s: %s\n", "PSRAM status", psramSize > 0 ? "DETECTED" : "NOT DETECTED");
    if (psramSize == 0) {
        out.println("PSRAM warning           : expected about 8 MB for N16R8-class board");
        out.println("PSRAM next action       : retest Arduino IDE setting OPI PSRAM / Enabled");
        out.println("PSRAM note              : QSPI PSRAM setting produced 0 bytes on first Sample A run");
    }
}

}  // namespace

bool ESP32_8048S043::begin() {
    return true;
}

void ESP32_8048S043::printBoardInfo(Stream &out) const {
    using namespace esp32_8048s043::pins;

    out.println("============================================================");
    out.println("ESP32-8048S043 Lab / 01_BoardInfo / Extended runtime report");
    out.println("Profile: esp32-8048s043c-i-reference");
    out.println("Profile status: SOURCE-BACKED / OWN TESTS IN PROGRESS");
    printDivider(out);

    out.println("[BUILD / RUNTIME]");
    out.printf("%-24s: %s %s\n", "Sketch build", __DATE__, __TIME__);
    out.printf("%-24s: %s\n", "ESP-IDF SDK", ESP.getSdkVersion());
    out.printf("%-24s: %d (%s)\n",
               "Reset reason",
               static_cast<int>(esp_reset_reason()),
               resetReasonName(esp_reset_reason()));
    printDivider(out);

    out.println("[CHIP]");
    out.printf("%-24s: %s\n", "Chip model", ESP.getChipModel());
    out.printf("%-24s: %u\n", "Chip revision", ESP.getChipRevision());
    out.printf("%-24s: %u\n", "Chip cores", ESP.getChipCores());
    out.printf("%-24s: %u MHz\n", "CPU frequency", ESP.getCpuFreqMHz());
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
    out.printf("%-24s: %lu Hz\n", "Flash chip speed", static_cast<unsigned long>(ESP.getFlashChipSpeed()));
    out.printf("%-24s: %s (%d)\n", "Flash chip mode", flashModeName(ESP.getFlashChipMode()), static_cast<int>(ESP.getFlashChipMode()));
    printSize(out, "Sketch size", ESP.getSketchSize());
    printSize(out, "Free sketch space", ESP.getFreeSketchSpace());
    out.printf("%-24s: %s\n", "Sketch MD5", ESP.getSketchMD5().c_str());
    printDivider(out);

    out.println("[PARTITIONS]");
    printPartition(out, "Running app", esp_ota_get_running_partition());
    printPartition(out, "Boot app", esp_ota_get_boot_partition());
    printDivider(out);

    out.println("[SOURCE-BACKED DISPLAY / TOUCH / SD PROFILE]");
    out.printf("%-24s: %dx%d RGB/DPI\n", "Display", LCD_WIDTH, LCD_HEIGHT);
    out.printf("%-24s: DE=%d VSYNC=%d HSYNC=%d PCLK=%d BL=%d\n",
               "RGB control",
               RGB_DE, RGB_VSYNC, RGB_HSYNC, RGB_PCLK, BACKLIGHT);
    out.printf("%-24s: %d,%d,%d,%d,%d\n", "RGB R0..R4", RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4);
    out.printf("%-24s: %d,%d,%d,%d,%d,%d\n", "RGB G0..G5", RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5);
    out.printf("%-24s: %d,%d,%d,%d,%d\n", "RGB B0..B4", RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4);
    out.printf("%-24s: SDA=%d SCL=%d RST=%d INT=%d ADDR=0x%02X/0x%02X\n",
               "GT911 touch",
               TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT, TOUCH_GT911_ADDR, TOUCH_GT911_ADDR_ALT);
    out.printf("%-24s: CS=%d MOSI=%d CLK=%d MISO=%d\n",
               "microSD SPI",
               SD_CS, SD_MOSI, SD_CLK, SD_MISO);
    printDivider(out);

    out.println("[EXPECTED SAMPLE A BASELINE]");
    out.println("Chip        : ESP32-S3");
    out.println("Flash       : about 16 MB / 16777216 bytes");
    out.println("PSRAM       : about 8 MB / 8388608 bytes; Arduino QSPI run currently shows 0, OPI retest needed");
    out.println("USB bridge  : CH340C USB-UART, Arduino upload through selected COM port");
    out.println("Display     : 800x480 RGB panel, already factory-runtime PASS");
    out.println("Touch       : GT911 source-backed + factory visual touch PASS; dedicated scan still pending");
    out.println("============================================================");
}
