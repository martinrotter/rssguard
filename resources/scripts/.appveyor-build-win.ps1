# List startup folder.
$old_pwd=$pwd.Path

ls
echo "Qmake args are: '$env:qmake_args'."

# Setup env path with qmake.
$env:PATH = "$env:QTDIR\bin;" + $env:PATH

mkdir "rssguard-build"
cd "rssguard-build"
qmake.exe ..\build.pro "$env:qmake_args"
nmake.exe
nmake.exe install

cd "src\rssguard\app"
"$env:QTDIR\bin\windeployqt.exe --verbose 1 --compiler-runtime --no-translations --release rssguard.exe librssguard.dll"
cd ".."
nmake.exe windows_all
cd "$old_pwd"