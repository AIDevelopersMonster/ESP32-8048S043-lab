# KONTAKTS Platform — комплект эксплуатационной и программной документации

Статус: **living documentation / структура зафиксирована**.

Эта папка разделяет документацию по ролям, чтобы системные изменения платформы, прикладная логика виджетов и действия конечного пользователя не смешивались.

## Документы

1. [`system-programmer-guide.md`](system-programmer-guide.md) — **Руководство системного программиста**.
   Для разработчика firmware/platform layer: ESP-IDF, boot, partitions, factory/recovery, OTA, Wi-Fi, SPIFFS, LVGL/GT911, CI/release, диагностика памяти и физические acceptance-тесты.

2. [`application-programmer-guide.md`](application-programmer-guide.md) — **Руководство прикладного (функционального) программиста**.
   Для разработчика виджетов, web UI, browser-side logic, JSON schema, bindings/actions, прикладных API и контента файловой системы без изменения boot/recovery ядра.

3. [`user-guide.md`](user-guide.md) — **Руководство пользователя**.
   Для эксплуатации устройства: первый запуск, Wi-Fi, STATUS/OTA/WIDGET, обновление, загрузка виджетов, восстановление и типовые неисправности.

4. [`storage-manager.md`](storage-manager.md) — **Storage Manager — руководство пользователя**.
   Объясняет браузерный файловый менеджер внутреннего `/storage`: таблицу `Name / Size / Type / Action`, Download/Delete/Upload, защиту `widget.json`, назначение `platform.cfg`, `ui-settings-screen.cfg` и `youtube-history.bin`, а также принципиальную границу между Storage Manager и установщиком Filesystem Widget.

5. [`widget-user-section-template.md`](widget-user-section-template.md) — шаблон пользовательского раздела для каждого нового виджета.

## Граница ответственности

```text
SYSTEM PROGRAMMER
    firmware / boot / partitions / drivers / OTA / recovery / filesystem engine
                    |
                    v
APPLICATION PROGRAMMER
    widgets / JSON schema / bindings / actions / web UI / application APIs
                    |
                    v
USER
    network / update / widget install / storage management / normal operation / recovery actions
```

## Пользовательская граница Widget / Storage

```text
Filesystem Widget = INSTALL / RUN APPLICATION
Storage Manager    = INTERNAL FILE MANAGER
```

Активный внешний виджет сохраняется как `/storage/widget.json`, но устанавливается только через валидирующий раздел **Filesystem Widget**. В Storage Manager этот файл виден как `system / protected`.

## Правило обновления документации

- изменения partition table, boot, rollback, factory, TLS/heap, drivers или firmware-resident shell обязательно отражаются в руководстве системного программиста;
- новые типы объектов, bindings, actions, web/API возможности и правила JSON — в руководстве прикладного программиста;
- новое действие, которое должен выполнять пользователь, — в руководстве пользователя;
- изменения браузерного `/storage`, типов файлов, protection/download/delete/upload или будущей границы внутренней памяти и SD — также отражаются в `storage-manager.md`;
- каждый законченный прикладной виджет получает отдельный пользовательский раздел по шаблону, без разрастания основной главы платформы.
