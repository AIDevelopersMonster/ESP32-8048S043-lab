# App 04 - Persistent Storage / Partial Resource Loading

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Programmer:** Sol  
**Engineer:** Alex Malachevsky  
**Status:** IMPLEMENTATION / BUILD PENDING / PHYSICAL PENDING

## Purpose

App 04 introduces the storage contract needed before Wi-Fi provisioning, OTA and real equipment projects.

It deliberately isolates storage from the HMI so failures can be attributed to NVS/filesystem behavior rather than the already validated display/touch runtime.

## New variables

1. persistent small settings in NVS;
2. internal filesystem mounted at `/storage`;
3. project/config resources loaded in bounded chunks rather than copied into one large RAM buffer;
4. an OTA-ready 16 MB partition layout with factory + two OTA application slots and a dedicated internal storage partition.

## App 04 boot behavior

On every boot the firmware:

- initializes NVS;
- creates default settings if they do not yet exist;
- increments and persists a boot counter;
- reads persistent `device_name` and `brightness` settings;
- mounts the internal SPIFFS resource partition;
- reports total/used filesystem size;
- reads `/storage/platform.cfg` and `/storage/ui/settings-screen.cfg` in fixed-size chunks;
- calculates a streaming FNV-1a checksum while loading;
- reports byte/chunk counts without allocating a buffer equal to the whole file.

## Why chunked loading matters

Future incubator, greenhouse, thermostat and other projects may contain many profiles, histories, UI resources and model descriptions. The platform must not require all of that material to be compiled into the application or loaded into RAM at once.

App 04 establishes the rule:

> resources are addressable files; consumers load only the file/resource they need, and may stream it in bounded chunks.

The same higher-level storage contract can later be backed by internal flash or SD.

## Storage hierarchy

- **NVS** - small durable settings and selectors;
- **internal filesystem** - compact resources required when no SD card is installed;
- **SD card (later)** - large project packages, history, media, logs, profiles and optional models.

## Partition direction

The App04 partition table reserves:

- NVS;
- OTA metadata;
- PHY init data;
- factory application slot;
- OTA slot 0;
- OTA slot 1;
- internal SPIFFS storage;
- coredump area.

This intentionally prepares the flash topology before the OTA application is introduced.

## Physical PASS boundary

App 04 is PHYSICAL PASS only after the real board demonstrates all of the following:

1. first boot creates defaults and mounts `/storage`;
2. second/repeated boot shows an increased persistent boot counter;
3. `device_name` and `brightness` are read from NVS without NVS initialization errors;
4. both resource files are read successfully;
5. chunk count / byte count / checksum are stable for unchanged files;
6. no reset loop, partition error, filesystem mount failure or heap exhaustion occurs.

## Explicit non-goals

App 04 does **not** add Wi-Fi, AP mode, web setup or OTA download yet. Those become the next controlled variables after the storage contract passes physically.

## Next step after PASS

App 05: network provisioning foundation:

- Wi-Fi STA registration with a router;
- first-run/fallback access point;
- local web setup;
- persistent credentials/settings through the App04 storage service.
