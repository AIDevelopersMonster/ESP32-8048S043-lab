# Roadmap

Platform 0.4.0 closes the current research-to-product milestone. Items below are future improvements, not blockers for the accepted release.

## Near term

- simplify user-facing Web Flasher choices to current / previous / recovery only;
- perform a browser fresh-install retest of the 0.4.0 full image;
- complete the dedicated MA01 communication-loss bench test when the stand is rewired;
- improve Android state parsing and reconnect behaviour;
- add optional HTTP/BLE authentication design;
- add longer-duration coexistence tests.

## Medium term

- define a stable external-device/provider extension API;
- rationalize historical numbered application directories without breaking links or CI;
- package desktop tools through Releases rather than committed ZIP snapshots;
- improve automated release-note and checksum validation;
- add installer screenshots and concise troubleshooting flow.

## Long term

- production security model;
- signed update strategy;
- multi-device/multi-client stress testing;
- broader board-revision compatibility matrix;
- hardware watchdog/interlock reference design;
- formal release qualification profiles for non-laboratory deployments.

## Rule

New work starts from `main` in a short-lived branch and is removed after merge. Research branches must not remain indefinitely as active refs; preserve important tips with archive tags instead.
