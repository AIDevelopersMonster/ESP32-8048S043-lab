# Sample A factory LVGL Widgets touchscreen visual PASS

Status: `PHYSICAL TOUCHSCREEN VISUAL PASS`.

Video evidence:

```text
https://youtube.com/shorts/XVaWqrtXHE4
```

This note records a visual runtime check of the physical Sample A board using the preserved factory application. The test is performed on the standard LVGL 8 widgets demo (`lv_demo_widgets`).

## Observed behavior

The visual check confirms:

- the board boots into the factory LVGL 8 widgets demo;
- the 4.3-inch 800x480 RGB display renders the demo correctly;
- orientation appears visually correct;
- RGB color order appears visually correct;
- complex LVGL widgets, charts, images and fonts render normally;
- the touchscreen responds to user interaction in the demo;
- the board passes a practical first visual GUI check.

## Video description

Suggested public video description:

```text
В этом коротком видео показываю визуальную проверку работы платы ESP32-8048S043 с 4.3-дюймовым дисплеем 800x480 и сенсорным экраном.

На плате запускается заводская демонстрационная прошивка с LVGL 8 Widgets Demo. На экране видны стандартные элементы демо: вкладки, графики, профиль, изображения, шрифты и индикатор производительности. Это хороший практический тест не просто подсветки или цветных полос, а полноценного графического интерфейса LVGL.

Что проверено визуально:
- дисплей запускается;
- картинка выводится в полном разрешении 800x480;
- ориентация выглядит правильной;
- цвета/RGB-порядок выглядят корректными;
- LVGL 8 Widgets Demo работает стабильно;
- сложные виджеты и графики отрисовываются нормально;
- тачскрин реагирует на касания;
- плата проходит первичную визуальную проверку GUI.

Проект ESP32-8048S043 Lab:
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

Важно: это визуальная runtime-проверка дисплея и тачскрина на заводском LVGL demo. Полная аппаратная валидация pinout, touch controller, SD, Wi-Fi/BLE и BSP выполняется отдельными тестами.
```

## Interpretation

This raises the practical runtime evidence level from display-only to:

```text
Factory LVGL Widgets Demo display + touchscreen visual interaction = PASS
```

## Boundary

This is a visual touchscreen runtime PASS, not a controller-identification PASS.

Still not proven by this evidence alone:

- exact touch controller model;
- GT911 / Goodix identity;
- I2C address;
- exact SDA/SCL/INT/RST pin mapping;
- coordinate calibration quality under a dedicated target test;
- multi-touch behavior;
- SD card behavior;
- Wi-Fi/BLE behavior;
- final BSP pinout.

A dedicated touch scan and coordinate-target test should still be added before marking the touch controller and BSP as fully verified.
