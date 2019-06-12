# List startup folder.
$old_pwd=$pwd.Path
ls
echo "Qmake args are: '$env:qmake_args'."

mkdir "rssguard-build"
cd "rssguard-build"
qmake.exe ..\build.pro "$env:qmake_args"
nmake.exe

cd "src\rssguard"
nmake.exe windows_all
cd "$old_pwd"