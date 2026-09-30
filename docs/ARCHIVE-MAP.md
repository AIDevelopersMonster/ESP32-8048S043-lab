# Archive map — 2026-09-30 consolidation

After KONTAKTS Platform 0.4.0 was accepted, all historical remote development/research branches were converted to immutable annotated tags and the branch refs were deleted.

Canonical long-lived branch:

```text
main
```

Archive namespace:

```text
archive/2026-09-30/*
```

## Archived application/platform branches

```text
archive/2026-09-30/agent/app03-live-dashboard
archive/2026-09-30/agent/app06-ota-recovery
archive/2026-09-30/agent/app07-v0.2.4-progressfix
archive/2026-09-30/agent/app07-widget-runtime
archive/2026-09-30/agent/app08-youtube-dashboard
archive/2026-09-30/agent/app09-sd-widget-library
archive/2026-09-30/agent/app10-ble-slider-sync
archive/2026-09-30/agent/app11-usb-serial-terminal
archive/2026-09-30/agent/app12-interactive-serial-terminal
archive/2026-09-30/agent/platform-foundation-next
```

## Archived LVGL / third-party / isolation research

```text
archive/2026-09-30/agent/test20-lvgl9-esp-idf-limpens
archive/2026-09-30/agent/test21-clumsycoder00
archive/2026-09-30/agent/test22-no-bounce-isolation
archive/2026-09-30/agent/test23-psram-draw-buffers-isolation
archive/2026-09-30/agent/test24-psram-plus-bounce-isolation
archive/2026-09-30/agent/test25-psram-internal-staging-isolation
archive/2026-09-30/agent/test26-psram-bounce0-pclk12-isolation
archive/2026-09-30/agent/test27-psram-bounce10-threshold
archive/2026-09-30/agent/test28-psram-bounce5-threshold
archive/2026-09-30/agent/test29-psram-bounce1-threshold
archive/2026-09-30/agent/test30-thirdparty-ffod-lovyangfx-eez
archive/2026-09-30/agent/test31-thirdparty-duck4i-native-idf
archive/2026-09-30/agent/test32-thirdparty-devany-arduinogfx-eez
archive/2026-09-30/agent/test33-thirdparty-ryanewen-esphome-lvgl
archive/2026-09-30/agent/test34-thirdparty-xoquox-esphome-lvgl
archive/2026-09-30/agent/test35-thirdparty-robot-core-display
archive/2026-09-30/agent/test36-thirdparty-halys-son-lvgl-editor
archive/2026-09-30/agent/test36b-touch-sleep16-isolation
archive/2026-09-30/agent/test36c-modern-i2c-isolation
archive/2026-09-30/agent/test36d-touch-pipeline-diagnostics
archive/2026-09-30/agent/test36e-icon-wrapper-hit-test-fix
```

## Archived closed integration branch

```text
archive/2026-09-30/feature/p4-io-rs485-widget
```

## Policy

These tags are historical evidence, not supported release lines and not merge targets.

Do not create new work from them unless reproducing a historical experiment. New development starts from current `main`. If a historical change is needed again, inspect and cherry-pick the smallest reviewed commit rather than merging an archive wholesale.
