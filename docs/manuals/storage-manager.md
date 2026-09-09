# Storage Manager — руководство пользователя

Устройство: **KONTAKTS / ESP32-8048S043 Platform**  
Назначение: управление файлами внутренней файловой системы `/storage` через браузер.

> **Storage Manager не является установщиком виджетов.**  
> Виджеты устанавливаются через отдельный раздел **Filesystem Widget**. Storage Manager показывает и обслуживает файлы внутреннего хранилища.

---

## 1. Как открыть Storage Manager

1. Подключить KONTAKTS Platform к Wi-Fi.
2. Узнать IP устройства на экране `STATUS` или в serial log.
3. Открыть в браузере:

```text
http://<IP-устройства>/
```

Например:

```text
http://192.168.1.71/
```

4. Найти карточку:

```text
Storage Manager
```

На текущей платформе она показывает содержимое внутренней SPIFFS-файловой системы, смонтированной как:

```text
/storage
```

---

## 2. Как выглядит таблица

Физически проверенный интерфейс имеет колонки:

```text
Name | Size | Type | Action
```

Пример состояния App08:

| Name | Size | Type | Action |
|---|---:|---|---|
| `platform.cfg` | 121 | user | Download / Delete |
| `ui-settings-screen.cfg` | 121 | user | Download / Delete |
| `widget.json` | 1948 | system / protected | protected |
| `youtube-history.bin` | 32 | user | Download / Delete |

Размеры файлов могут меняться между версиями и после работы устройства. Таблица показывает фактическое состояние конкретной платы.

---

## 3. Значение колонок

### Name

Имя файла внутри `/storage`.

### Size

Текущий размер файла в байтах.

### Type

Storage Manager различает два пользовательски важных класса:

```text
user
system / protected
```

- `user` — обычный файл, который интерфейс разрешает скачивать и удалять;
- `system / protected` — системный файл, который виден, но не может быть заменён или удалён через Storage Manager.

### Action

Для обычного файла доступны:

```text
Download
Delete
```

Для защищённого файла выводится:

```text
protected
```

---

## 4. `widget.json` — активный виджет

Файл:

```text
/storage/widget.json
```

— это активный внешний виджет платформы.

Он специально классифицируется как:

```text
system / protected
```

### Почему он защищён

Widget Runtime должен сначала проверить JSON, его схему, допустимые объекты, bindings и actions. Только после успешной проверки новый виджет становится активным и сохраняется как `/storage/widget.json`.

Поэтому **нельзя устанавливать виджет обычной загрузкой файла через Storage Manager**.

Правильный путь:

```text
Filesystem Widget
      |
      | выбрать widget-*.json
      v
UPLOAD & INSTALL WIDGET
      |
      | validation
      v
/storage/widget.json
      |
      v
WIDGET на TFT
```

Так платформа не позволяет случайному или некорректному JSON напрямую заменить активное приложение.

---

## 5. Как устанавливать виджеты и их клоны

Готовые виджеты проекта находятся в репозитории:

```text
apps/06_OTARecovery/widgets/
```

Например:

```text
widget-ntp-sevenseg-clock.json
widget-youtube-dashboard.json
widget-youtube-views.json
widget-youtube-subscribers.json
```

Порядок установки:

1. скачать нужный `.json`;
2. открыть web-интерфейс KONTAKTS Platform;
3. найти **Filesystem Widget**;
4. выбрать `.json`;
5. нажать **UPLOAD & INSTALL WIDGET**;
6. дождаться `INSTALL PASS`;
7. открыть вкладку `WIDGET` на TFT.

Новая firmware для замены совместимого виджета не требуется.

---

## 6. `youtube-history.bin`

Файл:

```text
/storage/youtube-history.bin
```

создаётся YouTube service автоматически.

Он содержит локальную историю статистики канала, из которой App08 строит изменения и графики за периоды:

```text
7D
30D
90D
ALL
```

Одна запись содержит дневной снимок:

```text
epoch day
subscribers
views
videos
```

Файл является **данными сервиса**, а не виджетом и не содержит YouTube API key.

### Можно ли его скачать

Да. `Download` позволяет сохранить копию локальной истории на ПК.

### Можно ли его удалить

Storage Manager технически разрешает удаление, потому что файл имеет тип `user`.

Однако при работающем YouTube service история также находится в оперативной памяти, поэтому сервис может снова создать файл при следующем сохранении. По этой причине `youtube-history.bin` не следует удалять как обычный мусорный файл. Удаление имеет смысл только при осознанном сбросе/диагностике истории.

---

## 7. `platform.cfg` и `ui-settings-screen.cfg`

Эти файлы появились в App04 как часть раннего persistent-storage контракта:

```text
/storage/platform.cfg
/storage/ui-settings-screen.cfg
```

Они сохраняются в файловой системе и поэтому могут быть видны в новых версиях платформы после обновлений, которые не стирают `/storage`.

Storage Manager показывает их как обычные `user`-файлы. Сам факт присутствия файла не означает, что каждый современный виджет обязательно использует его непосредственно.

Перед удалением старого конфигурационного файла рекомендуется сначала сохранить копию через `Download`.

---

## 8. Загрузка обычного файла в `/storage`

Storage Manager содержит:

```text
Choose file
UPLOAD FILE TO STORAGE
```

Эта функция предназначена для обычных пользовательских ресурсов и данных.

Имя файла нормализуется web-интерфейсом до безопасного набора символов.

Не использовать эту кнопку для установки активного widget JSON. Для этого существует **Filesystem Widget**.

---

## 9. Download

Для пользовательского файла кнопка `Download` обращается к маршруту вида:

```text
/storage/download?name=<имя-файла>
```

Это удобно для:

- резервного копирования конфигураций;
- выгрузки истории;
- сохранения диагностических данных;
- переноса пользовательских ресурсов на ПК.

---

## 10. Delete

`Delete` доступен только для файлов, не классифицированных как protected.

Перед удалением интерфейс запрашивает подтверждение.

Общее правило:

> если назначение файла неизвестно — сначала `Download`, затем решать вопрос об удалении.

Защищённые системные файлы через Storage Manager удалить нельзя.

---

## 11. Что переживает перезагрузку и OTA

В нормальной архитектуре платформы:

```text
NVS        -> Wi-Fi credentials, API/settings secrets
/storage   -> widget и файловые данные
app slot   -> firmware
```

Обычный reset не должен стирать `/storage`.

App-only OTA обновляет application partition и не должен стирать `/storage` или NVS.

Полный сервисный flash/merged image — отдельная операция и может иметь другой эффект в зависимости от диапазона стирания.

---

## 12. Граница Filesystem Widget и Storage Manager

Это принципиально разные функции:

| Раздел | Назначение |
|---|---|
| **Filesystem Widget** | проверить, установить, заменить или удалить активный виджет |
| **Storage Manager** | просматривать, скачивать, загружать и удалять обычные файлы `/storage` |

Коротко:

```text
Filesystem Widget = INSTALL / RUN APPLICATION
Storage Manager    = INTERNAL FILE MANAGER
```

---

## 13. Что изменится после добавления SD

Планируемое разделение интерфейса:

```text
Filesystem Widget  -> активный виджет
Storage Manager     -> внутренняя SPIFFS
SD Manager          -> внешняя SD library / data
```

Внутренняя память останется местом для системно необходимых и rescue-ресурсов, а SD сможет использоваться для библиотеки виджетов, истории, логов, изображений, CSV и других объёмных данных.

Отсутствие или неисправность SD не должно блокировать boot, системные экраны или внутренний rescue widget.

---

## 14. Безопасные действия пользователя

Рекомендуется:

- устанавливать `widget-*.json` только через **Filesystem Widget**;
- не пытаться заменить `widget.json` вручную;
- перед удалением неизвестного файла сначала скачать его;
- не хранить YouTube API key в JSON-виджетах или файлах SD;
- не считать удаление `youtube-history.bin` обычным способом очистки интерфейса;
- при проблемах сообщать firmware version, IP, имя файла, его размер и текст ошибки.

---

## 15. Текущий physical-validation статус

На реальном ESP32-8048S043 уже подтверждены:

```text
Storage enumeration                         PASS
Download/Delete для user files              PASS
widget.json protected classification        PASS
Widget installation through Widget section  PASS
Persistent widget autoload                  PASS
YouTube history file visibility             OBSERVED
```

App07 physical evidence:

```text
apps/06_OTARecovery/evidence/app07-ntp-clock-storage-manager-physical-pass.md
```

App08 добавил `youtube-history.bin` как отдельный файл данных YouTube service и расширил практическое использование внутреннего Storage Manager.
