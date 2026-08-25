# Sample A / Arduino / 06_WiFiTest / Full Wi-Fi infrastructure evidence

Status: `FULL WIFI PHYSICAL PASS CANDIDATE / SAMPLE A`.

Date: `2026-08-25`

Evidence source:

```text
Serial Monitor runtime log supplied by operator after the scan-only validation run.
```

Related scan-only video evidence:

```text
https://youtube.com/shorts/DOus0uNBBZI
```

## Test target

```text
Example      : 06_WiFiTest
Purpose      : Wi-Fi radio scan + optional infrastructure validation
Board target : ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
Profile      : local Arduino sketchbook hardware profile
Mode         : full infrastructure mode, local wifi_secrets.h present
```

## Observed scan evidence

```text
[PASS] Wi-Fi scan completed: 2 network(s) found
  01  RSSI= -33 dBm  CH= 1  AUTH=3  SSID=ASUS_38_2G
  02  RSSI= -45 dBm  CH=13  AUTH=3  SSID=TECNO CAMON 50
```

## Observed association / DHCP evidence

The board connected to the configured access point:

```text
[CONNECT] Connecting to SSID: TECNO CAMON 50
[PASS] Associated and DHCP configuration acquired
```

Runtime network information:

```text
SSID       : TECNO CAMON 50
Channel    : 13
RSSI       : -46 dBm
STA MAC    : 84:FC:E6:6C:69:3C
IPv4       : 10.113.29.119
Gateway    : 10.113.29.215
Subnet     : 255.255.255.0
DNS 0      : 10.113.29.215
DNS 1      : 0.0.0.0
```

Note: the real Wi-Fi password is not present in the evidence and must not be committed.

## Observed DNS evidence

```text
[DNS] Resolving example.com ...
[PASS] DNS example.com -> 8.6.112.1
```

## Observed TCP/HTTP evidence

```text
[TCP] Connecting to example.com:80 ...
[PASS] TCP connection established
[PASS] HTTP response: HTTP/1.1 200 OK
```

## Observed reconnect evidence

```text
[RECONNECT] Running 3 disconnect/reconnect cycle(s)
[PASS] reconnect cycle 1/3  RSSI=-46 dBm  IP=10.113.29.119
[PASS] reconnect cycle 2/3  RSSI=-45 dBm  IP=10.113.29.119
[PASS] reconnect cycle 3/3  RSSI=-46 dBm  IP=10.113.29.119
```

Final result:

```text
WIFI TEST PHYSICAL PASS CANDIDATE
Scan + association + DHCP + DNS + TCP/HTTP + reconnect passed.
```

## Result

```text
Wi-Fi active scan        : PASS, 2 network(s) found
STA MAC readout          : PASS
Association              : PASS, configured AP connected
DHCP                     : PASS, IPv4/gateway/DNS received
DNS                      : PASS, example.com resolved
TCP                      : PASS, connection to example.com:80 established
HTTP                     : PASS, HTTP/1.1 200 OK response received
Reconnect cycles         : PASS, 3/3 cycles completed
Overall 06_WiFiTest      : FULL WIFI PHYSICAL PASS CANDIDATE / SAMPLE A
```

## Boundary

This evidence supports full Wi-Fi infrastructure PASS candidate for the Arduino `06_WiFiTest` example on Sample A.

It does not yet prove:

```text
on-device Web setup UI;
embedded web server behavior;
TLS/HTTPS download;
GitHub OTA manifest/download/SHA validation;
Widget Runtime network workflow;
long-duration Wi-Fi stability;
Wi-Fi behavior while RGB/LVGL rendering is active.
```

Those remain separate tests.
