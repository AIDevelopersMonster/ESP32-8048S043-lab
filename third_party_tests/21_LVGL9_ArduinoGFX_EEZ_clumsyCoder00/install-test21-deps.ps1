$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\..")
$projectDir = Join-Path $repoRoot ".external-test-work\21_LVGL9_ArduinoGFX_EEZ_clumsyCoder00\upstream\Sunton-ESP32-8048S043"
$libDir = Join-Path $projectDir "lib"
$pio = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"

if (-not (Test-Path $projectDir)) {
    throw "Test 21 upstream project not found. Run prepare-test21.ps1 first."
}
if (-not (Test-Path $pio)) {
    throw "PlatformIO CLI not found at $pio"
}

function Reset-DependencyDir([string]$name) {
    $dest = Join-Path $libDir $name
    if (Test-Path $dest) {
        Write-Host "[INFO] Removing existing $name dependency copy..."
        Remove-Item $dest -Recurse -Force
    }
    return $dest
}

function Clone-TaggedDependency([string]$name, [string]$url, [string]$tag) {
    $dest = Reset-DependencyDir $name
    Write-Host "[INFO] Installing $name at $tag"
    git -c http.version=HTTP/1.1 clone --depth 1 --branch $tag $url $dest
    if ($LASTEXITCODE -ne 0) { throw "Clone failed for $name" }
    $rev = (git -C $dest rev-parse HEAD).Trim()
    Write-Host "[PASS] $name -> $rev"
}

function Clone-CommitDependency([string]$name, [string]$url, [string]$commit) {
    $dest = Reset-DependencyDir $name
    Write-Host "[INFO] Installing $name at commit $commit"
    New-Item -ItemType Directory -Force $dest | Out-Null
    git -C $dest init | Out-Null
    git -C $dest remote add origin $url
    git -C $dest -c http.version=HTTP/1.1 fetch --depth 1 origin $commit
    if ($LASTEXITCODE -ne 0) { throw "Fetch failed for $name" }
    git -C $dest checkout --detach FETCH_HEAD | Out-Null
    $rev = (git -C $dest rev-parse HEAD).Trim()
    if ($rev -ne $commit) { throw "Revision mismatch for ${name}: expected $commit, got $rev" }
    Write-Host "[PASS] $name -> $rev"
}

function Disable-EezNestedLvglDependency {
    $eezDir = Join-Path $libDir "eez-framework"
    $props = Join-Path $eezDir "library.properties"
    $json = Join-Path $eezDir "library.json"

    if (Test-Path $props) {
        $filtered = Get-Content $props | Where-Object { $_ -notmatch '^\s*depends\s*=\s*lvgl' }
        $filtered | Set-Content -Encoding ASCII $props
        Write-Host "[PASS] Removed nested LVGL dependency from eez-framework/library.properties"
    }

    if (Test-Path $json) {
        $manifest = Get-Content $json -Raw | ConvertFrom-Json
        if ($manifest.PSObject.Properties.Name -contains "dependencies") {
            $manifest.PSObject.Properties.Remove("dependencies")
        }
        $manifest | ConvertTo-Json -Depth 10 | Set-Content -Encoding ASCII $json
        Write-Host "[PASS] Removed nested LVGL dependency from eez-framework/library.json"
    }

    $badProps = Select-String -Path $props -Pattern '^\s*depends\s*=.*lvgl' -ErrorAction SilentlyContinue
    $badJson = Select-String -Path $json -Pattern 'lvgl/lvgl|"dependencies"' -ErrorAction SilentlyContinue
    if ($badProps -or $badJson) {
        throw "EEZ nested LVGL dependency is still present after compatibility patch"
    }
    Write-Host "[PASS] Verified: EEZ manifests no longer request PlatformIO-managed LVGL"
}

function Reset-PlatformIoBuildState {
    $pioDir = Join-Path $projectDir ".pio"
    if (Test-Path $pioDir) {
        Write-Host "[INFO] Removing cached PlatformIO build/dependency state: $pioDir"
        Remove-Item $pioDir -Recurse -Force
    }
    if (Test-Path $pioDir) {
        throw "Failed to remove cached .pio directory"
    }
    Write-Host "[PASS] PlatformIO .pio state reset"
}

Write-Host ""
Write-Host "=== ESP32-8048S043 Lab / Test 21 dependency layer ==="
Write-Host "Project : $projectDir"
Write-Host ""

# Exact versions published by the upstream README.
Clone-TaggedDependency "Arduino_GFX" "https://github.com/moononournation/Arduino_GFX.git" "v1.4.7"
Clone-TaggedDependency "lvgl" "https://github.com/lvgl/lvgl.git" "v9.1.0"
Clone-TaggedDependency "TAMC_GT911" "https://github.com/TAMCTec/gt911-arduino.git" "v1.0.2"

# eez-framework has kept library.properties version=0.0.1 but has no corresponding Git tag.
# Pin the last framework commit available before the upstream Test 21 project snapshot date.
Clone-CommitDependency "eez-framework" "https://github.com/eez-open/eez-framework.git" "0f8e367bfa10e32340514530a77a1098e5e90ce2"

# Upstream eez-framework declares "lvgl >=8.3.0" in both Arduino and PlatformIO manifests.
# Test 21 already carries the exact upstream-requested LVGL v9.1.0 in lib/lvgl, so disable
# only the package-manager metadata in this disposable external test copy. No library source
# code, UI code or display configuration is changed.
Disable-EezNestedLvglDependency
Reset-PlatformIoBuildState

Write-Host ""
Write-Host "=== INSTALLED LIBRARY METADATA ==="
Get-ChildItem $libDir -Directory | ForEach-Object {
    $props = Join-Path $_.FullName "library.properties"
    $json = Join-Path $_.FullName "library.json"
    if (Test-Path $props) {
        $name = (Select-String -Path $props -Pattern '^name=' | Select-Object -First 1).Line
        $version = (Select-String -Path $props -Pattern '^version=' | Select-Object -First 1).Line
        Write-Host "$($_.Name): $name $version"
    }
    elseif (Test-Path $json) {
        Write-Host "$($_.Name): library.json present"
    }
}

$lvConf = Join-Path $libDir "lv_conf.h"
if (-not (Test-Path $lvConf)) {
    throw "Upstream lv_conf.h is missing from project lib directory"
}
Write-Host "[PASS] Upstream lv_conf.h remains next to lib\lvgl"

Write-Host ""
Write-Host "=== PLATFORMIO ==="
& $pio --version
Write-Host ""
Write-Host "Expected before build: no 'Installing lvgl/lvgl @ >=8.3.0' line."
Write-Host "Next command:"
Write-Host "  cd `"$projectDir`""
Write-Host "  & `"$pio`" run"
