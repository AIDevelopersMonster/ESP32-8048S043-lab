/*
  ESP32-8048S043 Lab / 08_SDCardTest

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Purpose:
    Validate the source-backed microSD / TF SPI pin map on the ESP32-8048S043
    board after the board profile, display, touch, Wi-Fi and WebServer layers
    have been brought up.

  What this example checks:
    - custom board profile remains alive after upload;
    - SD SPI pins are configured from ESP32_8048S043_Pins.h;
    - SD card can be mounted through Arduino SD.h;
    - card type and size can be read;
    - root directory can be opened and listed;
    - no write, format, delete or rename operation is performed.

  What this example does NOT check:
    - filesystem write safety;
    - formatting;
    - long-duration SD stress;
    - concurrent SD + RGB/LVGL rendering;
    - SD-backed web upload or Widget Runtime storage.

  Pin map under test:
    CS   = GPIO10
    MOSI = GPIO11
    CLK  = GPIO12
    MISO = GPIO13

  PASS boundary:
    PASS candidate requires a mounted card, readable card metadata and a root
    directory listing, with no reboot/brownout/crash. This is a read-only test.
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include <ESP32_8048S043_Pins.h>

using namespace esp32_8048s043::pins;

static constexpr uint32_t TEST_FREQUENCIES_HZ[] = {
  10000000UL,
  4000000UL,
  1000000UL,
  400000UL
};

static constexpr uint8_t MAX_ROOT_ENTRIES = 40;
static constexpr uint8_t MAX_RECURSION_DEPTH = 1;

static uint32_t mountedFrequencyHz = 0;
static uint32_t lastAliveMs = 0;
static bool testPassed = false;

static void printDivider() {
  Serial.println("------------------------------------------------------------");
}

static String formatBytes(uint64_t value) {
  char buf[40];

  if (value >= 1024ULL * 1024ULL * 1024ULL) {
    snprintf(buf, sizeof(buf), "%.2f GB", static_cast<double>(value) / (1024.0 * 1024.0 * 1024.0));
  } else if (value >= 1024ULL * 1024ULL) {
    snprintf(buf, sizeof(buf), "%.2f MB", static_cast<double>(value) / (1024.0 * 1024.0));
  } else if (value >= 1024ULL) {
    snprintf(buf, sizeof(buf), "%.2f KB", static_cast<double>(value) / 1024.0);
  } else {
    snprintf(buf, sizeof(buf), "%llu B", static_cast<unsigned long long>(value));
  }

  return String(buf);
}

static const char *cardTypeName(uint8_t type) {
  switch (type) {
    case CARD_NONE: return "NONE";
    case CARD_MMC: return "MMC";
    case CARD_SD: return "SDSC";
    case CARD_SDHC: return "SDHC/SDXC";
    default: return "UNKNOWN";
  }
}

static bool mountAt(uint32_t frequencyHz) {
  Serial.printf("[MOUNT] Trying SD.begin(CS=%d, CLK=%d, MISO=%d, MOSI=%d, freq=%lu Hz)\n",
                SD_CS,
                SD_CLK,
                SD_MISO,
                SD_MOSI,
                static_cast<unsigned long>(frequencyHz));

  SD.end();
  SPI.end();
  delay(150);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI, frequencyHz)) {
    Serial.println("[WARN] SD.begin() failed at this frequency");
    return false;
  }

  const uint8_t type = SD.cardType();
  if (type == CARD_NONE) {
    Serial.println("[WARN] SD mounted but card type is CARD_NONE");
    SD.end();
    return false;
  }

  mountedFrequencyHz = frequencyHz;
  Serial.println("[PASS] SD mounted");
  return true;
}

static bool mountCard() {
  for (uint8_t i = 0; i < sizeof(TEST_FREQUENCIES_HZ) / sizeof(TEST_FREQUENCIES_HZ[0]); ++i) {
    if (mountAt(TEST_FREQUENCIES_HZ[i])) {
      return true;
    }
    delay(300);
  }

  return false;
}

static void printCardInfo() {
  const uint8_t type = SD.cardType();
  const uint64_t cardSize = SD.cardSize();
  const uint64_t totalBytes = SD.totalBytes();
  const uint64_t usedBytes = SD.usedBytes();

  Serial.println("[CARD]");
  Serial.printf("Card type          : %s (%u)\n", cardTypeName(type), static_cast<unsigned int>(type));
  Serial.printf("Mounted frequency  : %lu Hz\n", static_cast<unsigned long>(mountedFrequencyHz));
  Serial.printf("Card size          : %llu bytes / %s\n",
                static_cast<unsigned long long>(cardSize),
                formatBytes(cardSize).c_str());
  Serial.printf("Filesystem total   : %llu bytes / %s\n",
                static_cast<unsigned long long>(totalBytes),
                formatBytes(totalBytes).c_str());
  Serial.printf("Filesystem used    : %llu bytes / %s\n",
                static_cast<unsigned long long>(usedBytes),
                formatBytes(usedBytes).c_str());
  Serial.printf("Filesystem free    : %llu bytes / %s\n",
                static_cast<unsigned long long>(totalBytes > usedBytes ? totalBytes - usedBytes : 0),
                formatBytes(totalBytes > usedBytes ? totalBytes - usedBytes : 0).c_str());
}

static bool listDirectory(const char *dirname, uint8_t depth, uint8_t &entryCount) {
  File root = SD.open(dirname);

  if (!root) {
    Serial.printf("[FAIL] Failed to open directory: %s\n", dirname);
    return false;
  }

  if (!root.isDirectory()) {
    Serial.printf("[FAIL] Not a directory: %s\n", dirname);
    root.close();
    return false;
  }

  Serial.printf("[LIST] Directory: %s\n", dirname);

  while (entryCount < MAX_ROOT_ENTRIES) {
    File file = root.openNextFile();
    if (!file) {
      break;
    }

    const bool isDir = file.isDirectory();
    const String name = String(file.name());

    if (isDir) {
      Serial.printf("  DIR   %s\n", name.c_str());

      if (depth > 0 && name.length() > 0 && name != "/") {
        String childPath;
        if (name.startsWith("/")) {
          childPath = name;
        } else if (strcmp(dirname, "/") == 0) {
          childPath = "/" + name;
        } else {
          childPath = String(dirname) + "/" + name;
        }

        file.close();
        ++entryCount;
        listDirectory(childPath.c_str(), depth - 1, entryCount);
        continue;
      }
    } else {
      Serial.printf("  FILE  %-32s %10llu bytes / %s\n",
                    name.c_str(),
                    static_cast<unsigned long long>(file.size()),
                    formatBytes(file.size()).c_str());
    }

    file.close();
    ++entryCount;
  }

  root.close();

  if (entryCount >= MAX_ROOT_ENTRIES) {
    Serial.printf("[INFO] Listing stopped after %u entries\n", MAX_ROOT_ENTRIES);
  }

  return true;
}

static bool readFirstRegularFilePreview(const char *dirname) {
  File root = SD.open(dirname);
  if (!root || !root.isDirectory()) {
    if (root) {
      root.close();
    }
    return false;
  }

  while (true) {
    File file = root.openNextFile();
    if (!file) {
      break;
    }

    if (!file.isDirectory() && file.size() > 0) {
      Serial.printf("[READ] Preview first bytes from: %s\n", file.name());
      Serial.print("[READ] HEX:");

      uint8_t count = 0;
      while (file.available() && count < 32) {
        const uint8_t b = static_cast<uint8_t>(file.read());
        Serial.printf(" %02X", b);
        ++count;
      }

      Serial.println();
      Serial.printf("[PASS] Read-only preview completed, %u byte(s) shown\n", count);
      file.close();
      root.close();
      return true;
    }

    file.close();
  }

  root.close();
  Serial.println("[INFO] No regular non-empty file found in root for read preview; mount/list still valid.");
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" ESP32-8048S043 Lab / 08_SDCardTest");
  Serial.println(" Read-only microSD / TF SPI validation");
  Serial.println("============================================================");
  Serial.println("Author : Alex Malachevsky");
  Serial.println("GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
  Serial.println("----------------------------------------------------------------");
  Serial.printf("Pin map: CS=%d MOSI=%d CLK=%d MISO=%d\n", SD_CS, SD_MOSI, SD_CLK, SD_MISO);
  Serial.println("Mode   : read-only, no write/format/delete/rename operations");
  Serial.println("----------------------------------------------------------------");
  Serial.printf("Runtime: chip=%s rev=%u flash=%lu psram=%lu freePsram=%lu\n",
                ESP.getChipModel(),
                ESP.getChipRevision(),
                static_cast<unsigned long>(ESP.getFlashChipSize()),
                static_cast<unsigned long>(ESP.getPsramSize()),
                static_cast<unsigned long>(ESP.getFreePsram()));
  printDivider();

  if (!mountCard()) {
    Serial.println();
    Serial.println("============================================================");
    Serial.println(" SD CARD TEST FAIL");
    Serial.println(" Could not mount card on source-backed SPI pins.");
    Serial.println(" Check card insertion, formatting, pins and board revision.");
    Serial.println("============================================================");
    return;
  }

  printDivider();
  printCardInfo();

  printDivider();
  uint8_t entryCount = 0;
  const bool listed = listDirectory("/", MAX_RECURSION_DEPTH, entryCount);

  printDivider();
  readFirstRegularFilePreview("/");

  printDivider();

  if (listed) {
    testPassed = true;
    Serial.println("============================================================");
    Serial.println(" SD CARD READ-ONLY PHYSICAL PASS CANDIDATE");
    Serial.println(" Mount + metadata + root listing completed. No writes performed.");
    Serial.println("============================================================");
  } else {
    Serial.println("============================================================");
    Serial.println(" SD CARD TEST PARTIAL / DIRECTORY LIST FAILED");
    Serial.println(" Card mounted, but root directory listing did not pass.");
    Serial.println("============================================================");
  }
}

void loop() {
  const uint32_t now = millis();
  if (now - lastAliveMs >= 5000) {
    lastAliveMs = now;
    Serial.printf("[ALIVE] uptime=%lus sd=%s freq=%luHz freeHeap=%lu psram=%lu freePsram=%lu\n",
                  static_cast<unsigned long>(now / 1000),
                  testPassed ? "PASS_CANDIDATE" : "OPEN_OR_FAIL",
                  static_cast<unsigned long>(mountedFrequencyHz),
                  static_cast<unsigned long>(ESP.getFreeHeap()),
                  static_cast<unsigned long>(ESP.getPsramSize()),
                  static_cast<unsigned long>(ESP.getFreePsram()));
  }

  delay(10);
}
