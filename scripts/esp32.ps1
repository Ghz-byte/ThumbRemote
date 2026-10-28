param(
    [Parameter(Position = 0)]
    [ValidateSet('test', 'verify', 'upload')]
    [string]$Action = 'verify',
    [string]$Port,
    [ValidatePattern('^[a-z0-9][a-z0-9-]{0,39}$')]
    [string]$BuildId,
    [ValidateSet('touch', 'hid', 'dual')]
    [string]$Target = 'touch'
)
$ErrorActionPreference = 'Stop'
$workspaceRoot = Split-Path -Parent $PSScriptRoot
$fqbn = 'esp32:esp32:esp32c3:FlashMode=dio'
$toolchainRoot = Join-Path $env:TEMP 'thumbremote-esp32-toolchain-3.3.10-cn'
$arduinoCli = Join-Path $env:LOCALAPPDATA 'Programs/arduino-ide/resources/app/lib/backend/resources/arduino-cli.exe'
$bootApp0 = Join-Path $toolchainRoot 'packages/esp32/hardware/esp32/3.3.10-cn/tools/partitions/boot_app0.bin'
$utf8 = New-Object System.Text.UTF8Encoding($false)

function Get-SketchName([string]$ForTarget) {
    switch ($ForTarget) {
        'touch' { return 'touch_probe' }
        'hid' { return 'hid_probe' }
        'dual' { return 'dual_remote' }
        default { throw 'Unknown firmware target' }
    }
}
$sketchName = Get-SketchName $Target

function Get-SourceFiles([string]$ForTarget) {
    $name = Get-SketchName $ForTarget
    $files = @(Get-ChildItem -LiteralPath (Join-Path $workspaceRoot "firmware/$name") -File)
    if ($ForTarget -eq 'hid') {
        $files += Get-Item -LiteralPath (Join-Path $workspaceRoot 'firmware/touch_probe/touch_protocol.h')
        $files += Get-Item -LiteralPath (Join-Path $workspaceRoot 'firmware/touch_probe/gesture_recognizer.h')
    }
    if ($ForTarget -eq 'dual') {
        foreach ($sharedName in @('touch_protocol.h', 'gesture_recognizer.h', 'ble_event_protocol.h')) {
            $files += Get-Item -LiteralPath (Join-Path $workspaceRoot "firmware/touch_probe/$sharedName")
        }
        $files += Get-Item -LiteralPath (Join-Path $workspaceRoot 'firmware/hid_probe/hid_keyboard_gate.h')
    }
    if (@($files | Group-Object Name | Where-Object {$_.Count -gt 1}).Count) { throw 'Duplicate sketch input filename' }
    return $files | Sort-Object Name
}
function Get-SourceHash([string]$ForTarget = $Target) {
    $sourceText = (Get-SourceFiles $ForTarget | ForEach-Object {
        $_.Name + ':' + (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }) -join "`n"
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($hasher.ComputeHash($utf8.GetBytes($sourceText)))).Replace('-', '').ToLowerInvariant()
    } finally { $hasher.Dispose() }
}

function Test-Protocol {
    & (Join-Path $PSScriptRoot 'test_esp32_manifest.ps1')
    $compiler = (Get-Command g++ -ErrorAction Stop).Source
    $testSources = @(Get-ChildItem -LiteralPath (Join-Path $workspaceRoot 'firmware/tests') -Filter '*_test.cpp' -File | Sort-Object Name)
    if ($testSources.Count -eq 0) { throw 'No firmware tests found' }
    foreach ($testSource in $testSources) {
        $testExe = Join-Path $env:TEMP ('thumbremote-' + $testSource.BaseName + '.exe')
        & $compiler -std=c++11 -Wall -Wextra -Werror -I (Join-Path $workspaceRoot 'firmware/touch_probe') -I (Join-Path $workspaceRoot 'firmware/hid_probe') $testSource.FullName -o $testExe
        if ($LASTEXITCODE -ne 0) { throw "Host test compilation failed: $($testSource.Name)" }
        & $testExe
        if ($LASTEXITCODE -ne 0) { throw "Firmware tests failed: $($testSource.Name)" }
    }
}

if ($Action -eq 'test') { Test-Protocol; exit 0 }
if (!(Test-Path -LiteralPath $toolchainRoot) -or !(Test-Path -LiteralPath $arduinoCli)) {
    throw 'Existing Arduino CLI/toolchain mirror missing; see docs/esp32-toolchain.md'
}
$sourceHash = Get-SourceHash
$buildRoot = Join-Path $env:TEMP ('thumbremote-' + $Target + '-' + $sourceHash.Substring(0, 12) + '-targets-v1')
if ($BuildId) { $buildRoot += '-' + $BuildId }
$mirrorSketch = Join-Path $buildRoot $sketchName
$outputRoot = Join-Path $buildRoot 'build'
$manifestPath = Join-Path $buildRoot 'manifest.json'
$previousData = $env:ARDUINO_DIRECTORIES_DATA
$previousCache = $env:ARDUINO_BUILD_CACHE_PATH
$linkFlags = if ($Target -in @('hid','dual')) { '-Wl,--wrap=ble_gap_adv_start' } else { '' }
function Assert-Verified([string]$ForTarget, [string]$Hash, [string]$Root) {
    $path = Join-Path $Root 'manifest.json'
    if (!(Test-Path -LiteralPath $path)) { throw "Run verify -Target $ForTarget first" }
    $saved = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $flags = if ($ForTarget -in @('hid','dual')) { '-Wl,--wrap=ble_gap_adv_start' } else { '' }
    if ($saved.schema -ne 2 -or $saved.target -ne $ForTarget -or $saved.sourceHash -ne $Hash -or $saved.fqbn -ne $fqbn -or $saved.linkFlags -ne $flags) { throw 'Build target/source/options manifest mismatch' }
    if ($saved.bootApp0Hash -ne (Get-FileHash -LiteralPath $bootApp0).Hash) { throw 'Platform boot_app0 changed' }
    $binRoot = Join-Path $Root 'build'
    $entries = @($saved.binaries.PSObject.Properties)
    $name = Get-SketchName $ForTarget
    foreach ($required in @("$name.ino.bin", "$name.ino.bootloader.bin", "$name.ino.partitions.bin")) {
        if ($required -notin $entries.Name) { throw "Missing required binary $required" }
    }
    if (@(Get-ChildItem -LiteralPath $binRoot -File -Filter '*.bin').Count -ne $entries.Count) { throw 'Unexpected binary set' }
    foreach ($entry in $entries) {
        if ($entry.Name -notin @("$name.ino.bin", "$name.ino.bootloader.bin", "$name.ino.partitions.bin", "$name.ino.merged.bin", 'boot_app0.bin')) { throw 'Unexpected binary name in manifest' }
        if ($entry.Name -notmatch '^[a-zA-Z0-9_.-]+\.bin$' -or (Get-FileHash -LiteralPath (Join-Path $binRoot $entry.Name)).Hash -ne $entry.Value) { throw 'Binary manifest hash mismatch' }
    }
}
function Assert-BuildBinaries([string]$ForTarget, [string]$BinRoot, [switch]$Complete) {
    $name = Get-SketchName $ForTarget
    $expected = @("$name.ino.bin", "$name.ino.bootloader.bin", "$name.ino.partitions.bin", "$name.ino.merged.bin", 'boot_app0.bin')
    if (!(Test-Path -LiteralPath $BinRoot)) {
        if ($Complete) { throw 'Build output missing' }
        return
    }
    $actual = @(Get-ChildItem -LiteralPath $BinRoot -File -Filter '*.bin')
    foreach ($file in $actual) {
        if ($file.Name -notin $expected) { throw 'Unexpected existing binary; use a new -BuildId for a clean TEMP build, do not rewrite manifest' }
    }
    if ($Complete -and $actual.Count -ne $expected.Count) { throw 'Incomplete binary set from fixed SDK' }
}
function New-UploadInput([string]$ForTarget, [string]$Hash, [string]$Root) {
    Assert-Verified $ForTarget $Hash $Root
    $saved = Get-Content -LiteralPath (Join-Path $Root 'manifest.json') -Raw | ConvertFrom-Json
    $staging = Join-Path $Root ('upload-' + [Guid]::NewGuid().ToString('N'))
    [IO.Directory]::CreateDirectory($staging) | Out-Null
    foreach ($entry in $saved.binaries.PSObject.Properties) {
        $source = Join-Path (Join-Path $Root 'build') $entry.Name
        $destination = Join-Path $staging $entry.Name
        [IO.File]::WriteAllBytes($destination, [IO.File]::ReadAllBytes($source))
        if ((Get-FileHash -LiteralPath $destination).Hash -ne $entry.Value) { throw 'Upload staging hash mismatch' }
    }
    Assert-Verified $ForTarget $Hash $Root
    return $staging
}
try {
    $env:ARDUINO_DIRECTORIES_DATA = $toolchainRoot
    $env:ARDUINO_BUILD_CACHE_PATH = Join-Path $env:TEMP 'thumbremote-esp32-build-cache'
    if ($Action -eq 'verify') {
        Assert-BuildBinaries $Target $outputRoot
        Test-Protocol
        [IO.Directory]::CreateDirectory($mirrorSketch) | Out-Null
        foreach ($sourceFile in Get-SourceFiles $Target) {
            $destination = Join-Path $mirrorSketch $sourceFile.Name
            [IO.File]::WriteAllBytes($destination, [IO.File]::ReadAllBytes($sourceFile.FullName))
            if ((Get-FileHash -LiteralPath $sourceFile.FullName).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) {
                throw 'Source mirror hash mismatch'
            }
        }
        # Merged stderr is recorded in the log; native diagnostics are not PowerShell exceptions.
        $previousErrorPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        $compileArgs = @('compile', '--fqbn', $fqbn, '--build-path', $outputRoot)
        if ($linkFlags) { $compileArgs += @('--build-property', "compiler.c.elf.extra_flags=$linkFlags") }
        & $arduinoCli @compileArgs $mirrorSketch 2>&1 |
            Tee-Object -FilePath (Join-Path $buildRoot 'compile.log')
        $compileExit = $LASTEXITCODE
        $ErrorActionPreference = $previousErrorPreference
        if ($compileExit -ne 0) { throw "Arduino compile failed: $compileExit" }
        if ((Get-SourceHash) -ne $sourceHash) { throw 'Source changed during compilation; rerun verify' }
        Assert-BuildBinaries $Target $outputRoot -Complete
        $artifactHashes = @{}
        foreach ($artifact in Get-ChildItem -LiteralPath $outputRoot -File -Filter '*.bin') {
            $artifactHashes[$artifact.Name] = (Get-FileHash -LiteralPath $artifact.FullName).Hash
        }
        if ($artifactHashes.Count -lt 3) { throw 'Expected app, bootloader and partition binaries missing' }
        $manifest = @{ schema = 2; target = $Target; linkFlags = $linkFlags; sourceHash = $sourceHash; fqbn = $fqbn; binaries = $artifactHashes;
            bootApp0Hash = (Get-FileHash -LiteralPath $bootApp0).Hash }
        [IO.File]::WriteAllText($manifestPath, ($manifest | ConvertTo-Json -Depth 4), $utf8)
        Write-Output "VERIFIED build=$buildRoot source=$sourceHash"
        exit 0
    }
    if ([string]::IsNullOrWhiteSpace($Port) -or $Port -notmatch '^COM[0-9]+$') {
        throw 'upload requires explicit -Port COMn'
    }
    Assert-Verified $Target $sourceHash $buildRoot
    if ($Target -in @('hid','dual')) {
        $restoreHash = Get-SourceHash 'touch'
        $restoreRoot = Join-Path $env:TEMP ('thumbremote-touch-' + $restoreHash.Substring(0,12) + '-targets-v1')
        Assert-Verified 'touch' $restoreHash $restoreRoot
        Write-Output "RESTORE VERIFIED target=touch build=$restoreRoot"
    }
    if (!(Test-Path -LiteralPath $manifestPath)) { throw 'Run verify for the current source before uploading' }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($manifest.sourceHash -ne $sourceHash -or $manifest.fqbn -ne $fqbn) { throw 'Build manifest mismatch' }
    if ($manifest.bootApp0Hash -ne (Get-FileHash -LiteralPath $bootApp0).Hash) {
        throw 'Platform boot_app0.bin changed since verify; no upload performed'
    }
    foreach ($entry in $manifest.binaries.PSObject.Properties) {
        if ((Get-FileHash -LiteralPath (Join-Path $outputRoot $entry.Name)).Hash -ne $entry.Value) {
            throw "Binary hash mismatch: $($entry.Name)"
        }
    }
    $uploadInput = New-UploadInput $Target $sourceHash $buildRoot
    $targets = @(Get-PnpDevice -Class Ports -PresentOnly | Where-Object {
        $_.InstanceId -match 'VID_1A86&PID_55D3' -and $_.FriendlyName -match '\(COM[0-9]+\)'
    })
    if ($targets.Count -ne 1 -or $targets[0].FriendlyName -notmatch ('\(' + $Port + '\)')) {
        throw 'Expected exactly one CH343 board matching the explicit port; no upload performed'
    }
    $esptools = @(Get-ChildItem -LiteralPath (Join-Path $toolchainRoot 'packages/esp32/tools/esptool_py') -Filter 'esptool.exe' -Recurse -File)
    if ($esptools.Count -ne 1) { throw 'Cannot identify the existing esptool uniquely' }
    $chipInfo = & $esptools[0].FullName --port $Port --baud 115200 --no-stub chip-id 2>&1
    if ($LASTEXITCODE -ne 0 -or ($chipInfo -join "`n") -notmatch 'ESP32-C3') {
        throw 'ESP32-C3 identity check failed; no flash write performed'
    }
    Write-Output "Verified unique CH343 port and ESP32-C3 identity; uploading authorized target=$Target"
    $previousErrorPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $arduinoCli upload --fqbn $fqbn --port $Port --input-dir $uploadInput $mirrorSketch 2>&1 |
        Tee-Object -FilePath (Join-Path $buildRoot 'upload.log')
    $uploadExit = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorPreference
    if ($uploadExit -ne 0) { throw "Upload failed: $uploadExit" }
    Write-Output 'UPLOAD COMPLETE; serial/touch operation is not yet verified'
} finally {
    $env:ARDUINO_DIRECTORIES_DATA = $previousData
    $env:ARDUINO_BUILD_CACHE_PATH = $previousCache
}
