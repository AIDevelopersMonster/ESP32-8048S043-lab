param(
    [switch]$Apply,
    [switch]$ArchiveUnique
)

$ErrorActionPreference = "Stop"

# One-time repository consolidation checkpoint after KONTAKTS Platform 0.4.0.
# Dry-run by default. The audited SHA protects against deleting a branch that moved
# after this file was created.
$Expected = [ordered]@{
    "agent/app03-live-dashboard"                 = "cb32a5b75605aa2f111ce4868199535f02458892"
    "agent/app06-ota-recovery"                   = "448f1c45c773840dfb58d2546a064bc63ee5e2a0"
    "agent/app07-v0.2.4-progressfix"              = "87e40bbb34c3de207207a4a9f35dc6bbfe15a7c7"
    "agent/app07-widget-runtime"                  = "16c7360c8641cf4dd21e465aec8e15a1fd949472"
    "agent/app08-youtube-dashboard"               = "8f1171290f43bfcc768665cace8383dd2570ff3d"
    "agent/app09-sd-widget-library"               = "60af1b20f0f57cfd17e431b62127c5730a914c55"
    "agent/app10-ble-slider-sync"                 = "03a4fcd1b10e18709d7df1e33cad55212bd053fe"
    "agent/app11-usb-serial-terminal"             = "de4233ff712a91eae4e5ca57430a967cf0eba4ec"
    "agent/app12-interactive-serial-terminal"     = "bbc003def7387f670c0df2a1650ca9ec4024087a"
    "agent/platform-foundation-next"              = "925e5cabe7c397d604bdf91a6a96d68b66fccec8"
    "agent/test20-lvgl9-esp-idf-limpens"          = "33819b3c8c746e5852618d8b28cee8a776529f89"
    "agent/test21-clumsycoder00"                  = "635b0961e872fefa7d07d6f57e4b8e8b7bc3a1d2"
    "agent/test22-no-bounce-isolation"            = "d697d958f51b0c6b91a4349f87a6581a8acb3228"
    "agent/test23-psram-draw-buffers-isolation"   = "67b0b4f48f4c76f890bebc3eacc60c7bf2948240"
    "agent/test24-psram-plus-bounce-isolation"    = "1376063399b373a6f2ee328ad352e0b3607a87e4"
    "agent/test25-psram-internal-staging-isolation" = "1a57740c26fe53a57c5f5f1843ee6498fce5c68f"
    "agent/test26-psram-bounce0-pclk12-isolation" = "da4852c1f302551adfcfbe185a8f49faeba12ab1"
    "agent/test27-psram-bounce10-threshold"        = "2a4ab49b2703198bb7dda833da00fa9339790f95"
    "agent/test28-psram-bounce5-threshold"         = "58bf9b74c1ff46593c4f7ab804bd0d8ec3ef7a72"
    "agent/test29-psram-bounce1-threshold"         = "856eae71c738b50cf59b7efd0f225a00b9a9ad89"
    "agent/test30-thirdparty-ffod-lovyangfx-eez"   = "7edcf013652210d5c5202cd7c7d836d1a4e198d9"
    "agent/test31-thirdparty-duck4i-native-idf"    = "9fe7d46cc1c4e32804a80ddd1991246ac2bed9e6"
    "agent/test32-thirdparty-devany-arduinogfx-eez" = "03de05a940ac80b95ccf5107dd1dfc72bea0895b"
    "agent/test33-thirdparty-ryanewen-esphome-lvgl" = "a41aab5cc9f8598650a5c49eefdcb85f500fafd3"
    "agent/test34-thirdparty-xoquox-esphome-lvgl" = "4d9ad247856953bfde658003dbe1b618576ecdbc"
    "agent/test35-thirdparty-robot-core-display"   = "1412bd950635037d9ce392a01a3d85fa109c1d0b"
    "agent/test36-thirdparty-halys-son-lvgl-editor" = "d54fb5a990ef650bd3fb283b879b33dae4fca709"
    "agent/test36b-touch-sleep16-isolation"        = "baae9380eb72b635e3465d92b15bc044ba355f07"
    "agent/test36c-modern-i2c-isolation"           = "9824f7181bea336b6ac636e04619f9b9dd522c90"
    "agent/test36d-touch-pipeline-diagnostics"     = "cb7bf0d07ede75601fd03c8e0a1c65f890d30393"
    "agent/test36e-icon-wrapper-hit-test-fix"      = "b0df28579f006d385d6c8064185e4d43bdaa4163"
    "feature/p4-io-rs485-widget"                  = "408be1503e658e18c22cea63a593bf63066543c5"
}

function ArchiveTagName([string]$Branch) {
    return "archive/2026-09-30/" + ($Branch -replace "[^A-Za-z0-9._/-]", "-")
}

git rev-parse --is-inside-work-tree | Out-Null
git fetch origin --prune --tags

$main = (git rev-parse origin/main).Trim()
Write-Host "origin/main = $main"
Write-Host ""

$SafeDelete = @()
$ArchiveDelete = @()
$Blocked = @()

foreach ($Branch in $Expected.Keys) {
    $RemoteRef = "refs/remotes/origin/$Branch"
    git show-ref --verify --quiet $RemoteRef
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[MISSING] $Branch"
        continue
    }

    $Actual = (git rev-parse "origin/$Branch").Trim()
    $Want = $Expected[$Branch]
    if ($Actual -ne $Want) {
        Write-Host "[BLOCKED] $Branch moved: expected $Want actual $Actual"
        $Blocked += $Branch
        continue
    }

    git merge-base --is-ancestor "origin/$Branch" "origin/main"
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[DELETE]  $Branch (fully contained in main)"
        $SafeDelete += $Branch
    }
    else {
        $Tag = ArchiveTagName $Branch
        Write-Host "[ARCHIVE] $Branch -> $Tag @ $Actual"
        $ArchiveDelete += [pscustomobject]@{ Branch=$Branch; Sha=$Actual; Tag=$Tag }
    }
}

Write-Host ""
Write-Host "Contained branches: $($SafeDelete.Count)"
Write-Host "Unique branches:    $($ArchiveDelete.Count)"
Write-Host "Blocked/moved:      $($Blocked.Count)"

if (-not $Apply) {
    Write-Host ""
    Write-Host "Dry run only."
    Write-Host "Use -Apply -ArchiveUnique to archive unique tips as tags and delete all audited remote branches."
    exit 0
}

if ($Blocked.Count -gt 0) {
    throw "Refusing cleanup because one or more audited branches moved."
}

if ($ArchiveDelete.Count -gt 0 -and -not $ArchiveUnique) {
    throw "Unique branch tips exist. Re-run with -ArchiveUnique to preserve them as archive tags before deletion."
}

foreach ($Item in $ArchiveDelete) {
    git show-ref --tags --verify --quiet "refs/tags/$($Item.Tag)"
    if ($LASTEXITCODE -ne 0) {
        git tag -a $Item.Tag $Item.Sha -m "Archived research branch $($Item.Branch) at Platform 0.4.0 repository consolidation."
        git push origin "refs/tags/$($Item.Tag)"
    }
    else {
        $TagSha = (git rev-list -n 1 $Item.Tag).Trim()
        if ($TagSha -ne $Item.Sha) {
            throw "Archive tag $($Item.Tag) already exists but points to $TagSha instead of $($Item.Sha)."
        }
    }
}

foreach ($Branch in $SafeDelete) {
    git push origin --delete $Branch
}

foreach ($Item in $ArchiveDelete) {
    git push origin --delete $Item.Branch
}

git fetch origin --prune --tags

Write-Host ""
Write-Host "Repository branch consolidation complete."
Write-Host "Expected long-lived remote branch: main"
Write-Host "Historical unique tips are preserved under archive/2026-09-30/* tags."
