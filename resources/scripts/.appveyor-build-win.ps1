# List startup folder.
$old_pwd=$pwd.Path

ls "C:\OpenSSL-v111-Win64\bin"
ls "C:\Program Files\MySQL\MySQL Server 5.7\lib"

ls "$env:QTDIR"

ls
echo "Qmake args are: '$env:qmake_args'."

# Setup env path with qmake.
$env:PATH = "$env:QTDIR\bin;" + $env:PATH

mkdir "rssguard-build"
cd "rssguard-build"
qmake.exe ..\build.pro "$env:qmake_args"
nmake.exe

cd "src\rssguard"
nmake.exe install

cd "app"
windeployqt.exe --verbose 1 --compiler-runtime --no-translations --release rssguard.exe librssguard.dll
cd ".."
nmake.exe windows_all
cd "$old_pwd"