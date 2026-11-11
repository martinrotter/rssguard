# Windows PowerShell 5.1 compatible. No GitHub runner, Python or SDK required.
param([string]$Package = $PSScriptRoot, [switch]$AllowModernWindows)
$ErrorActionPreference = 'Stop'
$Package = (Resolve-Path $Package).Path
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class RssGuardWindowsVersion {
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct VersionInfo {
        public uint Size, Major, Minor, Build, Platform;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)] public string ServicePack;
    }
    [DllImport("ntdll.dll", CharSet = CharSet.Unicode)]
    public static extern int RtlGetVersion(ref VersionInfo info);
    public static string Read() {
        var v = new VersionInfo();
        v.Size = (uint)Marshal.SizeOf(v);
        if (RtlGetVersion(ref v) != 0) throw new Exception("Cannot read OS version");
        return v.Major + "." + v.Minor + "." + v.Build;
    }
}
'@
$os = [RssGuardWindowsVersion]::Read()
if (!$AllowModernWindows -and $os -notlike '6.3.*') { throw "Run on Windows 8.1 x64 (observed $os)" }
if (![Environment]::Is64BitOperatingSystem) { throw 'Windows x64 is required' }
$manifest = Get-Content "$Package\build-manifest.json" -Raw | ConvertFrom-Json
if (Get-ChildItem $Package -Filter 'Qt5*.dll' -Recurse) { throw 'Qt 5 DLLs must not be mixed into this Qt 6 package' }
foreach ($dll in Get-ChildItem $Package -Filter 'Qt6*.dll' -Recurse) {
    if ($dll.VersionInfo.FileVersion -notmatch '^6\.8\.3(?:\.|$)') { throw "Unexpected Qt version: $($dll.FullName)" }
}
foreach ($file in 'rssguard.exe', 'rssguard.dll', 'plugins\rssguard-xmpp.dll', 'QXmppQt6.dll',
    'platforms\qwindows.dll', 'sqldrivers\qsqlite.dll', 'tls\qopensslbackend.dll',
    'libssl-3-x64.dll', 'libcrypto-3-x64.dll', 'test_xmpp_pubsub.exe', 'rssguard-package-smoke.exe',
    'rssguard-article-extractor.exe') {
    if (!(Test-Path (Join-Path $Package $file))) { throw "Missing packaged dependency: $file" }
}
if ($manifest.viewer -eq 'web') {
    foreach ($file in 'QtWebEngineProcess.exe', 'Qt6WebEngineCore.dll', 'Qt6WebEngineWidgets.dll',
        'resources\icudtl.dat', 'resources\qtwebengine_resources.pak', 'translations\qtwebengine_locales\en-US.pak') {
        if (!(Test-Path (Join-Path $Package $file))) { throw "Missing WebEngine dependency: $file" }
    }
}
$temporary = Join-Path ([IO.Path]::GetTempPath()) ('rssguard81-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $temporary | Out-Null
function Probe([string]$Exe, [string]$Arguments, [string]$Marker = '') {
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = Join-Path $Package $Exe
    $info.Arguments = $Arguments
    $info.WorkingDirectory = $Package
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    foreach ($key in @($info.EnvironmentVariables.Keys)) {
        if ($key -match '^(QT|QML|QSG|QTDIR|CMAKE_PREFIX_PATH)') { $info.EnvironmentVariables.Remove($key) }
    }
    $info.EnvironmentVariables['PATH'] = "$Package;$env:SystemRoot\System32;$env:SystemRoot"
    $info.EnvironmentVariables['QT_PLUGIN_PATH'] = $Package
    $info.EnvironmentVariables['QT_QPA_PLATFORM'] = 'windows'
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $info
    [void]$process.Start()
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    if (!$process.WaitForExit(60000)) { $process.Kill(); throw "$Exe timed out" }
    $text = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
    $text | Add-Content (Join-Path $temporary 'validation.log')
    Write-Host $text
    if ($process.ExitCode -ne 0 -or ($Marker -and !$text.Contains($Marker))) {
        throw "$Exe failed (exit $($process.ExitCode)); logs: $temporary"
    }
}
Probe 'test_xmpp_pubsub.exe' '--plugin plugins/rssguard-xmpp.dll' '0 failed'
Probe 'rssguard-package-smoke.exe' ('--data "' + (Join-Path $temporary 'profile') + '" --debug') ('RSSGUARD_PACKAGE_SMOKE_OK:' + $manifest.viewer)
Probe 'rssguard-article-extractor.exe' '--help' 'Usage of'
$result = [ordered]@{ os = $os; targetOsTested = ($os -like '6.3.*'); viewer = $manifest.viewer;
    rssguard = $manifest.rssguard; finishedUtc = [DateTime]::UtcNow.ToString('o'); checks = 'plugin, STARTTLS, SASL, discovery, PubSub fetch/push, certificate rejection, article rendering, extractor startup' }
$result | ConvertTo-Json | Set-Content (Join-Path $temporary 'result.json')
Write-Host "Validation passed. Keep the report and logs in $temporary"
