#!/bin/bash

set -e

os="$1"
webengine="$2"

# Determine OS.
if [[ "$os" == *"ubuntu"* ]]; then
  echo "We are building for GNU/Linux on Ubuntu."
  is_linux=true
  prefix="AppDir/usr"

  if [[ "$webengine" == "ON" ]]; then
    libmpv="ON"
    qtmultimedia="OFF"
    app_id="io.github.martinrotter.rssguard"
  else
    libmpv="OFF"
    qtmultimedia="ON"
    app_id="io.github.martinrotter.rssguardlite"
  fi
else
  echo "We are building for macOS."
  is_linux=false
  prefix="RSS Guard.app"

  libmpv="OFF"
  qtmultimedia="ON"
fi

echo "OS: $os; Not lite: $webengine"

# Install needed dependencies.
if [ $is_linux = true ]; then
  # Qt 5.
  QTTARGET="linux"
  QTOS="gcc_64"
  QTARCH="gcc_64"
  USE_QT6="OFF"

  sudo add-apt-repository ppa:beineri/opt-qt-5.15.4-focal -y
  sudo apt-get update

  sudo apt-get -qy install qt515tools qt515base qt515webengine qt515svg qt515multimedia qt515imageformats appstream
  sudo apt-get -qy install cmake ninja-build openssl libssl-dev libgl1-mesa-dev gstreamer1.0-alsa gstreamer1.0-nice gstreamer1.0-plugins-good gstreamer1.0-plugins-base gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-qt5 gstreamer1.0-pulseaudio libmpv-dev

  # The script below performs some broken testing, which ends up tripping 'set -e'.
  # So we temporarily ignore errors when sourcing the script, and re-enable them afterward.
  set +e
  # shellcheck source=/dev/null
  source /opt/qt515/bin/qt515-env.sh
  set -e
else
  # Qt 6.
  QTTARGET="mac"
  QTOS="macos"
  QTARCH="clang_64"
  USE_QT6="ON"

  QTPATH="$(pwd)/Qt"
  QTVERSION="6.8.2"
  QTBIN="$QTPATH/$QTVERSION/$QTOS/bin"

  # Install "aqtinstall" from its master branch to have latest code.
  pip3 install -I git+https://github.com/miurahr/aqtinstall

  echo "Qt bin directory is: $QTBIN"
  echo "Qt will be installed to: $QTPATH"

  # Install Qt.
  aqt -c 'aqt/settings.ini' install-qt -O "$QTPATH" "$QTTARGET" "desktop" "$QTVERSION" "$QTARCH" -m "qtwebengine" "qtimageformats" "qtwebchannel" "qtmultimedia" "qt5compat" "qtpositioning" "qtserialport"
  aqt -c 'aqt/settings.ini' install-tool -O "$QTPATH" "$QTTARGET" "desktop" "tools_cmake"
  aqt -c 'aqt/settings.ini' install-tool -O "$QTPATH" "$QTTARGET" "desktop" "tools_ninja"

  export QT_PLUGIN_PATH="$QTPATH/$QTVERSION/$QTOS/plugins"
  export PATH="$QTBIN:$QTPATH/Tools/CMake/CMake.app/Contents/bin:$QTPATH/Tools/Ninja:$PATH"
fi

cmake --version

# Build application and package it.
git_tag=$(git describe --tags "$(git rev-list --tags --max-count=1)")
git_revision=$(git rev-parse --short HEAD)

mkdir rssguard-build
cd rssguard-build

cmake .. --warn-uninitialized -G Ninja -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15" -DFORCE_BUNDLE_ICONS="ON" -DCMAKE_BUILD_TYPE="MinSizeRel" -DCMAKE_VERBOSE_MAKEFILE="ON" -DCMAKE_INSTALL_PREFIX="$prefix" -DREVISION_FROM_GIT="ON" -DBUILD_WITH_QT6="$USE_QT6" -DENABLE_COMPRESSED_SITEMAP="ON" -DENABLE_MEDIAPLAYER_LIBMPV="$libmpv" -DENABLE_MEDIAPLAYER_QTMULTIMEDIA="$qtmultimedia" -DNO_LITE="$webengine" -DFEEDLY_CLIENT_ID="$FEEDLY_CLIENT_ID" -DFEEDLY_CLIENT_SECRET="$FEEDLY_CLIENT_SECRET" -DGMAIL_CLIENT_ID="$GMAIL_CLIENT_ID" -DGMAIL_CLIENT_SECRET="$GMAIL_CLIENT_SECRET"
cmake --build .
cmake --install . --prefix "$prefix"

if [ $is_linux = true ]; then
  # Validate AppStream metadata.
  echo 'Validating AppStream metadata...'
  appstreamcli validate "$prefix/share/metainfo/$app_id.metainfo.xml"
  # Obtain linuxdeployqt.
  wget -qc https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
  chmod a+x linuxdeployqt-continuous-x86_64.AppImage 

  # Copy GStreamer libs.
  install -v -Dm755 "/usr/lib/x86_64-linux-gnu/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner" "$prefix/lib/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner"
  gst_executables=("-executable=$prefix/lib/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner")

  for plugin in /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgst*.so; do
    basen=$(basename "$plugin")
    install -v -Dm755 "$plugin" "$prefix/lib/gstreamer-1.0/$basen"
    gst_executables+=("-executable=$prefix/lib/gstreamer-1.0/$basen")
  done

  echo "GStreamer command line for AppImage is: ${gst_executables[*]}"

  # Create AppImage.
  unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH

  # Run the Apppmage tool twice to include missing dependencies for GStreamer.
  # See: https://github.com/probonopd/linuxdeployqt/issues/123#issuecomment-346934117
  ./linuxdeployqt-continuous-x86_64.AppImage "$prefix/share/applications/$app_id.desktop" -bundle-non-qt-libs -no-translations "${gst_executables[@]}"
  ./linuxdeployqt-continuous-x86_64.AppImage "$prefix/share/applications/$app_id.desktop" -bundle-non-qt-libs -no-translations "${gst_executables[@]}"

  if [[ "$webengine" == "ON" ]]; then
    # Copy some NSS3 files to prevent WebEngine crashes.
    cp /usr/lib/x86_64-linux-gnu/nss/* "$prefix/lib/" -v
  fi

  ./linuxdeployqt-continuous-x86_64.AppImage "$prefix/share/applications/$app_id.desktop" -appimage -no-translations "${gst_executables[@]}"

  # Rename AppImaage.
  set -- R*.AppImage
  imagename="$1"
  
  if [[ "$webengine" == "ON" ]]; then
    imagenewname="rssguard-${git_tag}-${git_revision}-linux64.AppImage"
  else
    imagenewname="rssguard-${git_tag}-${git_revision}-lite-linux64.AppImage"
  fi
else
  # Fix .dylib linking.
  otool -L "$prefix/Contents/MacOS/rssguard"

  install_name_tool -add_rpath "@executable_path" "$prefix/Contents/MacOS/rssguard"
  install_name_tool -add_rpath "@executable_path/../Frameworks" "$prefix/Contents/MacOS/rssguard"

  otool -L "$prefix/Contents/MacOS/rssguard"
  
  macdeployqt "$prefix" -dmg

  # Rename DMG.
  set -- *.dmg
  imagename="$1"

  if [[ "$webengine" == "ON" ]]; then
    imagenewname="rssguard-${git_tag}-${git_revision}-mac64.dmg"
  else
    imagenewname="rssguard-${git_tag}-${git_revision}-lite-mac64.dmg"
  fi
fi

mv "$imagename" "$imagenewname"
ls