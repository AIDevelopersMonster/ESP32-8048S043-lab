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

4. [`widget-user-section-template.md`](widget-user-section-template.md) — шаблон пользовательского раздела для каждого нового виджета.

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
    network / update / widget install / normal operation / recovery actions
```

## Правило обновления документации

- изменения partition table, boot, rollback, factory, TLS/heap, drivers или firmware-resident shell обязательно отражаются в руководстве системного программиста;
- новые типы объектов, bindings, actions, web/API возможности и правила JSON — в руководстве прикладного программиста;
- новое действие, которое должен выполнять пользователь, — в руководстве пользователя;
- каждый законченный прикладной виджет получает отдельный пользовательский раздел по шаблону, без разрастания основной главы платформы.
