# App05 Network Provisioning v0.1.0 - Physical Pass Evidence

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Firmware:** `app05-network-provisioning-v0.1.0.bin`  
**Target:** ESP32-S3 / 16 MB flash / 8 MB PSRAM sample  
**Result:** PHYSICAL PASS

## Build evidence

GitHub Actions modular App05 build run `34006797499` completed successfully on ESP-IDF 5.5.5.

Artifact:

- `app05-network-provisioning-v0.1.0`
- artifact digest: `sha256:ee159332e966c09b995c1ac705e18353ca0f5a3ec687c69adcc757abc1844aee`

## Real-board boot evidence

The physical board booted App05 normally from the factory partition using the intended 16 MB OTA-ready layout.

Observed runtime sequence:

- ESP-IDF v5.5.5 booted successfully;
- Wi-Fi initialized in STA + SoftAP mode;
- SoftAP DHCP server started at `192.168.4.1`;
- local HTTP setup server started;
- saved credentials were found in NVS for SSID `TECNO CAMON 50`;
- the password was not printed in the serial log;
- STA connection started;
- the first association attempt was temporarily refused by the AP;
- App05 executed bounded retry `1/5`;
- the subsequent association succeeded using WPA2-PSK;
- DHCP assigned STA address `10.113.29.119`;
- App05 reported `STA online ip=10.113.29.119`;
- no reset loop, panic, NVS failure or networking crash was observed.

The runtime also kept the setup HTTP server available while the STA connection was being established.

## Functional confirmation

The user confirmed the App05 physical test with: `Все работает`.

This confirms the real-board network-provisioning stage as physically operational. The captured serial log directly demonstrates the saved-credential reconnect path, bounded retry behavior, WPA2 association and DHCP success. The user confirmation covers the exercised setup flow as a whole.

## Pass conclusion

**Final physical status:** `BUILD PASS / PHYSICAL PASS`.

The next validation boundary is Web Flasher installation of the same App05 build. Until that browser-to-board installation is physically confirmed, App05 Web Flasher status remains `CANDIDATE`.
