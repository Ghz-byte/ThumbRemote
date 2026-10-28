param(
    [ValidateSet('test', 'verify', 'build', 'install')]
    [string]$Action = 'verify',
    [switch]$Details
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$androidRoot = Join-Path $projectRoot 'android'
$jdkPath = 'C:\Program Files\Android\Android Studio\jbr'
$sdkPath = Join-Path $env:USERPROFILE 'AppData\Local\Android\Sdk'
# AGP rejects non-ASCII project directories on Windows. Build an immutable
# source snapshot in an ASCII temp directory, without bypassing that check.
$sourceFiles = @(Get-ChildItem -LiteralPath $androidRoot -Recurse -File | Where-Object {
    $_.FullName.Substring($androidRoot.Length) -notmatch '\\(build|\.gradle|\.kotlin|\.idea)\\' -and $_.Name -ne 'local.properties'
})
$fingerprint = ($sourceFiles | Sort-Object FullName | ForEach-Object {
    $_.FullName.Substring($androidRoot.Length) + ':' + (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
}) -join "`n"
$hasher = [Security.Cryptography.SHA256]::Create()
try { $sourceHash = ([BitConverter]::ToString($hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($fingerprint)))).Replace('-', '').Substring(0, 12).ToLowerInvariant() }
finally { $hasher.Dispose() }
$buildRoot = Join-Path ([IO.Path]::GetTempPath()) "thumbremote-$sourceHash"
$pathHasher = [Security.Cryptography.SHA256]::Create()
try { $pathHash = ([BitConverter]::ToString($pathHasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($projectRoot)))).Replace('-', '').Substring(0, 12).ToLowerInvariant() }
finally { $pathHasher.Dispose() }
$cacheRoot = Join-Path ([IO.Path]::GetTempPath()) "thumbremote-cache-$pathHash\gradle-home"
if (!(Test-Path -LiteralPath $cacheRoot)) {
    [IO.Directory]::CreateDirectory($cacheRoot) | Out-Null
    foreach ($cachePart in @('caches', 'wrapper')) {
        $existing = Join-Path $projectRoot ".tools\gradle-home\$cachePart"
        if (Test-Path -LiteralPath $existing) { Copy-Item -LiteralPath $existing -Destination $cacheRoot -Recurse -Force }
    }
}
foreach ($source in $sourceFiles) {
    $relative = $source.FullName.Substring($androidRoot.Length).TrimStart('\')
    $target = Join-Path $buildRoot $relative
    [IO.Directory]::CreateDirectory((Split-Path -Parent $target)) | Out-Null
    Copy-Item -LiteralPath $source.FullName -Destination $target -Force
}
Write-Output "ASCII build snapshot: $buildRoot"
if (!(Test-Path -LiteralPath (Join-Path $jdkPath 'bin\javac.exe'))) {
    throw 'Android Studio JDK not found. Configure the JDK path in scripts/android.ps1.'
}
if (!(Test-Path -LiteralPath (Join-Path $sdkPath 'platform-tools\adb.exe'))) {
    throw 'Android SDK not found. Configure the SDK path in scripts/android.ps1.'
}
$oldJava = $env:JAVA_HOME
$oldSdk = $env:ANDROID_HOME
$oldCache = $env:GRADLE_USER_HOME
try {
    $env:JAVA_HOME = $jdkPath
    $env:ANDROID_HOME = $sdkPath
    $env:GRADLE_USER_HOME = $cacheRoot
    # Reuse the existing Windows proxy for this Java process only.
    $proxyArgs = @('-Dorg.gradle.internal.http.connectionTimeout=15000', '-Dorg.gradle.internal.http.socketTimeout=30000')
    $proxySettings = Get-ItemProperty -LiteralPath 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings' -ErrorAction SilentlyContinue
    if ($proxySettings.ProxyEnable -eq 1 -and $proxySettings.ProxyServer) {
        $proxyText = ($proxySettings.ProxyServer -split ';' | Where-Object { $_ -like 'https=*' } | Select-Object -First 1) -replace '^https=', ''
        if (!$proxyText -and $proxySettings.ProxyServer -notmatch '=') { $proxyText = $proxySettings.ProxyServer }
        if ($proxyText) {
            if ($proxyText -notmatch '^https?://') { $proxyText = "http://$proxyText" }
            $proxyUri = [Uri]$proxyText
            $proxyArgs += @("-Dhttps.proxyHost=$($proxyUri.Host)", "-Dhttps.proxyPort=$($proxyUri.Port)", "-Dhttp.proxyHost=$($proxyUri.Host)", "-Dhttp.proxyPort=$($proxyUri.Port)")
        }
    }
    $tasks = @(switch ($Action) {
        'test' { @(':app:testDebugUnitTest') }
        'verify' { @(':app:testDebugUnitTest', ':app:lintDebug', ':app:assembleDebug') }
        default { @(':app:assembleDebug') }
    })
    if ($Details) { $tasks += '--info' }
    & (Join-Path $buildRoot 'gradlew.bat') @proxyArgs -p $buildRoot --console=plain --no-daemon @tasks
    if ($LASTEXITCODE -ne 0) { throw "Gradle failed with exit code $LASTEXITCODE" }
    foreach ($output in @('outputs', 'reports', 'test-results')) {
        $from = Join-Path $buildRoot "app\build\$output"
        if (Test-Path -LiteralPath $from) {
            $to = Join-Path $androidRoot 'app\build'
            [IO.Directory]::CreateDirectory($to) | Out-Null
            Copy-Item -LiteralPath $from -Destination $to -Recurse -Force
        }
    }
    if ($Action -eq 'install') {
        $adbPath = Join-Path $sdkPath 'platform-tools\adb.exe'
        $lines = & $adbPath devices
        if ($LASTEXITCODE -ne 0) { throw 'ADB devices failed.' }
        $devices = @($lines | Where-Object { $_ -match '^\S+\s+device$' })
        if ($devices.Count -ne 1) { throw 'Connect exactly one authorized Android device before installing.' }
        $serial = ($devices[0] -split '\s+')[0]
        & $adbPath -s $serial install -r (Join-Path $androidRoot 'app\build\outputs\apk\debug\app-debug.apk')
        if ($LASTEXITCODE -ne 0) { throw 'APK installation failed.' }
    }
} finally {
    $env:JAVA_HOME = $oldJava
    $env:ANDROID_HOME = $oldSdk
    $env:GRADLE_USER_HOME = $oldCache
}
