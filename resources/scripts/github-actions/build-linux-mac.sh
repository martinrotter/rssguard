#!/bin/bash

os="$1"
webengine="$2"

# Determine OS.
if [[ "$os" == *"ubuntu"* ]]; then
  echo "We are building for GNU/Linux on Ubuntu."
  is_linux=true
  prefix="AppDir/usr"
else
  echo "We are building for macOS."
  is_linux=false
  prefix="RSS Guard.app"
fi

echo "OS: $os; WebEngine: $webengine"

# Install needed dependencies.
if [ $is_linux = true ]; then
  sudo apt update

  sudo apt -qy install libgl1-mesa-dev gstreamer1.0-alsa gstreamer1.0-plugins-good gstreamer1.0-plugins-base gstreamer1.0-plugins-bad gstreamer1.0-qt5 gstreamer1.0-pulseaudio libodbc1 postgresql python3-pip

  QTTARGET="linux"
  QTOS="gcc_64"
  QTARCH="gcc_64"  
else  

  QTTARGET="mac"
  QTOS="macos"
  QTARCH="clang_64"  
fi

pip3 install aqtinstall

# Setup Qt information.
QTPATH="$(pwd)/Qt"
QTVERSION="6.3.1"
QTBIN="$QTPATH/$QTVERSION/$QTOS/bin"

export QT_PLUGIN_PATH="$QTPATH/$QTVERSION/$QTOS/plugins"
export PATH="$QTBIN:$QTPATH/Tools/CMake/bin:$QTPATH/Tools/Ninja:$PATH"

echo "Qt bin directory is: $QTBIN"
echo "Qt will be installed to: $QTPATH"

# Install Qt.
aqt install-qt -O "$QTPATH" "$QTTARGET" "desktop" "$QTVERSION" "$QTARCH" -m "qtwebengine" "qtwebchannel" "qtmultimedia" "qt5compat" "qtpositioning" "qtserialport"
aqt install-tool -O "$QTPATH" "$QTTARGET" "desktop" "tools_cmake"
aqt install-tool -O "$QTPATH" "$QTTARGET" "desktop" "tools_ninja"

if [ $is_linux = true ]; then
  aqt install-tool -O "$QTPATH" "$QTTARGET" "desktop" "tools_openssl_x64"
fi

cmake --version

# Build application and package it.
git_tag=$(git describe --tags $(git rev-list --tags --max-count=1))
git_revision=$(git rev-parse --short HEAD)

mkdir rssguard-build && cd rssguard-build
cmake .. -G Ninja -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DFORCE_BUNDLE_ICONS="ON" -DCMAKE_BUILD_TYPE="MinSizeRel" -DCMAKE_INSTALL_PREFIX="$prefix" -DREVISION_FROM_GIT="ON" -DBUILD_WITH_QT6="ON" -DUSE_WEBENGINE="$webengine" -DFEEDLY_CLIENT_ID="$FEEDLY_CLIENT_ID" -DFEEDLY_CLIENT_SECRET="$FEEDLY_CLIENT_SECRET" -DGMAIL_CLIENT_ID="$GMAIL_CLIENT_ID" -DGMAIL_CLIENT_SECRET="$GMAIL_CLIENT_SECRET" -DINOREADER_CLIENT_ID="$INOREADER_CLIENT_ID" -DINOREADER_CLIENT_SECRET="$INOREADER_CLIENT_SECRET"
cmake --build .
cmake --install . --prefix "$prefix"

if [ $is_linux = true ]; then
  # Obtain linuxdeployqt.
  wget -qc https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
  wget -qc https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
  chmod a+x linuxdeploy*.AppImage 

  # Copy Gstreamer libs.
  install -v -Dm755 "/usr/lib/x86_64-linux-gnu/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner" "AppDir/usr/lib/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner"

  for plugin in /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgst*.so; do
    basen=$(basename "$plugin")
    install -v -Dm755 "$plugin" "AppDir/usr/lib/gstreamer-1.0/$basen"
  done

  if [[ "$webengine" == "ON" ]]; then
    # Copy some NSS3 files to prevent WebEngine crashes.
    cp /usr/lib/x86_64-linux-gnu/nss/* ./AppDir/usr/lib/ -v
  fi

  # Fix some missing Qt folders.
  mkdir "$QTPATH/$QTVERSION/$QTOS/plugins/audio"
  mkdir "$QTPATH/$QTVERSION/$QTOS/plugins/mediaservice"

  # Create AppImage.
  ./linuxdeploy-x86_64.AppImage --output "appimage" --plugin "qt" --appdir "AppDir"

  # Rename AppImaage.
  set -- R*.AppImage
  imagename="$1"
  
  if [[ "$webengine" == "ON" ]]; then
    imagenewname="rssguard-${git_tag}-${git_revision}-linux64.AppImage"
  else
    imagenewname="rssguard-${git_tag}-${git_revision}-nowebengine-linux64.AppImage"
  fi
else
  # Fix .dylib linking.
  otool -L "RSS Guard.app/Contents/MacOS/rssguard"

  install_name_tool -add_rpath "@executable_path" "RSS Guard.app/Contents/MacOS/rssguard"
  install_name_tool -add_rpath "@executable_path/../Frameworks" "RSS Guard.app/Contents/MacOS/rssguard"

  otool -L "RSS Guard.app/Contents/MacOS/rssguard"
  
  macdeployqt "./RSS Guard.app" -dmg

  # Rename DMG.
  set -- *.dmg
  imagename="$1"

  if [[ "$webengine" == "ON" ]]; then
    imagenewname="rssguard-${git_tag}-${git_revision}-mac64.dmg"
  else
    imagenewname="rssguard-${git_tag}-${git_revision}-nowebengine-mac64.dmg"
  fi
fi

mv "$imagename" "$imagenewname"
ls
