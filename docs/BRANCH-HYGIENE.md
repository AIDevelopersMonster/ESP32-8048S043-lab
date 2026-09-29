# Branch hygiene — 2026-09-29

This file records the post-Platform-0.3.9 branch disposition. It is intentionally conservative: Git history is kept when a branch contains unique experimental commits, even if the corresponding feature has been superseded on `main`.

## Rules

- **KEEP** — active canonical development branch or branch whose unique history is still directly useful.
- **ARCHIVE** — superseded experiment/research branch with unique commits not fully contained in `main`; do not merge blindly.
- **DELETE CANDIDATE** — branch tip is fully contained in `main`; safe to remove after a final human check because its commits remain reachable from `main`.
- **CLOSED PR / ARCHIVE** — obsolete integration branch whose PR is closed but unique history is retained.

No branch deletion is required for correctness. Deleting a branch only removes the ref, not commits already reachable from `main`.

## Canonical

```text
main                                      KEEP / CANONICAL
```

## Fully contained in main — delete candidates

The comparison on 2026-09-29 showed no branch-only commits for these refs:

```text
agent/app01-six-card-serial-deck
agent/app02-mixed-widgets
agent/app04-storage-config
agent/app05-network-provisioning
agent/app05-stability
agent/app12-modbus-controller
agent/app14-modbus-service
agent/app15-web-control
agent/app16-ble-control
agent/app17-mobile-control
feat/app13-advanced-serial-terminal
feat/app13-keyboard-overlay
feat/web-flasher-platform-037
fix/app12-keyboard-visibility
fix/app12-launcher-tombstone
fix/app12-remove-tombstone
fix/app12-serial-package-keyboard
fix/app13-buttonmatrix-keyboard
```

These branches are **DELETE CANDIDATE**, not active development lines.

## Superseded application branches with unique history — archive

```text
agent/app03-live-dashboard
agent/app06-ota-recovery
agent/app07-v0.2.4-progressfix
agent/app07-widget-runtime
agent/app08-youtube-dashboard
agent/app09-sd-widget-library
agent/app10-ble-slider-sync
agent/app11-usb-serial-terminal
agent/app12-interactive-serial-terminal
agent/platform-foundation-next
```

These refs diverge from current `main` and retain branch-only commits. Keep them as **ARCHIVE** until their unique commits are either deliberately documented/cherry-picked or judged disposable.

## LVGL / third-party / isolation research — archive

The following branches are experimental evidence, not product branches:

```text
agent/test20-lvgl9-esp-idf-limpens
agent/test21-clumsycoder00
agent/test22-no-bounce-isolation
agent/test23-psram-draw-buffers-isolation
agent/test24-psram-plus-bounce-isolation
agent/test25-psram-internal-staging-isolation
agent/test26-psram-bounce0-pclk12-isolation
agent/test27-psram-bounce10-threshold
agent/test28-psram-bounce5-threshold
agent/test29-psram-bounce1-threshold
agent/test30-thirdparty-ffod-lovyangfx-eez
agent/test31-thirdparty-duck4i-native-idf
agent/test32-thirdparty-devany-arduinogfx-eez
agent/test33-thirdparty-ryanewen-esphome-lvgl
agent/test34-thirdparty-xoquox-esphome-lvgl
agent/test35-thirdparty-robot-core-display
agent/test36-thirdparty-halys-son-lvgl-editor
agent/test36b-touch-sleep16-isolation
agent/test36c-modern-i2c-isolation
agent/test36d-touch-pipeline-diagnostics
agent/test36e-icon-wrapper-hit-test-fix
```

Disposition: **ARCHIVE / KEEP FOR RESEARCH HISTORY**.

These branches intentionally preserve controlled-variable tests and third-party reproduction work. Do not bulk-delete them merely because the current platform has moved on.

## Closed integration branch with unique commits

```text
feature/p4-io-rs485-widget
```

PR #11 is closed because Platform 0.3.9 supersedes it. The branch still contains unique commits, so its disposition is **CLOSED PR / ARCHIVE**, not delete candidate.

## Closed pull requests

The following stale PRs were closed during the 2026-09-29 consolidation:

```text
#8  Agent/app09 sd widget library
#10 Agent/app12 interactive serial terminal
#11 Platform 0.3.8: P4 UART1/RS-485 and GPIO output service
```

Their useful current functionality is already represented on `main`; their branch history remains available.

## Recommended maintenance

1. New work starts from current `main`.
2. Product branches should be short-lived and removed after merge once fully contained.
3. Research/isolation branches may remain long-lived, but their role should be documented here.
4. Never merge an archived divergent branch wholesale into `main`; inspect/cherry-pick only the intended commit(s).
5. Re-run this audit before the next major platform release.
