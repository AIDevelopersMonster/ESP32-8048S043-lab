# Productization programme — research lab to user project

Status: **ACTIVE CLEANUP AFTER PLATFORM 0.4.0 ACCEPTANCE**  
Baseline: **KONTAKTS Platform 0.4.0 / Sample A / PHYSICAL + WEB PASS**

This programme turns the repository from an evidence-heavy engineering laboratory into a repository that is easy for a user to install, operate, update and recover, while preserving enough research history for reproducibility.

## Target state

The public repository should have one obvious path for each audience:

1. **User** — install firmware, install SD content, open the Web UI, use the controller.
2. **Integrator** — wire UART1/RS485 devices, use the command/service interfaces, understand ownership and safety rules.
3. **Developer** — build the canonical platform, SD packages, Android client and host tools.
4. **Research/archive reader** — inspect historical evidence without seeing dozens of active branches.

The default branch is the product truth. Historical experiments must not look like active development.

## Phase 0 — freeze the accepted baseline

Completed for 0.4.0:

- App18 Climate Controller merged to `main`;
- firmware CI PASS;
- exact accepted OTA image published;
- full image published for first install / Web Flasher;
- SD Climate Controller package published;
- local `/climate` Web control physically checked;
- OTA candidate confirmed `VALID`;
- Web Flasher catalog and GitHub Pages deployment PASS.

No rebuild may be called equivalent to the accepted image unless its digest is checked or it receives a new acceptance record.

## Phase 1 — repository hygiene

### Branch policy

Final product state:

```text
main    canonical and only long-lived branch
```

Short-lived feature/fix branches are deleted after merge.

Old divergent research branches are not kept as active branches. If their unique commit history has evidence value, preserve the tip with an immutable archive tag, record it in `docs/ARCHIVE-MAP.md`, then delete the branch ref.

The one-time cleanup is automated by:

```text
tools/windows/finalize-repository.ps1
```

The script is dry-run by default and refuses to continue if a historical branch has moved from the audited SHA.

### Tracked-file cleanup

Remove:

- empty placeholder directories represented only by `.gitkeep`;
- stale "no release yet" documents;
- unreferenced Web Flasher manifests for superseded lab builds;
- generated build products that belong in GitHub Releases or CI artifacts;
- duplicate temporary instructions whose canonical content now exists elsewhere.

Keep:

- source code;
- current release manifests and checksums;
- hardware evidence needed to reproduce claims;
- concise acceptance records;
- third-party attribution/reproduction notes where licensing and provenance matter.

## Phase 2 — user-first entry points

The repository front page must answer, in this order:

1. What hardware is supported?
2. What is the current stable version?
3. How do I install it?
4. How do I install SD applications?
5. How do I open the Web UI?
6. How do I update by OTA?
7. How do I recover?
8. Where are developer and research details?

Canonical user document:

```text
docs/QUICKSTART.md
```

The Web Flasher is the preferred fresh-install path. GitHub Releases remain the authoritative artifact store.

## Phase 3 — distribution model

Public channels are intentionally separate:

| Channel | Purpose |
|---|---|
| Platform release | full image + OTA image + OTA JSON + SHA-256 |
| Web Flasher | browser installation of the current full image |
| SD current | complete SD bundle + individual widget ZIPs |
| Android release | APK + SHA-256 |
| Host tools | source or packaged tool, with version and usage notes |

The repository itself should not become a binary warehouse. Generated packages should normally be attached to Releases or CI artifacts.

## Phase 4 — canonical source model

Do not perform a risky mass rename in the 0.4.0 stabilization commit. Existing CI paths under `apps/` remain valid.

For future development, treat these as the canonical product sources:

```text
apps/06_OTARecovery       platform core
apps/09_SDWidgetLibrary   SD distribution
apps/17_MobileControl     Android client
apps/18_ClimateController climate service/HMI documentation
libraries/ESP32_8048S043  Arduino line
tools/                    desktop/service tools
web-flasher/              browser installer
```

Older numbered applications are laboratory history, not separate current products. They may stay in source history until a dedicated 0.5.x layout migration proves that moving them does not break CI, links, videos or downstream users.

## Phase 5 — documentation hardening

Required maintained documents:

- `README.md` — user-facing front page;
- `docs/QUICKSTART.md` — installation and first use;
- `RELEASES.md` — release channels and acceptance rules;
- `docs/RELEASE-ASSET-INVENTORY.md` — exact current digests;
- `SECURITY.md` — trusted-network boundary and reporting;
- `ROADMAP.md` — optional future work, not release blockers;
- `docs/ARCHIVE-MAP.md` — research refs preserved as tags.

Historical detail belongs in per-application READMEs, evidence records and Git history, not in the first screen seen by a new user.

## Phase 6 — GitHub project conventions

- `main` must remain buildable.
- Pull requests must pass the PR gate.
- Product releases use semantic versions.
- Mutable channels such as `app09-sd-current` remain prereleases so they do not steal repository-wide `latest`.
- A release is promoted only after the claimed physical path is tested.
- Bug reports and feature requests use GitHub issue forms.
- Security-sensitive reports follow `SECURITY.md`.

## Phase 7 — acceptance definition for "productized"

The transition is complete when:

- only `main` remains as a long-lived branch;
- historical unique branch tips are reachable through archive tags or deliberately discarded;
- no stale placeholder or generated temporary files remain tracked;
- README and release docs point to 0.4.0;
- Web Flasher exposes the current platform without obsolete intermediate entries;
- SD library exposes Climate Controller;
- a new user can install, configure and recover using one documented path;
- current security limitations are explicit;
- remaining roadmap items are non-blocking enhancements.

## Non-claims

Platform 0.4.0 is a physically validated laboratory/product baseline for the documented hardware. It is not a statement of industrial certification, public-Internet security, universal compatibility with every ESP32-8048S043 revision, or guaranteed safe actuation when the physical RS485/relay path itself is unavailable.
