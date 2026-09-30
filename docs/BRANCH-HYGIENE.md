# Branch hygiene — Platform 0.4.0 consolidation

The repository has completed the App18 / Platform 0.4.0 research-to-product milestone. The one-time branch consolidation was executed successfully: all 32 historical divergent branches were preserved as annotated archive tags and their remote branch refs were removed.

## Final branch policy

The long-lived branch set is now:

```text
main
```

All product/fix branches should be short-lived and deleted after merge.

Historical branches that still contain unique experimental commits are not a reason to keep an active branch list indefinitely. Their useful tips should be preserved as immutable archive tags, then the branch refs should be deleted.

Archive tag namespace:

```text
archive/2026-09-30/<former-branch-name>
```

## One-time consolidation

Use:

```powershell
.\tools\windows\cleanup-merged-branches.ps1
```

for a dry run.

Then, only after reviewing the output:

```powershell
.\tools\windows\cleanup-merged-branches.ps1 -Apply -ArchiveUnique
```

The script:

1. fetches current remote refs and tags;
2. checks every audited branch against the exact SHA recorded at the 0.4.0 checkpoint;
3. blocks cleanup if any branch moved;
4. deletes branches already fully contained in `main`;
5. creates an annotated archive tag for each divergent branch with unique commits;
6. pushes the archive tags;
7. deletes the corresponding remote branches;
8. prunes local remote-tracking refs.

This preserves unique research history without presenting historical experiments as active development.

## Why tags instead of long-lived research branches

Branches imply ongoing work and create navigation noise. Archive tags provide a stable pointer to a historical state without suggesting that it should be merged or maintained.

Git commit history, evidence documents and per-application READMEs remain the primary historical record.

## Future rule

- New work starts from current `main`.
- Use a short-lived `feature/*`, `fix/*` or equivalent branch.
- Merge through a PR after CI.
- Delete the branch after merge.
- If an abandoned research branch contains uniquely valuable evidence, archive its exact tip as a tag before deletion.
- Never merge an old divergent archive wholesale into `main`; cherry-pick only a deliberately reviewed change.

## Completed consolidation

Remote branch audit after cleanup:

```text
main
```

Historical refs are indexed in `docs/ARCHIVE-MAP.md`.
