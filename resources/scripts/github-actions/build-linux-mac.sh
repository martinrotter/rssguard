#!/bin/bash

set -e

os="$1"

echo "OS: $os"

# Determine OS.
if [[ "$os" == *"ubuntu"* ]]; then
  echo "We are building for GNU/Linux on Ubuntu."
  is_linux=true
  os_id="linux64"
  image_suffix="AppImage"
  prefix="/usr"

  libmpv="ON"
  qtmultimedia="OFF"
  app_id="io.github.martinrotter.rssguard"
else
  echo "We are building for macOS."
  is_linux=false
  os_id="mac64"
  image_suffix="dmg"
  prefix="RSS Guard.app"

  libmpv="OFF"
  qtmultimedia="ON"
fi

USE_QT6="ON"

if [[ "$USE_QT6" == "ON" ]]; then
  qt_id="qt6"
else
  qt_id="qt5"
fi

# Install needed dependencies.
if [ $is_linux = true ]; then
  echo "No need to install additional packages."
  git config --global --add safe.directory "*"
else
  # Qt 6.
  QTTARGET="mac"
  QTOS="macos"
  QTARCH="clang_64"

  QTPATH="$(pwd)/Qt"
  QTVERSION="6.9.3"
  QTBIN="$QTPATH/$QTVERSION/$QTOS/bin"

  # Install "aqtinstall" from its master branch to have latest code.
  pip3 install -I git+https://github.com/miurahr/aqtinstall

  echo "Qt bin directory is: $QTBIN"
  echo "Qt will be installed to: $QTPATH"

  # Install Qt.
  aqt install-qt -O "$QTPATH" "$QTTARGET" "desktop" "$QTVERSION" "$QTARCH" -m "qtimageformats" "qtmultimedia" "qt5compat" "qtpositioning" "qtserialport"
  aqt install-tool -O "$QTPATH" "$QTTARGET" "desktop" "tools_cmake"
  aqt install-tool -O "$QTPATH" "$QTTARGET" "desktop" "tools_ninja"

  export QT_PLUGIN_PATH="$QTPATH/$QTVERSION/$QTOS/plugins"
  export PATH="$QTBIN:$QTPATH/Tools/CMake/CMake.app/Contents/bin:$QTPATH/Tools/Ninja:$PATH"
fi

cmake --version

# Build application and package it.
git_tag=$(git describe --tags "$(git rev-list --tags --max-count=1)")
git_revision=$(git rev-parse --short HEAD)
image_full_name="rssguard-${git_tag}-${git_revision}-${qt_id}-${os_id}.${image_suffix}"

echo "New output file name is: $image_full_name"

mkdir rssguard-build
cd rssguard-build

cmake .. --warn-uninitialized -G Ninja -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15" -DFORCE_BUNDLE_ICONS="ON" -DCMAKE_BUILD_TYPE="MinSizeRel" -DCMAKE_VERBOSE_MAKEFILE="ON" -DCMAKE_INSTALL_PREFIX="$prefix" -DREVISION_FROM_GIT="ON" -DBUILD_WITH_QT6="$USE_QT6" -DENABLE_COMPRESSED_SITEMAP="ON" -DENABLE_MEDIAPLAYER_LIBMPV="$libmpv" -DENABLE_MEDIAPLAYER_QTMULTIMEDIA="$qtmultimedia" -DFEEDLY_CLIENT_ID="$FEEDLY_CLIENT_ID" -DFEEDLY_CLIENT_SECRET="$FEEDLY_CLIENT_SECRET"
cmake --build .
cmake --install . --prefix "$prefix"

if [ $is_linux = true ]; then
  # Validate AppStream metadata.
  echo 'Validating AppStream metadata...'
  appstreamcli validate "$prefix/share/metainfo/$app_id.metainfo.xml"
  
  URUNTIME="https://raw.githubusercontent.com/pkgforge-dev/Anylinux-AppImages/refs/heads/main/useful-tools/uruntime2appimage.sh"
  SHARUN="https://raw.githubusercontent.com/pkgforge-dev/Anylinux-AppImages/refs/heads/main/useful-tools/quick-sharun.sh"
  
  export DESKTOP=$prefix/share/applications/io.github.martinrotter.rssguard.desktop
  export DEPLOY_OPENGL=1

  echo "Desktop file: $DESKTOP"

  wget --retry-connrefused --tries=30 "$SHARUN" -O ./quick-sharun
  chmod +x ./quick-sharun

  wget --retry-connrefused --tries=30 "$URUNTIME" -O ./uruntime2appimage
  chmod +x ./uruntime2appimage

  #mkdir -p "./AppDir/shared/lib/qt6/plugins/sqldrivers"
  #cp -v -R "/usr/lib/qt6/plugins/sqldrivers" "./AppDir/shared/lib/qt6/plugins/"

  ./quick-sharun /usr/bin/rssguard /usr/lib/rssguard/*
  ./uruntime2appimage

  set -- *.AppImage
else
  # Fix .dylib linking.
  otool -L "$prefix/Contents/MacOS/rssguard"

  install_name_tool -add_rpath "@executable_path" "$prefix/Contents/MacOS/rssguard"
  install_name_tool -add_rpath "@executable_path/../Frameworks" "$prefix/Contents/MacOS/rssguard"

  otool -L "$prefix/Contents/MacOS/rssguard"
  
  # Try to self-sign the app.
  #codesign -v --deep -fs - "$prefix"

  # Deploy to DMG.
  macdeployqt "$prefix" -dmg

  set -- *.dmg
fi

image_generated_name="$1"
mv "$image_generated_name" "$image_full_name"
ls