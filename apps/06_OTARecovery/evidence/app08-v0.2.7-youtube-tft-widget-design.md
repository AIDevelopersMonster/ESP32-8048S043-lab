# App08 v0.2.7 - YouTube TFT widget candidate

Date: 2026-09-09
Branch: `agent/app08-youtube-dashboard`
Candidate firmware: `0.2.7`
Status: SOURCE CANDIDATE / CI + PHYSICAL VALIDATION REQUIRED

## Architecture

App08 keeps the platform shell firmware-resident and moves the end-user dashboard into the filesystem widget layer.

```text
firmware shell
  STATUS
  OTA
  WIDGET
     |
     +-- /storage/widget.json
             |
             +-- YouTube labels
             +-- YouTube charts
             +-- 7D / 30D / 90D / ALL
             +-- REFRESH
```

The YouTube API service remains firmware-resident. The widget contains only layout, bindings and allowed actions. API credentials remain in NVS and are not present in widget JSON.

## Candidate widget set

- `widget-youtube-dashboard.json` - default dual-chart dashboard.
- `widget-youtube-views.json` - large views-history chart.
- `widget-youtube-subscribers.json` - large subscriber-history chart.

All three use the same service and can be exchanged without reflashing firmware.

## Default 800x480 presentation

The default dashboard uses the 744x344 filesystem-widget content area:

```text
+--------------------------------------------------------------+
| CHANNEL                         API STATE        PERIOD       |
| SUBSCRIBERS        VIEWS                     VIDEOS          |
| DELTA SUB          DELTA VIEWS                                |
|                                                              |
| VIEWS TREND                  | SUBSCRIBERS TREND              |
| [line chart]                 | [line chart]                   |
|                                                              |
|  7D   30D   90D   ALL          REFRESH                        |
+--------------------------------------------------------------+
```

The firmware top bar remains available above this area with STATUS / OTA / WIDGET navigation.

If a valid filesystem widget is installed and the running image is not `PENDING_VERIFY`, the platform now opens WIDGET automatically at boot. A pending OTA candidate still opens OTA first so recovery/confirm controls take priority.

## Runtime extension

The renderer now consumes:

```text
youtube.subscribers
youtube.views
youtube.videos
youtube.views_delta
youtube.subscribers_delta
youtube.channel
youtube.state
youtube.period

youtube.views_history
youtube.subscribers_history
```

Allowed widget actions:

```text
youtube_refresh
youtube_period_7d
youtube_period_30d
youtube_period_90d
youtube_period_all
```

Chart point arrays are allocated in PSRAM when available. This is intentional because App06/App08 still carries an open TLS largest-internal-block hardening item; the graph should not consume several kilobytes of internal RAM merely to retain history points.

## Physical acceptance gate

Do not mark the TFT dashboard PHYSICAL PASS until the board proves all of the following:

1. `0.2.7` boots stably with the existing Wi-Fi and API-key NVS data preserved.
2. `widget-youtube-dashboard.json` installs through the existing widget upload path.
3. the WIDGET page renders channel, subscribers, views and videos from the live service;
4. both charts render without corruption;
5. 7D / 30D / 90D / ALL changes the period and redraws the charts;
6. REFRESH queues a live YouTube API update;
7. navigation to STATUS and OTA and back to WIDGET remains stable;
8. reboot autoloads the persisted dashboard and opens WIDGET automatically;
9. serial log shows no Guru Meditation, stack canary or PSRAM/DMA regression;
10. largest internal free block remains sufficient for the already validated HTTPS OTA path.

A single stored day is expected to produce a zero-baseline/one-point history. That is not a chart failure; multi-day slope validation requires accumulated daily samples.
