# RGB display panel

The 800x480 panel is the central subsystem. Unlike SPI/I80 displays, RGB panels need careful timing, buffering and PSRAM strategy.

Initial validation path:

1. backlight only;
2. simple solid colors;
3. RGB bars;
4. edge frame;
5. orientation target;
6. flicker/tearing observation;
7. LVGL partial-buffer test;
8. optional larger buffer tests.

No full-framebuffer or double-buffer claim is made yet.
