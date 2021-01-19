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
$ProgressPreference = 'SilentlyContinue'

# Get and prepare needed dependencies.
$qt_version = "5.15.2"
$qt_link = "https://github.com/qt/qtbase/archive/$qt_version.zip"
$qt_output = "qt.zip"

$maria_version = "10.5.8"
$maria_link = "https://downloads.mariadb.org/f/mariadb-$maria_version/winx64-packages/mariadb-$maria_version-winx64.zip/from/https%3A//mirror.vpsfree.cz/mariadb/?serve"
$maria_output = "maria.zip"

Invoke-WebRequest -Uri "$qt_link" -OutFile "$qt_output"
Invoke-WebRequest -Uri "$maria_link" -OutFile "$maria_output"

& ".\resources\scripts\7za\7za.exe" x $qt_output
& ".\resources\scripts\7za\7za.exe" x $maria_output

# Download Qt itself.
$qt_path = "$old_pwd\QtBin"
pip3 install aqtinstall
aqt install -O "$qt_path" "$qt_version" "windows" "desktop" "win64_msvc2019_64"-m "qtwebengine" 

$qt_qmake = "$qt_path\bin\qmake.exe"
$env:PATH = "$qt_path\bin\;" + $env:PATH

# Build dependencies.
....

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