$os = $args[0]
$webengine = $args[1]

echo "We are building for MS Windows."
echo "OS: $os; WebEngine: $webengine"

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

$env:PATH = "$qt_path\bin\;" + $env:PATH