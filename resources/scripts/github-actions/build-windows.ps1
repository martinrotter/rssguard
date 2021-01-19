$os = $args[0]
$webengine = $args[1]

echo "We are building for MS Windows."
echo "OS: $os; WebEngine: $webengine"

$git_revlist = git rev-list --tags --max-count=1
$git_tag = git describe --tags $git_revlist
$git_revision = git rev-parse --short HEAD
$old_pwd = $pwd.Path

# Prepare environment.
Install-Module Pscx -Scope CurrentUser -AllowClobber -Force
Install-Module VSSetup -Scope CurrentUser -AllowClobber -Force
Import-VisualStudioVars -Architecture x64

$AllProtocols = [System.Net.SecurityProtocolType]'Tls11,Tls12'
[System.Net.ServicePointManager]::SecurityProtocol = $AllProtocols

Invoke-WebRequest -Uri "https://downloads.mariadb.org/f/mariadb-10.5.8/winx64-packages/mariadb-10.5.8-winx64.zip/from/https%3A//mirror.vpsfree.cz/mariadb/?serve" -OutFile "maria"

Invoke-WebRequest -Uri "https://github.com/qt/qtbase/archive/5.15.2.zip" -OutFile "qt"

ls

# Get Qt.
$qt_version = "5.15.2"
$qt_stub = "qt-$qt_version-dynamic-msvc2019-x86_64"
$qt_link = "https://github.com/martinrotter/qt5-minimalistic-builds/releases/download/$qt_version/$qt_stub.7z"
$qt_output = "qt.7z"

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri $qt_link -OutFile $qt_output
& ".\resources\scripts\7za\7za.exe" x $qt_output

$qt_path = (Resolve-Path $qt_stub).Path
$qt_qmake = "$qt_path\bin\qmake.exe"

$env:PATH = "$qt_path\bin\;" + $env:PATH

# Build application.
mkdir "rssguard-build"
cd "rssguard-build"
& "$qt_qmake" "..\build.pro" "-r" "USE_WEBENGINE=$webengine" "CONFIG-=debug" "CONFIG-=debug_and_release" "CONFIG*=release"
nmake.exe

cd "src\rssguard"
nmake.exe install

cd "app"
windeployqt.exe --verbose 1 --compiler-runtime --no-translations --release rssguard.exe librssguard.dll
cd ".."

# Copy OpenSSL.
Copy-Item -Path "$qt_path\bin\libcrypto*.dll" -Destination ".\app\"
Copy-Item -Path "$qt_path\bin\libssl*.dll" -Destination ".\app\"

# Copy MySQL Qt plugin.
Copy-Item -Path "$qt_path\bin\libmariadb.dll" -Destination ".\app\"

if ($webengine -eq "true") {
  $packagebase = "rssguard-${git_tag}-${git_revision}-win64"
}
else {
  $packagebase = "rssguard-${git_tag}-${git_revision}-nowebengine-win64"
}

# Create 7zip package.
& "$old_pwd\resources\scripts\7za\7za.exe" a -t7z -mmt -mx9 "$packagebase.7z" ".\app\*"

# Create NSIS installation package.
& "$old_pwd\resources\scripts\nsis\makensis.exe" "/XOutFile $packagebase.exe" ".\NSIS.template.in"

ls