$ErrorActionPreference = "Stop"
$PSNativeCommandUseErrorActionPreference = $true

$os = $args[0]
$use_qt5 = $args[1]
$webengine_viewer = $args[2]

echo "We are packaging for MS Windows."
echo "OS: $os; Qt5: $use_qt5; WebEngine: $webengine_viewer"

$git_revlist = git rev-list --tags --max-count=1
$git_tag = git describe --tags $git_revlist
$git_revision = git rev-parse --short HEAD
$old_pwd = $pwd.Path
$is_devbuild = $env:GITHUB_REF -notmatch '^refs/tags/[0-9]'

$7za = "$old_pwd\resources\scripts\7za\7za.exe"
$nsis = "$old_pwd\resources\scripts\nsis\makensis.exe"

# Compile an exact list of packaged paths; it also validates required files before archiving.
$prepare_installer = "$old_pwd\resources\scripts\github-actions\prepare-windows-installer.py"

cd "rssguard-build"

if ($is_devbuild) {
  $packagebase = "rssguard-dev-$git_revision"
}
else {
  $packagebase = "rssguard-$git_tag"
}

if ($webengine_viewer -eq "ON") {
  $packagebase += "-web"
}
else {
  $packagebase += "-text"
}

if ($use_qt5 -eq "ON") {
  $packagebase += "-qt5-win7"
}
else {
  $packagebase += "-qt6-win10"
}

& python "$prepare_installer" --stage ".\app" --definitions ".\NSIS.definitions.nsh" --output ".\NSIS.payload.nsh"
if ($LASTEXITCODE -ne 0) { throw "Windows installer payload validation failed." }

# Create 7zip package (including debugging symbols).
& "$7za" a -t7z -mmt -mx9 "$packagebase.7z" ".\app\*"

# Create NSIS installation package.
& "$nsis" /NOCONFIG /WX "/XOutFile $packagebase.exe" ".\NSIS.template.in"

ls
