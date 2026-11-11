# Run in an x64 VS 2022 developer shell, toolset 14.38, on Windows 10+.
# Build hosts may be modern; every shipped component targets Windows 8.1.
param([string]$Work = 'C:\rssguard81', [ValidateSet('text', 'web')][string]$Viewer = 'text', [int]$Jobs = 4)
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true
$repo = (Resolve-Path "$PSScriptRoot\..\..\..").Path
New-Item -ItemType Directory -Force $Work | Out-Null
$Work = (Resolve-Path $Work).Path

function Run([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE)" }
}
function Source([string]$Name, [string]$Url, [string]$Commit) {
    $path = Join-Path $Work $Name
    if (!(Test-Path "$path\.git")) {
        Run git @('clone', '--no-checkout', $Url, $path)
        Run git @('-C', $path, 'checkout', '--detach', $Commit)
    }
    $actual = (& git -C $path rev-parse HEAD).Trim()
    if ($actual -ne $Commit) { throw "Unexpected revision in $path ($actual)" }
    return $path
}

if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64' -or $env:VCToolsVersion -notlike '14.38.*') {
    throw 'Use the VS 2022 14.38 x64 toolset, matching the deployed runtime.'
}
$qtSource = Source qt-source https://github.com/qt/qt5.git bab1fecd556ea561c4a89686293116741acfa1b4
$compat = Source qt-compat https://github.com/crystalidea/qt6windows7.git 7cd5e9532e1af0e62a3bfe412d03fb725c6378ed
$openssl = Source openssl-source https://github.com/openssl/openssl.git c10e643222ce5a185f2980d7352d545afacb0fed
$goPatches = Source go-compat https://github.com/XTLS/go-win7.git dda9e23edb010401ab045e3d2fbce9e09e0e1f87
$web = if ($Viewer -eq 'web') { 'ON' } else { 'OFF' }
$qt = "$Work\qt-$Viewer"
$ssl = "$Work\openssl"

# No binary overlays: patch exactly the matching Qt source module, then build
# one coherent SDK. Plugins and QXmpp are subsequently compiled against it.
if (!(Test-Path "$qt\bin\qmake.exe")) {
    Run git @('-C', $qtSource, 'submodule', 'update', '--init', '--depth', '1',
        'qtbase', 'qtdeclarative', 'qtshadertools', 'qtmultimedia', 'qttools', 'qtsvg', 'qtimageformats', 'qt5compat')
    Copy-Item "$compat\qtbase\*" "$qtSource\qtbase" -Recurse -Force
    $modules = 'qtbase,qtdeclarative,qtshadertools,qtmultimedia,qttools,qtsvg,qtimageformats,qt5compat'
    if ($Viewer -eq 'web') {
        Run git @('-C', $qtSource, 'submodule', 'update', '--init', '--recursive', '--depth', '1',
            'qtwebengine', 'qtwebchannel', 'qtpositioning')
        # A reviewed source port to Qt 6.8.3, derived from crystalidea's newer
        # Chromium compatibility work. Fail on mismatched source, never overlay DLLs.
        Run git @('-C', "$qtSource\qtwebengine", 'apply', '--check', "$PSScriptRoot\qtwebengine-6.8.3.patch")
        Run git @('-C', "$qtSource\qtwebengine", 'apply', "$PSScriptRoot\qtwebengine-6.8.3.patch")
        $modules += ',qtwebengine,qtwebchannel,qtpositioning'
        # Chromium also requires gperf, bison and flex on the build host.
        # Keep these out of the distributed application's dependency set.
        $hostTools = "$Work\host-tools"
        $buildTools = Source qt-build-tools https://github.com/crystalidea/qt-build-tools.git 306c3c5be2c6ad27d75a98eba614be373d8c9d14
        $flex = Source winflexbison https://github.com/lexxmark/winflexbison.git 1176da2adaf972b2216b1ac8dd611e128fe80bc5
        Run cmake @('-S', $flex, '-B', "$Work\flex-build", '-G', 'Ninja',
            '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_POLICY_VERSION_MINIMUM=3.5')
        Run cmake @('--build', "$Work\flex-build", '--parallel', "$Jobs")
        Run cmake @('--install', "$Work\flex-build", '--prefix', $hostTools)
        Copy-Item "$buildTools\_tools\gperf.exe" $hostTools
        $env:PATH = "$hostTools;$env:PATH"
        Run python @('-m', 'pip', 'install', 'html5lib==1.1')
    }
    Push-Location $openssl
    try {
        Run perl @('Configure', 'VC-WIN64A', 'shared', 'no-asm', 'no-tests', "--prefix=$ssl", "--openssldir=$ssl\ssl")
        Run nmake @()
        Run nmake @('install_sw')
    } finally { Pop-Location }
    $env:PATH = "$ssl\bin;$env:PATH"
    New-Item -ItemType Directory -Force "$Work\qt-build" | Out-Null
    Push-Location "$Work\qt-build"
    try {
        Run "$qtSource\configure.bat" @('-prefix', $qt, '-release', '-opensource', '-confirm-license',
            '-nomake', 'tests', '-nomake', 'examples', '-submodules',
            $modules,
            '-opengl', 'desktop', '-openssl-linked', '--', "-DOPENSSL_ROOT_DIR=$ssl",
            '-DQT_FEATURE_sql_mysql=OFF', '-DQT_FEATURE_sql_odbc=OFF', '-DQT_FEATURE_sql_psql=OFF',
            '-DQT_FEATURE_ffmpeg=OFF', '-DQT_FEATURE_wmf=OFF')
        Run cmake @('--build', '.', '--parallel', "$Jobs")
        Run cmake @('--install', '.')
    } finally { Pop-Location }
}
$env:PATH = "$qt\bin;$ssl\bin;$env:PATH"
$env:CMAKE_PREFIX_PATH = "$qt"
$env:QT_PLUGIN_PATH = "$qt\plugins"

# Official Go 1.21+ executables require newer Windows. Rebuild the pinned SDK
# with the legacy runtime patch, and prohibit automatic toolchain substitution.
$bootstrap = (& go env GOROOT).Trim()
$go = "$Work\go"
if (!(Test-Path "$go\bin\go.exe")) {
    $archive = "$Work\go1.26.11.src.tar.gz"
    Invoke-WebRequest https://go.dev/dl/go1.26.11.src.tar.gz -OutFile $archive
    $release = Invoke-RestMethod 'https://go.dev/dl/?mode=json&include=all'
    $record = $release | Where-Object version -eq 'go1.26.11' | Select-Object -First 1
    $expected = ($record.files | Where-Object filename -eq 'go1.26.11.src.tar.gz').sha256
    if (!$expected -or (Get-FileHash $archive -Algorithm SHA256).Hash.ToLower() -ne $expected) {
        throw 'Go source archive checksum mismatch'
    }
    Run tar @('-xzf', $archive, '-C', $Work)
    Run git @('-C', $go, 'apply', '--check', "$goPatches\go-1-26.diff")
    Run git @('-C', $go, 'apply', "$goPatches\go-1-26.diff")
    $env:GOROOT_BOOTSTRAP = $bootstrap
    $env:GOTOOLCHAIN = 'local'
    Push-Location "$go\src"
    try { Run .\make.bat @() } finally { Pop-Location }
}
$env:GOROOT = $go
$env:GOTOOLCHAIN = 'local'
$env:CGO_ENABLED = '0'
$env:PATH = "$go\bin;$env:PATH"

Run git @('-C', $repo, 'submodule', 'update', '--init',
    'src/librssguard/3rd-party/gumbo', 'src/librssguard/3rd-party/qtlinq',
    'src/librssguard/3rd-party/qt-publicsuffix', 'src/librssguard-xmpp/src/3rd-party/qxmpp')
$qxmpp = "$Work\qxmpp"
Run cmake @('-S', "$repo\src\librssguard-xmpp\src\3rd-party\qxmpp", '-B', "$Work\qxmpp-build", '-G', 'Ninja',
    '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_INSTALL_PREFIX=$qxmpp", '-DBUILD_TESTING=OFF', '-DBUILD_EXAMPLES=OFF',
    '-DBUILD_OMEMO=OFF', '-DWITH_ENCRYPTION=OFF', '-DWITH_GSTREAMER=OFF')
Run cmake @('--build', "$Work\qxmpp-build", '--parallel', "$Jobs")
Run cmake @('--install', "$Work\qxmpp-build")
$build = "$Work\app-build"
Run cmake @('-S', $repo, '-B', $build, '-G', 'Ninja', '-C', "$PSScriptRoot\profile.cmake",
    '-DCMAKE_BUILD_TYPE=Release', "-Dqxmpp_ROOT=$qxmpp", "-DGO_EXECUTABLE=$go\bin\go.exe", "-DWEB_ARTICLE_VIEWER_WEBENGINE=$web")
Run cmake @('--build', $build, '--parallel', "$Jobs")
$env:PATH = "$qxmpp\bin;$build\src\librssguard;$build\src\librssguard-standard;$env:PATH"
Run ctest @('--test-dir', $build, '--output-on-failure', '-R', '^(test_xmpp_pubsub|test_shortcutpriority|test_shortcutsettings)$')
Push-Location "$repo\resources\scripts\standalone\article-extractor"
try { Run go @('test', '-count=1', './...') } finally { Pop-Location }

# Refuse to overlay an existing package or copy unrelated DLLs.
$stage = "$Work\rssguard-win81-x64-$Viewer"
if (Test-Path $stage) { throw "Remove the previous generated package $stage before packaging again" }
Run cmake @('--install', $build, '--prefix', $stage)
Copy-Item "$qxmpp\bin\QXmppQt6.dll" $stage
Copy-Item "$ssl\bin\libssl-3-x64.dll", "$ssl\bin\libcrypto-3-x64.dll" $stage
Copy-Item "$build\tests\test_xmpp_pubsub.exe", "$build\src\rssguard\rssguard-package-smoke.exe" $stage
Run "$qt\bin\windeployqt.exe" @('--release', '--no-compiler-runtime', '--no-translations', '--force-openssl', '--dir', $stage,
    "$stage\rssguard.exe", "$stage\rssguard.dll", "$stage\plugins\rssguard-xmpp.dll",
    "$stage\test_xmpp_pubsub.exe", "$stage\rssguard-package-smoke.exe")
@'
[Paths]
Prefix=.
Plugins=.
Libraries=.
Data=.
Translations=translations
'@ | Set-Content "$stage\qt.conf"
# Ship Microsoft's pinned installer rather than copying a newer host CRT.
# Windows 8.1 users install this prerequisite once; no manual DLL replacement.
New-Item -ItemType Directory "$stage\prerequisites" | Out-Null
$redist = "$stage\prerequisites\VC_redist.x64.exe"
Invoke-WebRequest 'https://download.visualstudio.microsoft.com/download/pr/6ba404bb-6312-403e-83be-04b062914c98/1AD7988C17663CC742B01BEF1A6DF2ED1741173009579AD50A94434E54F56073/VC_redist.x64.exe' -OutFile $redist
if ((Get-FileHash $redist -Algorithm SHA256).Hash -ne '1AD7988C17663CC742B01BEF1A6DF2ED1741173009579AD50A94434E54F56073') {
    throw 'Microsoft VC runtime installer checksum mismatch'
}
Copy-Item "$PSScriptRoot\validate.ps1" "$stage\validate-windows81.ps1"
Copy-Item "$PSScriptRoot\README.md" "$stage\WINDOWS81.md"
New-Item -ItemType Directory "$stage\licenses" | Out-Null
Copy-Item "$openssl\LICENSE.txt" "$stage\licenses\OpenSSL.txt"
Copy-Item "$qtSource\qtbase\LICENSES" "$stage\licenses\Qt" -Recurse
if ($Viewer -eq 'web') {
    Copy-Item "$qtSource\qtwebengine\LICENSES" "$stage\licenses\QtWebEngine" -Recurse
}
Copy-Item "$repo\LICENSE.md" "$stage\licenses\RSS-Guard.txt"
Copy-Item "$PSScriptRoot\qtwebengine-6.8.3.patch" "$stage\licenses\qtwebengine-6.8.3.patch"
Copy-Item "$PSScriptRoot\PATCHES.md" "$stage\licenses\SOURCE-PROVENANCE.md"
Copy-Item "$repo\src\librssguard-xmpp\src\3rd-party\qxmpp\LICENSES" "$stage\licenses\QXmpp" -Recurse
Copy-Item "$go\LICENSE" "$stage\licenses\Go.txt"
Copy-Item "$goPatches\LICENSE" "$stage\licenses\Go-compatibility.txt"
@{
    rssguard = (& git -C $repo rev-parse HEAD).Trim()
    qt = '6.8.3'; qtSource = 'bab1fecd556ea561c4a89686293116741acfa1b4'
    qtCompatibility = '7cd5e9532e1af0e62a3bfe412d03fb725c6378ed'
    webCompatibility = 'fb355ed9ecc7c1667084db2dd62508b3801cd85f (source port to 6.8.3)'
    qxmpp = (& git -C "$repo\src\librssguard-xmpp\src\3rd-party\qxmpp" rev-parse HEAD).Trim()
    openssl = 'c10e643222ce5a185f2980d7352d545afacb0fed'
    go = '1.26.11'; goCompatibility = 'dda9e23edb010401ab045e3d2fbce9e09e0e1f87'
    compiler = $env:VCToolsVersion; viewer = $Viewer; target = 'Windows 8.1 x64'
    windows81Validated = $false
} | ConvertTo-Json | Set-Content "$stage\build-manifest.json"
# Smoke test with SDK paths removed: only the packaged dependencies can load.
& "$PSScriptRoot\validate.ps1" -Package $stage -AllowModernWindows
if (!$?) { throw 'Package validation failed' }
Compress-Archive -Path "$stage\*" -DestinationPath "$Work\rssguard-win81-x64-$Viewer-candidate.zip"
