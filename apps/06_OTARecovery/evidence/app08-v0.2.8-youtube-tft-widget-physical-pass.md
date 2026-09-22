# App08 v0.2.8 YouTube TFT Widget Physical Pass

Date: 2026-09-10  
Branch: `agent/app08-youtube-dashboard`  
Board: ESP32-8048S043 / ESP32-S3 / 800x480 / GT911

## Result

**PHYSICAL PASS** for the App08 v0.2.8 YouTube TFT/widget-runtime stage demonstrated on real hardware.

User report after installing the v0.2.8 firmware candidate and the `widget-youtube-dashboard.json` widget:

> `Все работает`

Follow-up report after recording the physical demonstration:

> `работает очень хорошо. Когда начнется статистика расскажу подробнее`

## Video evidence

YouTube Short:

https://youtube.com/shorts/lkSPy2Qc6TU

The video is accepted as physical evidence for the current TFT dashboard/widget runtime operation on the target board.

## What this closes

- App08 v0.2.8 boots and runs on the target board;
- filesystem widget installation path works with the YouTube dashboard JSON;
- the YouTube dashboard is rendered on the TFT;
- current YouTube data can be displayed through the widget bindings;
- the renderer no longer leaves the first-day chart visually blank when only one history sample exists;
- the platform shell and external widget model remain operational together.

## What remains observational rather than closed

The local history currently has only the first daily sample. Therefore this physical pass does **not** yet claim validation of a naturally accumulated multi-day statistical curve.

The following item remains a longitudinal observation gate:

```text
MULTI-DAY YOUTUBE HISTORY / REAL TREND CURVE -> WAITING FOR NATURAL DATA ACCUMULATION
```

When additional daily samples exist, record a follow-up showing that 7D/30D/90D/ALL operate on real accumulated history rather than a single-sample baseline.

## Gate status

```text
APP08 YOUTUBE API + LOCAL WEB DASHBOARD       PHYSICAL PASS
APP08 v0.2.8 TFT YOUTUBE WIDGET               PHYSICAL PASS
SINGLE-SAMPLE VISIBLE CHART BASELINE          PHYSICAL PASS
WIDGET INSTALL / PERSISTENCE PATH              PHYSICAL PASS
MULTI-DAY NATURAL HISTORY CURVE                OBSERVATION PENDING
```

This distinction preserves the project rule that a visually working first-day chart must not be overstated as proof of multi-day statistical behavior.
