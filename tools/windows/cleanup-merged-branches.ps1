param(
    [switch]$Apply
)

$ErrorActionPreference = "Stop"

$Branches = @(
    "agent/app01-six-card-serial-deck",
    "agent/app02-mixed-widgets",
    "agent/app04-storage-config",
    "agent/app05-network-provisioning",
    "agent/app05-stability",
    "agent/app12-modbus-controller",
    "agent/app14-modbus-service",
    "agent/app15-web-control",
    "agent/app16-ble-control",
    "agent/app17-mobile-control",
    "feat/app13-advanced-serial-terminal",
    "feat/app13-keyboard-overlay",
    "feat/web-flasher-platform-037",
    "fix/app12-keyboard-visibility",
    "fix/app12-launcher-tombstone",
    "fix/app12-remove-tombstone",
    "fix/app12-serial-package-keyboard",
    "fix/app13-buttonmatrix-keyboard"
)

git rev-parse --is-inside-work-tree | Out-Null
git fetch origin --prune

Write-Host ""
Write-Host "Checking branches against origin/main..."
Write-Host ""

$Safe = @()
$Blocked = @()

foreach ($Branch in $Branches) {
    git show-ref --verify --quiet "refs/remotes/origin/$Branch"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[MISSING] $Branch"
        continue
    }

    git merge-base --is-ancestor "origin/$Branch" "origin/main"
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[SAFE]    $Branch"
        $Safe += $Branch
    }
    else {
        Write-Host "[BLOCKED] $Branch is not fully contained in origin/main"
        $Blocked += $Branch
    }
}

Write-Host ""
Write-Host "Safe to remove: $($Safe.Count)"
Write-Host "Blocked:        $($Blocked.Count)"
Write-Host ""

if ($Blocked.Count -gt 0) {
    Write-Host "No blocked branch will be deleted."
}

if (-not $Apply) {
    Write-Host "Dry run only. Re-run with -Apply to delete the SAFE remote branches."
    exit 0
}

foreach ($Branch in $Safe) {
    Write-Host "Deleting origin/$Branch ..."
    git push origin --delete $Branch
}

Write-Host ""
Write-Host "Remote branch cleanup complete."
