$old_pwd = $pwd.Path

# Get Qt.
$qt_version = "5.15.1"
$qt_stub = "qt-$qt_version-dynamic-msvc2019-x86_64"
$qt_link = "https://github.com/martinrotter/qt5-minimalistic-builds/releases/download/$qt_version/$qt_stub.7z"
$qt_output = "qt.7z"

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri $qt_link -OutFile $qt_output
& ".\resources\scripts\7za\7za.exe" x $qt_output

$qt_path = (Resolve-Path $qt_stub).Path
$qt_qmake = "$qt_path\bin\qmake.exe"

cd "$qt_stub\bin\"
& ".\qtbinpatcher.exe"
cd "$old_pwd"

$env:PATH = "$qt_path\bin\;" + $env:PATH

# Build RSS Guard itself.
echo "qmake args are: '$env:qmake_args'."

mkdir "rssguard-build"
cd "rssguard-build"
& "$qt_qmake" "..\build.pro" "-r" "$env:qmake_args" "CONFIG-=debug" "CONFIG-=debug_and_release" "CONFIG*=release"
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

nmake.exe windows_all
cd "$old_pwd"