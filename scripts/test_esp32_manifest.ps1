param([string]$SourceScript = (Join-Path $PSScriptRoot 'esp32.ps1'))
$ErrorActionPreference = 'Stop'
# Exercise the actual upload guard without evaluating the script's device/build entry point.
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($SourceScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Build script parse errors' }
$function = $ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Assert-Verified'}, $true) | Select-Object -First 1
if (!$function) { throw 'Upload guard not found' }
Invoke-Expression $function.Extent.Text
foreach ($helperName in @('Get-SketchName','New-UploadInput','Assert-BuildBinaries')) {
    $helper = $ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $helperName}, $true) | Select-Object -First 1
    if (!$helper) { throw "Required build helper missing: $helperName" }
    Invoke-Expression $helper.Extent.Text
}
$fixtureRoot = Join-Path $env:TEMP ('thumbremote-manifest-test-' + [Guid]::NewGuid().ToString('N'))
$binRoot = Join-Path $fixtureRoot 'build'
[IO.Directory]::CreateDirectory($binRoot) | Out-Null
$bootApp0 = Join-Path $fixtureRoot 'boot_app0.fixture.bin'
[IO.File]::WriteAllBytes($bootApp0, [byte[]]@(9))
$fqbn = 'esp32:esp32:esp32c3:FlashMode=dio'
$sourceHash = 'a' * 64
$binaries = @{}
foreach ($name in @('touch_probe.ino.bin', 'touch_probe.ino.bootloader.bin', 'touch_probe.ino.partitions.bin')) {
    $path = Join-Path $binRoot $name
    [IO.File]::WriteAllBytes($path, [byte[]]@(1,2,3))
    $binaries[$name] = (Get-FileHash -LiteralPath $path).Hash
}
$manifest = @{schema=2; target='touch'; sourceHash=$sourceHash; fqbn=$fqbn; linkFlags=''; binaries=$binaries;
    bootApp0Hash=(Get-FileHash -LiteralPath $bootApp0).Hash}
function Save-Manifest { [IO.File]::WriteAllText((Join-Path $fixtureRoot 'manifest.json'), ($manifest | ConvertTo-Json -Depth 4)) }
$checks = 0
function Expect-Rejected {
    $rejected = $false
    try { Assert-Verified 'touch' $sourceHash $fixtureRoot } catch { $rejected = $true }
    if (!$rejected) { throw 'Invalid manifest/artifact was accepted' }
    $script:checks++
}
Save-Manifest
Assert-Verified 'touch' $sourceHash $fixtureRoot; $checks++
$manifest.target='hid'; Save-Manifest; Expect-Rejected
$manifest.target='touch'; $manifest.schema=1; Save-Manifest; Expect-Rejected
$manifest.schema=2; $manifest.sourceHash='b'*64; Save-Manifest; Expect-Rejected
$manifest.sourceHash=$sourceHash; $manifest.linkFlags='unexpected'; Save-Manifest; Expect-Rejected
$manifest.linkFlags=''; $appHash=$binaries['touch_probe.ino.bin']; $binaries.Remove('touch_probe.ino.bin'); Save-Manifest; Expect-Rejected
$binaries['touch_probe.ino.bin']=$appHash; Save-Manifest
$appPath=Join-Path $binRoot 'touch_probe.ino.bin'
[IO.File]::WriteAllBytes($appPath,[byte[]]@(7)); Expect-Rejected
[IO.File]::WriteAllBytes($appPath,[byte[]]@(1,2,3))
[IO.File]::WriteAllBytes($bootApp0,[byte[]]@(8)); Expect-Rejected
[IO.File]::WriteAllBytes($bootApp0,[byte[]]@(9))
[IO.File]::WriteAllBytes((Join-Path $binRoot 'extra.bin'),[byte[]]@(0)); Expect-Rejected
Write-Output "Upload manifest: $checks checks passed; no device accessed; fixture=$fixtureRoot"

# Each target must bind its own sketch name; uploads use a disposable COPY, never the build folder.
foreach ($testTarget in @('touch','hid','dual')) {
    $targetRoot = Join-Path $env:TEMP ('thumbremote-target-test-' + [Guid]::NewGuid().ToString('N'))
    $targetBinRoot = Join-Path $targetRoot 'build'
    [IO.Directory]::CreateDirectory($targetBinRoot) | Out-Null
    $targetSketch = Get-SketchName $testTarget
    $targetBins = @{}
    foreach ($binaryName in @("$targetSketch.ino.bin", "$targetSketch.ino.bootloader.bin", "$targetSketch.ino.partitions.bin")) {
        $targetBinPath=Join-Path $targetBinRoot $binaryName
        [IO.File]::WriteAllBytes($targetBinPath,[byte[]]@(1,2,3))
        $targetBins[$binaryName]=(Get-FileHash -LiteralPath $targetBinPath).Hash
    }
    $targetManifest = @{schema=2;target=$testTarget;sourceHash=$sourceHash;fqbn=$fqbn;
        linkFlags=$(if($testTarget -in @('hid','dual')){'-Wl,--wrap=ble_gap_adv_start'}else{''});
        binaries=$targetBins;bootApp0Hash=(Get-FileHash -LiteralPath $bootApp0).Hash}
    $targetManifestPath=Join-Path $targetRoot 'manifest.json'
    [IO.File]::WriteAllText($targetManifestPath,($targetManifest|ConvertTo-Json -Depth 4))
    Assert-Verified $testTarget $sourceHash $targetRoot; $checks++
    Assert-BuildBinaries $testTarget $targetBinRoot; $checks++
    $staged=New-UploadInput $testTarget $sourceHash $targetRoot
    if($staged -eq $targetBinRoot -or @(Get-ChildItem -LiteralPath $staged -Filter '*.bin' -File).Count -ne 3){throw 'Invalid upload staging'}
    $checks++
    [IO.File]::WriteAllBytes((Join-Path $staged ($targetSketch+'.ino_flashed.bin')),[byte[]]@(1))
    Assert-Verified $testTarget $sourceHash $targetRoot;$checks++
    $secondStage=New-UploadInput $testTarget $sourceHash $targetRoot
    if($secondStage -eq $staged){throw 'Upload staging was reused'};$checks++
    $wrongTarget=if($testTarget -eq 'touch'){'dual'}else{'touch'}
    $rejected=$false;try{Assert-Verified $wrongTarget $sourceHash $targetRoot}catch{$rejected=$true}
    if(!$rejected){throw 'Cross-target manifest accepted'};$checks++
    if($testTarget -eq 'dual'){
        $targetManifest.linkFlags='';[IO.File]::WriteAllText($targetManifestPath,($targetManifest|ConvertTo-Json -Depth 4))
        $rejected=$false;try{Assert-Verified $testTarget $sourceHash $targetRoot}catch{$rejected=$true}
        if(!$rejected){throw 'Dual without bond protection accepted'};$checks++
        $targetManifest.linkFlags='-Wl,--wrap=ble_gap_adv_start';[IO.File]::WriteAllText($targetManifestPath,($targetManifest|ConvertTo-Json -Depth 4))
    }
    [IO.File]::WriteAllBytes((Join-Path $targetBinRoot ($targetSketch+'.ino_flashed.bin')),[byte[]]@(1))
    $rejected=$false;try{Assert-BuildBinaries $testTarget $targetBinRoot}catch{$rejected=$true}
    if(!$rejected){throw 'Rerun verify could bless unexpected binary'};$checks++
    $rejected=$false;try{New-UploadInput $testTarget $sourceHash $targetRoot|Out-Null}catch{$rejected=$true}
    if(!$rejected){throw 'Unexpected build binary bypassed by staging'};$checks++
    $extraName=$targetSketch+'.ino_flashed.bin'
    $targetManifest.binaries[$extraName]=(Get-FileHash -LiteralPath (Join-Path $targetBinRoot $extraName)).Hash
    [IO.File]::WriteAllText($targetManifestPath,($targetManifest|ConvertTo-Json -Depth 4))
    $rejected=$false;try{Assert-Verified $testTarget $sourceHash $targetRoot}catch{$rejected=$true}
    if(!$rejected){throw 'Unknown binary was laundered into manifest'};$checks++
}
Write-Output "Target/staging manifest: $checks total checks passed; no device accessed"
