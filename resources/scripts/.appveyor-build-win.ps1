# List startup folder.
$old_pwd=$pwd.Path
$ssl_bin = "C:\OpenSSL-v111-Win64\bin"
$mysql_dir = "C:\Program Files\MySQL\MySQL Server 5.7"

ls "$ssl_bin"
ls "$mysql_dir\lib"

ls "$env:QTDIR"
ls "$env:QTDIR\.."

ls
echo "Qmake args are: '$env:qmake_args'."

# Setup env path with qmake.
$env:PATH = "$env:QTDIR\bin;" + $env:PATH

# Build MySQL Qt plugin.
$qt_ver = "5.14"
$qt_rev = "2"
$qtbase_url = "https://download.qt.io/archive/qt/$qt_ver/$qt_ver.$qt_rev/submodules/qtbase-everywhere-src-$qt_ver.$qt_rev.zip"
$output = "qt.zip"

mkdir "build-mysql"
cd "build-mysql"

Invoke-WebRequest -Uri $qtbase_url -OutFile $output

& "..\resources\scripts\7za\7za.exe" x $output

$qt_mysql_dir = "./qtbase-everywhere-src-5.14.2/src/plugins/sqldrivers"
$mysql_d_rev = $mysql_dir.Replace('\', '/')
$qmake_args_mysql = "MYSQL_INCDIR=$mysql_d_rev/include MYSQL_LIBDIR=$mysql_d_rev/lib"

echo "$qmake_args_mysql"

cd "$qt_mysql_dir"

qmake.exe -- $qmake_args_mysql

nmake.exe sub-mysql
ls

mkdir "rssguard-build"
cd "rssguard-build"
qmake.exe ..\build.pro "$env:qmake_args"
nmake.exe

cd "src\rssguard"
nmake.exe install

cd "app"
windeployqt.exe --verbose 1 --compiler-runtime --no-translations --release rssguard.exe librssguard.dll
cd ".."

# Copy OpenSSL.
Copy-Item -Path "$ssl_bin\libcrypto*.dll" -Destination ".\app\"
Copy-Item -Path "$ssl_bin\libssl*.dll" -Destination ".\app\"

# Copy MySQL Qt plugin.


nmake.exe windows_all
cd "$old_pwd"