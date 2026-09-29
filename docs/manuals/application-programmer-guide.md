# Руководство прикладного (функционального) программиста

Проект: **KONTAKTS / ESP32-8048S043 Platform**  
Назначение: создание виджетов, прикладных экранов, web-интерфейсов и разрешённых функций поверх стабильного platform shell.  
Текущий runtime: JSON Widget Schema v1.

---

## 1. Роль прикладного программиста

Прикладной программист развивает пользовательскую функциональность, не изменяя bootloader, partition table, recovery contract и аппаратные драйверы без отдельной системной задачи.

Основные зоны:

```text
widget JSON
layout / styles
bindings
safe actions
web UI
browser JavaScript
application HTTP APIs
resource packaging
user-facing validation/errors
```

## 2. Архитектурная граница

```text
Firmware shell: STATUS / OTA / recovery / drivers / parser
                         |
                         | stable API
                         v
Application layer: widget.json / web UI / bindings / actions
```

Внешний виджет — данные, а не ELF/C/C++ модуль. Нельзя использовать JSON как способ исполнения произвольного native code.

## 3. Жизненный цикл виджета

```text
create JSON
 -> local/schema validation
 -> browser upload
 -> POST /widget/install
 -> runtime validation
 -> /storage/widget.json
 -> generation increment
 -> live LVGL rebuild
 -> reset/autoload
```

Для замены firmware OTA не требуется.

## 4. Widget Schema v1

### 4.1 Root

```json
{
  "schema": 1,
  "id": "vendor.widget",
  "name": "Widget Name",
  "version": "1.0.0",
  "background": "#101820",
  "objects": []
}
```

Текущие ограничения:

```text
JSON size       <= 32 KiB
objects         <= 24
bound labels    <= 8
```

### 4.2 Объекты

#### label

Назначение: статический текст или динамический binding.

Поля: `x`, `y`, `w`, `text`, `bind`, `prefix`, `suffix`, `color`.

#### bar

Текущий v1: статическое значение `0..100`.

#### button

Кнопка может выполнять только разрешённое firmware action.

Текущий базовый allow-list:

```text
show_status
show_ota
```

## 5. Bindings

Текущий v1:

```text
system.uptime
system.heap
system.psram
wifi.ip
wifi.rssi
firmware.version
ota.state
```

### Правило расширения binding

Новый binding должен иметь:

1. уникальное стабильное имя;
2. определённый тип/строковый формат;
3. поведение при unavailable/offline;
4. частоту обновления;
5. оценку стоимости по CPU/heap/I/O;
6. описание в этом руководстве;
7. regression test.

## 6. Safe Actions

Action не должен давать JSON прямой доступ к arbitrary address/function pointer/native command.

Новая action оформляется как firmware allow-list capability:

```text
JSON action name
 -> validator allows
 -> runtime maps to known handler
 -> handler checks state/arguments
 -> observable result/error
```

Будущие кандидаты: navigation, brightness, acknowledge, user-defined application command через безопасный dispatcher.

## 7. Координаты и layout

Рабочая зона WIDGET меньше физического 800×480, потому что системная верхняя навигация остаётся firmware-resident.

При создании виджета учитывать:

- reserved system header;
- размер внутреннего widget container;
- touch target минимум ~44–52 px для основных кнопок;
- длинные строки и перенос;
- отсутствие перекрытия системной навигации;
- читаемость на реальном 4.3" дисплее, а не только в браузерном макете.

## 8. Web UI и браузерная часть

### 8.1 Текущая функция

Web-страница устройства даёт:

- network/status;
- OTA controls;
- filesystem widget status;
- выбор `.json`;
- `UPLOAD & INSTALL WIDGET`;
- delete widget.

### 8.2 Как улучшать web UI

Изменения делать слоями:

```text
HTML structure
CSS responsive layout
browser-side JS
REST-like device endpoints
```

Не смешивать большие HTML-строки firmware с прикладной логикой бесконечно. По мере роста перейти к разделению статических web resources и API.

### 8.3 Рекомендуемая следующая web-архитектура

```text
GET  /api/platform/status
GET  /api/ota/status
POST /api/ota/check
POST /api/ota/install
GET  /api/widget/status
POST /api/widget/install
POST /api/widget/delete

/static/index.html
/static/app.js
/static/app.css
```

Статические web resources в будущем могут также жить в filesystem, но должна существовать firmware-resident recovery page на случай повреждения web resources.

### 8.4 Browser UX требования

- mobile + desktop responsive;
- явный текущий version/partition/state;
- disable кнопок во время busy action;
- progress/error text;
- upload size/schema error до отправки, если возможно;
- после install показывать id/name/version/generation;
- не хранить Wi-Fi пароль в JavaScript/localStorage;
- destructive actions должны быть визуально отделены.

## 9. Widget development workflow

1. Скопировать `widget-demo-a.json` или минимальный template.
2. Изменить `id`, `name`, `version`.
3. Собрать layout из разрешённых objects.
4. Проверить размеры/цвета/bindings/actions.
5. Загрузить через web UI.
6. Проверить live render без reboot.
7. Проверить touch.
8. Reset и проверить autoload.
9. Проверить системные STATUS/OTA buttons.
10. Сохранить widget JSON в репозитории и добавить пользовательский раздел.

## 10. Версионирование виджетов

Рекомендуется независимая SemVer-like версия:

```text
firmware 0.2.1
widget demo.platform-status 1.0.0
```

Firmware и widget не обязаны иметь одинаковую версию.

При использовании новой schema/binding/action виджет должен указывать минимально поддерживаемый runtime, когда это поле будет введено.

## 11. Совместимость

Прикладной программист не должен предполагать, что любой будущий firmware поддержит неизвестные поля.

Правило schema v1: unsupported object/binding/action -> reject, а не частично исполнять неизвестную семантику.

Будущая schema 2 должна иметь явную migration/compatibility policy.

## 12. Ошибки и сообщения

Хорошая прикладная ошибка должна сообщать:

```text
что отклонено
какое поле/объект
почему
какой диапазон/значение допустимо
```

Не заменять validation failure общим `500` без причины.

## 13. Тестовый набор виджета

Минимум:

- [ ] install valid JSON;
- [ ] immediate live render;
- [ ] reset/autoload;
- [ ] replace with second widget without OTA;
- [ ] invalid JSON rejected;
- [ ] unsupported binding rejected;
- [ ] unsupported action rejected;
- [ ] STATUS/OTA reachable after widget failure;
- [ ] repeated upload does not leak memory;
- [ ] long text does not break system navigation.

## 14. Текущая программа прикладных работ

Базовые Web/BLE/mobile transport proofs уже закрыты в App15/App16/App17. Прикладной слой теперь должен развивать UX и reusable capabilities, не создавая отдельную аппаратную логику для каждого клиента.

### Виджеты и SD applications

1. стандартизовать package/entrypoint UX;
2. продолжить capability-driven packages;
3. улучшить state/error presentation;
4. документировать совместимость package -> minimum platform.

### Web/browser

5. уменьшить polling и отделить live state от агрессивного Modbus READ;
6. постепенно разделять static frontend и API;
7. добавить ясные diagnostics/reconnect states.

### Mobile/BLE

8. улучшить App17 state parsing DO1..DO8;
9. добавить reconnect UX;
10. определить notifications/authentication только после отдельного platform contract.

### Runtime API

11. формализовать binding/action/provider registry;
12. capability/version negotiation;
13. schema documentation + examples;
14. не допускать дублирования Modbus register maps в Web/BLE/mobile clients.

## 15. Что требует передачи системному программисту

Следующие задачи нельзя тихо решать только в прикладном слое:

- новый filesystem backend/partition;
- native plugins;
- новый аппаратный драйвер;
- изменение LVGL task/memory architecture;
- изменение boot/recovery/OTA;
- TLS/security/authentication;
- изменение системных endpoints;
- увеличение лимитов, влияющее на RAM/flash safety.


## 16. Общий command/service контракт

Прикладные клиенты Platform 0.3.9 должны использовать общий service/API слой:

```text
Web
BLE
Android
UART0 engineering tool
        |
        v
command/service layer
        |
        v
provider
```

Для MA01 прикладному клиенту разрешены команды уровня:

```text
MA01 READ
MA01 DO1 INFO
MA01 DO1 ACTION
MA01 DO1 ON
MA01 DO1 OFF
MA01 DO1 MODE LEVEL
MA01 DO1 MODE PULSE
MA01 DO1 PULSEMS 5000
```

Modbus register addresses являются деталью provider layer и не должны копироваться в Android/JavaScript/UI packages.
