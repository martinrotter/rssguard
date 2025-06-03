$os = $args[0]
$use_qt5 = $args[1]

echo "We are building for MS Windows."
echo "OS: $os; Qt5: $use_qt5"

$git_revlist = git rev-list --tags --max-count=1
$git_tag = git describe --tags $git_revlist
$git_revision = git rev-parse --short HEAD
$old_pwd = $pwd.Path

# Functions.
function Fetch-Latest-Release([string]$OrgRepo, [string]$NameRegex) {
  $releases_url = "https://api.github.com/repos/" + $OrgRepo +"/releases"
  $releases_req = Invoke-WebRequest -Uri "$releases_url" -Headers @{ "Authorization" = "Bearer $env:GITHUB_TOKEN" }
  $releases_json = $releases_req.Content | ConvertFrom-Json
  $releases_release = $releases_json[0]
  $asset = $releases_release.assets | Where-Object {$_.name -match $NameRegex} | Select-Object

  Add-Member -InputObject $asset -NotePropertyName "tag_name" -NotePropertyValue $releases_release.tag_name.Substring(1)

  Write-Host $asset

  return $asset
}

# Prepare environment.
Install-Module Pscx -Scope CurrentUser -AllowClobber -Force
Install-Module VSSetup -Scope CurrentUser -AllowClobber -Force
Import-VisualStudioVars -Architecture x64

$AllProtocols = [System.Net.SecurityProtocolType]'Tls11,Tls12'
[System.Net.ServicePointManager]::SecurityProtocol = $AllProtocols
$ProgressPreference = 'SilentlyContinue'

# Get and prepare needed dependencies.
if ($use_qt5 -eq "ON") {
  $qt_version = "5.15.2"
  $qt_arch_base = "msvc2019_64"

  $use_libmpv = "OFF"
  $use_qtmultimedia = "ON"
}
else {
  $qt_version = "6.8.3"
  $qt_arch_base = "msvc2022_64"

  $use_libmpv = "ON"
  $use_qtmultimedia = "OFF"
}

$is_qt_6 = $qt_version.StartsWith("6")
$qt_arch = "win64_" + $qt_arch_base

$maria_version = "11.4.5"
$maria_link = "https://archive.mariadb.org/mariadb-$maria_version/winx64-packages/mariadb-$maria_version-winx64.zip"
$maria_output = "maria.zip"

$cmake_asset = Fetch-Latest-Release -OrgRepo "Kitware/CMake" -NameRegex "cmake-.+-windows-x86_64\.zip"
$cmake_version = $cmake_asset.tag_name
$cmake_link = $cmake_asset.browser_download_url
$cmake_output = "cmake.zip"

$zlib_asset = Fetch-Latest-Release -OrgRepo "madler/zlib" -NameRegex "zlib.+\.zip$"
$zlib_version = $zlib_asset.tag_name
$zlib_link = $zlib_asset.browser_download_url
$zlib_output = "zlib.zip"

$libmpv_link = Fetch-Latest-Release -OrgRepo "zhongfly/mpv-winbuild" -NameRegex "mpv-dev-x86_64-2.+7z" | Select-Object -ExpandProperty browser_download_url
$libmpv_output = "mpv.zip"

$ytdlp_link = Fetch-Latest-Release -OrgRepo "yt-dlp/yt-dlp" -NameRegex "yt-dlp.exe" | Select-Object -ExpandProperty browser_download_url
$ytdlp_output = "yt-dlp.exe"

Invoke-WebRequest -Uri "$maria_link" -OutFile "$maria_output"
& ".\resources\scripts\7za\7za.exe" x "$maria_output"

Invoke-WebRequest -Uri "$cmake_link" -OutFile "$cmake_output"
& ".\resources\scripts\7za\7za.exe" x "$cmake_output"

Invoke-WebRequest -Uri "$zlib_link" -OutFile "$zlib_output"
& ".\resources\scripts\7za\7za.exe" x "$zlib_output"

Invoke-WebRequest -Uri "$libmpv_link" -OutFile "$libmpv_output"
& ".\resources\scripts\7za\7za.exe" x "$libmpv_output" -ompv

Invoke-WebRequest -Uri "$ytdlp_link" -OutFile "$ytdlp_output"

$cmake_path = "$old_pwd\cmake-$cmake_version-windows-x86_64\bin\cmake.exe"
$zlib_path = "$old_pwd\zlib-$zlib_version"
$libmpv_path = "$old_pwd\mpv"
$ytdlp_path = "$old_pwd\$ytdlp_output"

# Download Qt itself.
$qt_path = "$old_pwd\qt"

# Install "aqtinstall" from its master branch to have latest code.
pip3 install -U pip
pip3 install -I git+https://github.com/miurahr/aqtinstall

if ($is_qt_6) {
  aqt install-qt -O "$qt_path" windows desktop $qt_version $qt_arch -m qtwebengine qtimageformats qtmultimedia qt5compat qtwebchannel qtpositioning
}
else {
  aqt install-qt -O "$qt_path" windows desktop $qt_version $qt_arch -m qtwebengine
}

aqt install-src -O "$qt_path" windows desktop $qt_version --archives qtbase

$qt_qmake = "$qt_path\$qt_version\$qt_arch_base\bin\qmake.exe"
$env:PATH = "$qt_path\$qt_version\$qt_arch_base\bin\;" + $env:PATH

if ($is_qt_6) {
  # Download openssl 3.x.
  aqt install-tool -O "$qt_path" windows desktop tools_opensslv3_x64 qt.tools.opensslv3.win_x64
  $openssl_base_path = "$qt_path\Tools\OpenSSLv3\Win_x64"
}
else {
  # Download openssl 1.x from external source.
  $openssl_link = "https://download.firedaemon.com/FireDaemon-OpenSSL/openssl-1.1.1w.zip";
  $openssl_output = "openssl.zip"
  Invoke-WebRequest -Uri "$openssl_link" -OutFile "$openssl_output"
  & ".\resources\scripts\7za\7za.exe" x $openssl_output
  $openssl_base_path = "$pwd\openssl-1.1\x64"
}

# Build dependencies.

# MariaDB.
$maria_path = "$old_pwd\mariadb-$maria_version-winx64"
$qt_sqldrivers_path = "$qt_path\$qt_version\Src\qtbase\src\plugins\sqldrivers"

cd "$qt_sqldrivers_path"

if ($is_qt_6) {
  & $cmake_path -G Ninja -DCMAKE_BUILD_TYPE="Release" -DMySQL_INCLUDE_DIR="$maria_path\include\mysql" -DMySQL_LIBRARY="$maria_path\lib\libmariadb.lib"
  & $cmake_path --build .

  $with_qt6 = "ON"
}
else {
  & $qt_qmake -- MYSQL_INCDIR="$maria_path\include\mysql" MYSQL_LIBDIR="$maria_path\lib"
  nmake.exe sub-mysql

  $with_qt6 = "OFF"
}

# zlib
cd "$zlib_path"
nmake.exe -f "win32\Makefile.msc"

cd "$old_pwd"

# Build application.
mkdir "rssguard-build"
cd "rssguard-build"

& "$cmake_path" ".." -G Ninja -DCMAKE_BUILD_TYPE="RelWithDebInfo" -DCMAKE_VERBOSE_MAKEFILE="ON" -DBUILD_WITH_QT6="$with_qt6" -DREVISION_FROM_GIT="ON" -DUSE_SYSTEM_SQLITE="OFF" -DZLIB_ROOT="$zlib_path" -DENABLE_COMPRESSED_SITEMAP="ON" -DENABLE_MEDIAPLAYER_LIBMPV="$use_libmpv" -DENABLE_MEDIAPLAYER_QTMULTIMEDIA="$use_qtmultimedia" -DLibMPV_ROOT="$libmpv_path" -DFEEDLY_CLIENT_ID="$env:FEEDLY_CLIENT_ID" -DFEEDLY_CLIENT_SECRET="$env:FEEDLY_CLIENT_SECRET"
& "$cmake_path" --build .
& "$cmake_path" --install . --prefix app

cd "app"
windeployqt.exe --verbose 1 --no-compiler-runtime --no-translations --release "rssguard.exe" "rssguard.dll" "." ".\\plugins"
cd ".."

# Copy OpenSSL.
if ($is_qt_6) {
  Copy-Item -Path "$qt_path\$qt_version\$qt_arch_base\plugins\tls\qopensslbackend.dll" -Destination ".\app\tls\"
}

Copy-Item -Path "$openssl_base_path\bin\libcrypto*.dll" -Destination ".\app\"
Copy-Item -Path "$openssl_base_path\bin\libssl*.dll" -Destination ".\app\"

# Copy MySQL.
Copy-Item -Path "$maria_path\lib\libmariadb.dll" -Destination ".\app\"
Copy-Item -Path "$qt_sqldrivers_path\plugins\sqldrivers\qsqlmysql.dll" -Destination ".\app\sqldrivers\" -Force

# Copy zlib.
Copy-Item -Path "$zlib_path\zlib1.dll" -Destination ".\app\"

if ($git_tag -match "devbuild") {
  # Copy debug symbols.
  Copy-Item -Path ".\src\librssguard\rssguard.pdb" -Destination ".\app\"
}

if ($use_libmpv -eq "ON") {
  # Copy libmpv and yt-dlp.
  Copy-Item -Path "$libmpv_path\libmpv*.dll" -Destination ".\app\"
  Copy-Item -Path "$ytdlp_path" -Destination ".\app\"
}

$packagebase = "rssguard-${git_tag}-${git_revision}-win"

if ($use_qt5 -eq "ON") {
  $packagebase += "7"
}
else {
  $packagebase += "10"
}

# Create 7zip package.
& "$old_pwd\resources\scripts\7za\7za.exe" a -t7z -mmt -mx9 "$packagebase.7z" ".\app\*"

# Create NSIS installation package.
& "$old_pwd\resources\scripts\nsis\makensis.exe" "/XOutFile $packagebase.exe" ".\NSIS.template.in"

ls
