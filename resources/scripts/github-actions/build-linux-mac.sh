#!/bin/bash

set -e

os="$1"

# Determine OS.
if [[ "$os" == *"ubuntu"* ]]; then
  echo "We are building for GNU/Linux on Ubuntu."
  is_linux=true
  prefix="AppDir/usr"

  libmpv="ON"
  qtmultimedia="OFF"
  app_id="io.github.martinrotter.rssguard"
else
  echo "We are building for macOS."
  is_linux=false
  prefix="RSS Guard.app"

  libmpv="OFF"
  qtmultimedia="ON"
fi

echo "OS: $os"
USE_QT6="ON"

# Install needed dependencies.
if [ $is_linux = true ]; then
  # Qt 5.
  sudo apt-get update

  sudo apt-get -qy install appstream cmake ninja-build openssl libssl-dev 
  sudo apt-get -qy install qt6-5compat-dev qt6-base-dev-tools qt6-image-formats-plugins qt6-multimedia-dev qt6-positioning-dev linguist-qt6 qt6-tools-dev
  sudo apt-get -qy install libmpv-dev libssl-dev libsqlite3-dev alsa-base alsa-oss alsa-tools alsa-utils gstreamer1.0-alsa gstreamer1.0-plugins-bad gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly gstreamer1.0-pulseaudio gstreamer1.0-qt6 gstreamer1.0-vaapi gstreamer1.0-x libasound2-plugins libasound2-plugins-extra libasound2-dev

  
else
  # Qt 6.
  QTTARGET="mac"
  QTOS="macos"
  QTARCH="clang_64"

  QTPATH="$(pwd)/Qt"
  QTVERSION="6.8.3"
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

mkdir rssguard-build
cd rssguard-build

cmake .. --warn-uninitialized -G Ninja -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15" -DFORCE_BUNDLE_ICONS="ON" -DCMAKE_BUILD_TYPE="MinSizeRel" -DCMAKE_VERBOSE_MAKEFILE="ON" -DCMAKE_INSTALL_PREFIX="$prefix" -DREVISION_FROM_GIT="ON" -DBUILD_WITH_QT6="$USE_QT6" -DENABLE_COMPRESSED_SITEMAP="ON" -DENABLE_MEDIAPLAYER_LIBMPV="$libmpv" -DENABLE_MEDIAPLAYER_QTMULTIMEDIA="$qtmultimedia" -DFEEDLY_CLIENT_ID="$FEEDLY_CLIENT_ID" -DFEEDLY_CLIENT_SECRET="$FEEDLY_CLIENT_SECRET"
cmake --build .
cmake --install . --prefix "$prefix"

if [ $is_linux = true ]; then
  # Validate AppStream metadata.
  echo 'Validating AppStream metadata...'
  appstreamcli validate "$prefix/share/metainfo/$app_id.metainfo.xml"
  
  # ## Obtain appimagetool.
  # appimagetool_file=$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)
  # wget -c "https://github.com/$appimagetool_file"
  # chmod +x appimagetool-*.AppImage
  # mv appimagetool-*.AppImage appimagetool.AppImage

  # export VERSION=1.0

  # GH_TKN=$GITHUB_TOKEN
  # unset GITHUB_TOKEN

  ## https://github.com/QuasarApp/CQtDeployer
  # ./appimagetool.AppImage -s deploy AppDir/usr/share/applications/*.desktop
  # ./appimagetool.AppImage ./AppDir

  # export GITHUB_TOKEN=$GH_TKN

  # ## Rename AppImaage.
  # set -- R*.AppImage
  # imagename="$1"

  # imagenewname="rssguard-${git_tag}-${git_revision}-linux64.AppImage"
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

  # Rename DMG.
  set -- *.dmg
  imagename="$1"
  imagenewname="rssguard-${git_tag}-${git_revision}-mac64.dmg"

  mv "$imagename" "$imagenewname"
  ls
fi