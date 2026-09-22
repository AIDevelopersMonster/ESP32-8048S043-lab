# Руководство системного программиста

Проект: **KONTAKTS / ESP32-8048S043 Lab**  
Платформа: ESP32-S3, 800×480 RGB, GT911, 16 MiB Flash, 8 MiB PSRAM  
Текущая линия: App06/App07 Platform Shell + OTA + Persistent Widget Runtime  
Текущая версия разработки: `0.2.1`  
Статус документа: **рабочая структура с текущими правилами; дополняется по мере физической валидации**.

---

## 1. Назначение руководства

Документ описывает работы, требующие изменения системного слоя прошивки. Системный программист отвечает за то, чтобы прикладные функции могли развиваться без нарушения boot/recovery, Wi-Fi, OTA, файловой системы и базового HMI.

Не относится к системному уровню: создание конкретного JSON-виджета, изменение его текстов/координат, прикладная компоновка web-страницы, если это не меняет системный API или безопасность.

## 2. Состав системной платформы

### 2.1 Firmware-resident слой

В прошивке постоянно находятся:

```text
ESP-IDF platform
RGB LCD driver
GT911 touch driver
LVGL 9 runtime
Wi-Fi provisioning / STA manager
HTTP control server
NVS credentials
SPIFFS mount / storage engine
Widget Runtime parser + validator
STATUS page
OTA page
CONFIRM / ROLLBACK
FACTORY RECOVERY
allow-list bindings/actions
```

### 2.2 File-resident слой

Файловая система не должна быть необходима для доступа к recovery-функциям.

Текущий контракт:

```text
/storage/widget.tmp     temporary candidate
/storage/widget.json    active declarative widget
/storage/widget.bak     transient replacement backup
```

При повреждении/отсутствии `widget.json` системные STATUS/OTA/recovery должны оставаться работоспособны.

## 3. Карта Flash и политика разделов

Зафиксировать и поддерживать таблицу:

```text
nvs        credentials/settings
otadata    OTA selection/rollback metadata
phy_init
factory    recovery application
ota_0      working application slot A
ota_1      working application slot B
storage    persistent SPIFFS resources
coredump
```

### 3.1 Инварианты

- обычный GitHub OTA пишет только app-slot;
- обычный OTA не форматирует NVS и `storage`;
- изменение partition table — отдельная миграционная операция и не должно маскироваться под обычный OTA;
- `storage` должен переживать reset, rollback и штатный app-slot OTA;
- полный образ `0x0` — сервисная операция, способная изменить больше разделов, чем обычный OTA.

## 4. Boot, VALID/PENDING_VERIFY и rollback

### 4.1 Рабочий цикл

```text
VALID image
   -> install newer image
   -> PENDING_VERIFY
   -> physical/software checks
   -> CONFIRM
   -> VALID
```

Если PENDING_VERIFY перезагрузить без CONFIRM, bootloader может вернуть последний работоспособный образ.

### 4.2 Практическое правило лаборатории

**Не начинать reset/persistence-тест новой версии, пока она не CONFIRM.**

Иначе тест файловой системы смешивается с тестом rollback.

## 5. Factory / recovery policy

### 5.1 Выявленный недостаток

Историческая `factory 0.1.0` слишком бедна для постоянной эксплуатации: при rollback пользователь получает старый интерфейс и теряет удобный локальный системный shell.

### 5.2 Целевая модель

```text
factory = редкo обновляемый стабильный recovery shell
ota_0/ota_1 = часто обновляемая рабочая платформа
```

Recovery factory должна минимум иметь:

- дисплей и touch;
- STATUS;
- Wi-Fi provisioning;
- GitHub OTA check/install;
- диагностическую информацию;
- безопасный доступ к восстановлению;
- по возможности чтение существующего `/storage`, но не зависеть от него для загрузки.

### 5.3 Замена factory

Предпочтительно иметь отдельный `factory-app.bin`, записываемый строго в factory partition без стирания NVS и `storage`. Такая операция должна иметь собственную инструкцию, контроль адреса/размера/SHA-256 и отдельный physical PASS.

## 6. OTA subsystem

### 6.1 Manifest contract

Проверяются минимум:

- schema;
- board id;
- app id;
- semantic version;
- channel;
- image size;
- SHA-256;
- разрешённый GitHub Release URL prefix.

### 6.2 Download/install

Порядок:

```text
HTTPS -> app slot -> streaming SHA-256 -> esp_ota_end
-> project/version descriptor check
-> set boot partition
-> reboot PENDING_VERIFY
```

### 6.3 TLS/heap

На `0.2.0` физически наблюдался повторный CHECK с ошибкой TLS allocation (`mbedtls_ssl_setup -0x7F00`). В `0.2.1` включён курс на PSRAM-backed mbedTLS dynamic allocation.

Перед сетевыми hardening-тестами логировать:

```text
free internal heap
largest internal free block
free PSRAM
UI task stack high-water
OTA task stack high-water
```

Acceptance: несколько последовательных `CHECK GITHUB`, затем download/install, при активном LVGL и Widget Runtime без `ESP_ERR_NO_MEM`/TLS alloc failure.

## 7. Файловая система

### 7.1 Mount policy

Текущий backend — SPIFFS partition `storage`, mount `/storage`.

Нужно документировать:

- total/used bytes;
- mount failure;
- format policy;
- максимальное число открытых файлов;
- поведение после brownout/reset во время записи.

### 7.2 Atomic-ish widget replacement

```text
receive JSON
 -> validate in RAM
 -> write widget.tmp
 -> flush/fsync
 -> current widget.json -> widget.bak
 -> widget.tmp -> widget.json
 -> remove backup
 -> publish new model/generation
```

Rejected JSON не должен становиться активным.

## 8. Widget Runtime как системный сервис

Системный программист отвечает за:

- parser/validator;
- memory ownership;
- schema version dispatch;
- object limits;
- allow-list bindings/actions;
- generation/reload mechanism;
- thread safety между HTTP/OTA/UI tasks;
- graceful fallback при invalid/missing widget.

Прикладной программист использует этот контракт, но не обходит его произвольным native code из файловой системы.

## 9. Display / touch / LVGL

Зафиксированный аппаратный baseline должен содержать:

- RGB timings/PCLK;
- pin mapping;
- framebuffer/bounce buffer policy;
- LVGL partial buffer size/location;
- GT911 I2C port/pins/reset;
- raw-to-screen coordinate scaling;
- UI task core/priority/stack.

Изменение этих параметров требует отдельного display/touch regression test.

## 10. Network manager

Состояния минимум:

```text
BOOT
AP_SETUP
STA_CONNECTING
STA_ONLINE
```

Требования:

- пароль не печатается в лог;
- сохранённые credentials — в NVS;
- после успешного STA provisioning AP отключается;
- intentional reboot не должен порождать ложный recovery/AP fallback;
- web/API режим должен соответствовать текущему network state.

## 11. HTTP control server

Системный слой владеет регистрацией URI, ограничениями body size, authentication policy и безопасными системными endpoints.

Текущие группы:

```text
/status
/scan
/save
/clear
/ota/*
/widget/*
```

При добавлении endpoint обязательно определить:

- доступность в AP_SETUP/STA;
- максимальный request size;
- response codes;
- concurrency/locking;
- влияние на heap/TLS;
- права прикладного слоя.

## 12. Версионирование и релизы

### 12.1 Правило

Не перезаписывать уже опубликованный физически тестируемый релиз новой логикой. Исправление после `0.2.0` -> новая версия `0.2.1`.

### 12.2 CI

Минимальный pipeline:

```text
resolve version
build ESP-IDF
assemble OTA/full artifacts
SHA-256
publish artifact
publish release manifest/assets
```

### 12.3 Physical evidence

CI PASS != physical PASS.

Каждая системно значимая версия получает evidence-файл с:

- hardware specimen;
- version/partition/image state;
- serial proof;
- display/touch observation;
- network observation;
- tested transitions;
- known defects;
- PASS boundary.

## 13. Диагностика и журналы

Лог должен позволять восстановить цепочку:

```text
BOOT -> running partition -> version -> image state
-> filesystem mount -> widget autoload
-> display/touch init
-> network state/IP
-> OTA action/result
```

Системные ошибки классифицировать: boot, storage, display/touch, network, TLS, OTA validation, memory, widget parser, web server.

## 14. Текущая программа системных работ

### Приоритет A

1. физически проверить `0.2.1` после TLS hardening;
2. сделать `0.2.1` VALID до reset/persistence тестов;
3. повторить несколько `CHECK GITHUB` подряд;
4. проверить widget A/B и reset persistence.

### Приоритет B

5. спроектировать современный recovery factory;
6. собрать отдельный factory application image;
7. проверить запись только factory partition без NVS/storage erase;
8. доказать boot/recovery/OTA из нового factory.

### Приоритет C

9. доказать сохранение `widget.json` через следующий обычный firmware OTA;
10. добавить crash/brownout tests для widget replacement;
11. определить migration policy для будущей schema 2/filesystem backend.

## 15. Checklist перед изменением системного кода

- [ ] Меняется partition/boot contract?
- [ ] Сохраняются NVS и `/storage`?
- [ ] STATUS/OTA/recovery доступны без widget.json?
- [ ] Новый код увеличивает internal DRAM pressure?
- [ ] Проверен largest internal block перед TLS?
- [ ] OTA rollback path остаётся рабочим?
- [ ] Изменение совместимо со старым widget schema?
- [ ] Обновлены CI/release/evidence/manuals?
- [ ] Physical PASS отделён от CI PASS?
