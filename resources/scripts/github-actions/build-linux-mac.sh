#!/bin/bash

set -e

os="$1"
use_qt5="$2"
webengine_viewer="$3"
script_directory="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
platform_options=()

if [[ "$use_qt5" == "ON" ]]; then
  USE_QT6="OFF"
else
  USE_QT6="ON"
fi

echo "OS: $os"

# Determine OS.
if [[ "$os" == *"ubuntu"* ]]; then
  echo "We are building for GNU/Linux on Ubuntu."
  is_linux=true
  os_id="linux64"
  image_suffix="AppImage"
  prefix="/usr"

if [[ "$use_qt5" == "ON" ]]; then
  qxmpp="OFF"
else
  qxmpp="ON"
fi

  libmpv="ON"
  qtmultimedia="OFF"
else
  echo "We are building for macOS."
  if [[ "$(uname -s)" != "Darwin" || "$use_qt5" != "OFF" ]]; then
    echo "Official macOS packages must be built on macOS with Qt 6."
    exit 1
  fi

  case "${MACOS_ARCHITECTURE:-}" in
    arm64) expected_suffix="mac64arm"; go_architecture="arm64" ;;
    x86_64) expected_suffix="mac64intel"; go_architecture="amd64" ;;
    *) echo "Set MACOS_ARCHITECTURE to arm64 or x86_64."; exit 1 ;;
  esac

  if [[ "$(uname -m)" != "$MACOS_ARCHITECTURE" || "${MACOS_PACKAGE_SUFFIX:-}" != "$expected_suffix" ]]; then
    echo "The runner, requested architecture and package suffix must agree."
    exit 1
  fi

  export GOOS="darwin"
  export GOARCH="$go_architecture"
  export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-13.0}"
  platform_options+=("-DCMAKE_OSX_ARCHITECTURES=$MACOS_ARCHITECTURE" "-DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET" "-DENABLE_PACKAGE_SMOKE_TEST=ON")
  is_linux=false
  os_id="$expected_suffix"
  image_suffix="dmg"
  prefix="RSS Guard.app"

  qxmpp="OFF"
  libmpv="OFF"
  qtmultimedia="ON"
fi

if [[ "$USE_QT6" == "ON" ]]; then
  qt_id="qt6"
else
  qt_id="qt5"
fi

if [[ "$webengine_viewer" == "ON" ]]; then
  app_id="io.github.martinrotter.rssguard"
  variant_id="web"
else
  app_id="io.github.martinrotter.rssguardlite"
  variant_id="text"
fi

# Install needed dependencies.
if [ $is_linux = true ]; then
  echo "No need to install additional packages."
  git config --global --add safe.directory "*"
  unit_tests="ON"
  benchmarks="ON"
else
  # Qt 6.
  QTTARGET="mac"
  QTOS="macos"
  QTARCH="clang_64"

  QTPATH="$(pwd)/Qt"
  QTVERSION="6.11.2"
  QTBIN="$QTPATH/$QTVERSION/$QTOS/bin"

  brew install aqtinstall go icu4c
  icu_root="$(bash "$script_directory/prepare-macos-icu.sh" "$MACOS_ARCHITECTURE" "$MACOSX_DEPLOYMENT_TARGET")"
  platform_options+=("-DICU_ROOT=$icu_root")

  echo "Qt bin directory is: $QTBIN"
  echo "Qt will be installed to: $QTPATH"

  # Install Qt.
  aqt install-qt -O "$QTPATH" "$QTTARGET" "desktop" "$QTVERSION" "$QTARCH" -m "qtimageformats" "qtmultimedia" "qt5compat" "qtpositioning" "qtserialport" "qtwebengine" "qtwebchannel"
  aqt install-tool -O "$QTPATH" "$QTTARGET" "desktop" "tools_cmake"
  aqt install-tool -O "$QTPATH" "$QTTARGET" "desktop" "tools_ninja"

  # clang_64 is Qt's macOS SDK name; check the actual CPU slices it contains.
  lipo -verify_arch "$MACOS_ARCHITECTURE" "$QTPATH/$QTVERSION/$QTOS/lib/QtCore.framework/Versions/A/QtCore"

  export QT_PLUGIN_PATH="$QTPATH/$QTVERSION/$QTOS/plugins"
  export PATH="$QTBIN:$QTPATH/Tools/CMake/CMake.app/Contents/bin:$QTPATH/Tools/Ninja:$PATH"
  unit_tests="OFF"
  benchmarks="OFF"
fi

cmake --version

# Build application and package it.
git_tag=$(git describe --tags --abbrev=0)
git_revision=$(git rev-parse --short HEAD)
devbuild_opt=$([[ "$GITHUB_REF" =~ ^refs/tags/[0-9] ]] && echo "OFF" || echo "ON")

if [[ "$devbuild_opt" == "ON" ]]; then
  image_full_name="rssguard-dev-${git_revision}-${variant_id}-${qt_id}-${os_id}.${image_suffix}"
else
  # This GitHub Actions run was triggered by pushing a tag that starts with a number, so assume it's a stable release.
  image_full_name="rssguard-${git_tag}-${variant_id}-${qt_id}-${os_id}.${image_suffix}"
fi

echo "New output file name is: $image_full_name"

mkdir rssguard-build
cd rssguard-build

cmake .. -G Ninja "${platform_options[@]}" -DFORCE_BUNDLE_ICONS="ON" -DCMAKE_BUILD_TYPE="MinSizeRel" -DCMAKE_VERBOSE_MAKEFILE="ON" -DCMAKE_INSTALL_PREFIX="$prefix" -DREVISION_FROM_GIT="$devbuild_opt" -DBUILD_WITH_QT6="$USE_QT6" -DWEB_ARTICLE_VIEWER_WEBENGINE="$webengine_viewer" -DBUILD_XMPP_PLUGIN="$qxmpp" -DUSE_SYSTEM_QXMPP="ON" -DENABLE_TESTING="$unit_tests" -DENABLE_BENCHMARKS="$benchmarks" -DENABLE_COMPRESSED_SITEMAP="ON" -DIS_DEVBUILD="$devbuild_opt" -DENABLE_ICU="ON" -DENABLE_MEDIAPLAYER_LIBMPV="$libmpv" -DENABLE_MEDIAPLAYER_QTMULTIMEDIA="$qtmultimedia" -DFEEDLY_CLIENT_ID="$FEEDLY_CLIENT_ID" -DFEEDLY_CLIENT_SECRET="$FEEDLY_CLIENT_SECRET"
cmake --build .
cmake --install . --prefix "$prefix"

if [ $is_linux = true ]; then
  # Validate AppStream metadata.
  echo 'Validating AppStream metadata...'
  appstreamcli validate "$prefix/share/metainfo/$app_id.metainfo.xml"
  
  SHARUN="https://raw.githubusercontent.com/pkgforge-dev/Anylinux-AppImages/refs/heads/main/useful-tools/quick-sharun.sh"
  
  export OUTNAME=$image_full_name
  export UPINFO="gh-releases-zsync|${GITHUB_REPOSITORY%/*}|${GITHUB_REPOSITORY#*/}|latest|rssguard-*-${variant_id}-${qt_id}-${os_id}.${image_suffix}.zsync"
  export VERSION=$git_tag
  export ICON=$prefix/share/icons/hicolor/512x512/apps/$app_id.png
  export DESKTOP=$prefix/share/applications/$app_id.desktop
  export DEPLOY_OPENGL=1
  export OUTPUT_APPIMAGE=1

  echo "Outname: $OUTNAME"
  echo "Upinfo: $UPINFO"
  echo "Version: $VERSION"
  echo "Desktop file: $DESKTOP"

  wget --retry-connrefused --tries=30 "$SHARUN" -O ./quick-sharun
  chmod +x ./quick-sharun

  ./quick-sharun /usr/*/rssguard* /usr/lib/libmujs.so*
else
  mkdir -p "$prefix/Contents/Frameworks"
  mv "$prefix/Contents/MacOS/librssguard.dylib" "$prefix/Contents/Frameworks/"

  otool -L "$prefix/Contents/Frameworks/librssguard.dylib"
  otool -L "$prefix/Contents/MacOS/rssguard"

  # Official macOS packages support SQLite only.
  test -f "$QT_PLUGIN_PATH/sqldrivers/libqsqlite.dylib"
  find "$QT_PLUGIN_PATH/sqldrivers" -type f ! -name "libqsqlite.dylib" -print -delete

  # Deploy the CI probe with the same libraries, then keep it out of release packages.
  cp ./src/rssguard/rssguard-package-smoke "$prefix/Contents/MacOS/"
  macdeployqt "$prefix" -executable="$prefix/Contents/MacOS/rssguard-package-smoke" -verbose=2
  mv "$prefix/Contents/MacOS/rssguard-package-smoke" ./rssguard-package-smoke

  # Sign the final bundle after removing the probe, and deploy to DMG.
  macdeployqt "$prefix" -dmg -verbose=2 -codesign=-

  otool -L "$prefix/Contents/Frameworks/librssguard.dylib"
  otool -L "$prefix/Contents/MacOS/rssguard"

  find "$prefix" -print | sed -e 's;[^/]*/;|____;g;s;____|; |;g'

  set -- *.dmg
  image_generated_name="$1"
  mv "$image_generated_name" "$image_full_name"
fi

ls
