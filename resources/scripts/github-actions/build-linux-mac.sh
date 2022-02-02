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

# Prepare environment.
if [ $is_linux = true ]; then
  sudo add-apt-repository ppa:beineri/opt-qt-5.15.2-bionic -y
  sudo apt-get update

  sudo apt-get -qy install qt515tools qt515base qt515webengine qt515svg qt515multimedia 
  sudo apt-get -qy install cmake ninja-build openssl libssl-dev libgl1-mesa-dev gstreamer1.0-alsa gstreamer1.0-plugins-good gstreamer1.0-plugins-base gstreamer1.0-plugins-bad gstreamer1.0-qt5 gstreamer1.0-pulseaudio
  
  source /opt/qt515/bin/qt515-env.sh
else
  pip3 install aqtinstall
  
  QTPATH="$(pwd)/Qt"
  QTVERSION="5.15.2"
  QTBIN="$QTPATH/$QTVERSION/clang_64/bin"

  echo "Qt bin directory is: $QTBIN"
  echo "Qt will be installed to: $QTPATH"

  aqt install-qt -O "$QTPATH" "mac" "desktop" "$QTVERSION" "clang_64" -m "qtwebengine"
  aqt install-tool -O "$QTPATH" "mac" "desktop" "tools_cmake"
  aqt install-tool -O "$QTPATH" "mac" "desktop" "tools_ninja"

  export QT_PLUGIN_PATH="$QTPATH/$QTVERSION/clang_64/plugins"
  export PATH="$QTBIN:$QTPATH/Tools/CMake/bin:$QTPATH/Tools/Ninja:$PATH"
fi

cmake --version

# Build application and package it.
git_tag=$(git describe --tags $(git rev-list --tags --max-count=1))
git_revision=$(git rev-parse --short HEAD)

mkdir rssguard-build && cd rssguard-build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DREVISION_FROM_GIT=ON -DUSE_WEBENGINE="$webengine" -DFEEDLY_CLIENT_ID="$FEEDLY_CLIENT_ID" -DFEEDLY_CLIENT_SECRET="$FEEDLY_CLIENT_SECRET" -DGMAIL_CLIENT_ID="$GMAIL_CLIENT_ID" -DGMAIL_CLIENT_SECRET="$GMAIL_CLIENT_SECRET" -DINOREADER_CLIENT_ID="$INOREADER_CLIENT_ID" -DINOREADER_CLIENT_SECRET="$INOREADER_CLIENT_SECRET"
cmake --build .
cmake --install . --prefix "$prefix"

if [ $is_linux = true ]; then
  # Obtain linuxdeployqt.
  wget -qc https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
  chmod a+x linuxdeployqt-continuous-x86_64.AppImage 

  # Copy Gstreamer libs.
  install -v -Dm755 "/usr/lib/x86_64-linux-gnu/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner" "AppDir/usr/lib/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner"
  gst_executables="-executable=AppDir/usr/lib/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner"

  for plugin in /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgst*.so; do
    basen=$(basename "$plugin")
    install -v -Dm755 "$plugin" "AppDir/usr/lib/gstreamer-1.0/$basen"
    gst_executables="${gst_executables} -executable=AppDir/usr/lib/gstreamer-1.0/$basen"
  done

  echo "Gstream command line for AppImage is: $gst_executables"

  # Create AppImage.
  unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
  ./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -bundle-non-qt-libs -no-translations $gst_executables
  ./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -bundle-non-qt-libs -no-translations $gst_executables

  if [[ "$webengine" == "ON" ]]; then
    # Copy some NSS3 files to prevent WebEngine crashes.
    cp /usr/lib/x86_64-linux-gnu/nss/* ./AppDir/usr/lib/ -v
  fi

  ./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -appimage -no-translations $gst_executables

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
  install_name_tool -change "librssguard.dylib" "@executable_path/librssguard.dylib" "RSS Guard.app/Contents/MacOS/rssguard"

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
