# App09 SD tab UX — v0.3.2

## Goal

The SD tab is a user-facing application library, not a storage/debug screen.

The normal user surface should answer only three questions:

1. Is the SD card ready?
2. What applications are available?
3. How do I refresh/open one?

## Target layout

```text
APPLICATIONS

SD card ready                                  [ REFRESH ]
Available: N

[ application entry ]
[ application entry ]
[ application entry ]
[ application entry ]
```

## Removed from the normal SD tab

The following remain useful for logs, diagnostics, SYS, or developer tools, but should not occupy the application-library screen:

- card model/name;
- capacity;
- SPI frequency;
- `/sd/widgets` path;
- `/sd/UPDATE` path;
- manifest terminology;
- internal-storage installation wording;
- raw error/status codes;
- implementation notes about flash writes.

## Rules

- `SD` means **Applications** to the user.
- `REFRESH` performs mount/rescan internally without exposing that mechanism in the button label.
- The application list receives most of the screen area.
- Launcher entries show the entrypoint's user-facing name only; package/path implementation details are hidden.
- Detailed failures still go to ESP-IDF logs; the UI shows a short human-readable failure message.
- Offline firmware update remains a platform/SYS concern and is not advertised as a filesystem path on the application-library screen.
