#!/bin/bash

os="$1"
webengine="$2"

# Determine OS.
if [[ "$os" == *"ubuntu"* ]]; then
  echo "We are building for GNU/Linux on Ubuntu."
  is_linux=true
else
  echo "We are building for Mac OS X."
  is_linux=false
fi

echo "OS: $os; WebEngine: $webengine"

# Prepare environment.
if [ $is_linux = true ]; then
  sudo add-apt-repository ppa:beineri/opt-qt-5.15.2-bionic -y
  sudo apt-get update

  sudo apt-get -y install qt515tools qt515base qt515webengine qt515svg qt515multimedia 
  sudo apt-get -y install openssl libssl-dev libgl1-mesa-dev gstreamer1.0-alsa gstreamer1.0-plugins-good gstreamer1.0-plugins-base gstreamer1.0-plugins-bad gstreamer1.0-qt5 gstreamer1.0-pulseaudio
  
  source /opt/qt515/bin/qt515-env.sh
else
  pip3 install aqtinstall
  
  QTPATH="$(pwd)/Qt"
  QTVERSION="5.15.2"
  QTBIN="$QTPATH/$QTVERSION/clang_64/bin"

  echo "Qt bin directory is: $QTBIN"
  echo "Qt will be installed to: $QTPATH"

  aqt install-qt -O "$QTPATH" "mac" "desktop" "$QTVERSION" "clang_64" -m "qtwebengine"

  export QT_PLUGIN_PATH="$QTPATH/$QTVERSION/clang_64/plugins"
  export PATH="$QTBIN:$PATH"
fi

qmake --version

# Build application and package it.
git_tag=$(git describe --tags `git rev-list --tags --max-count=1`)
git_revision=$(git rev-parse --short HEAD)

mkdir rssguard-build && cd rssguard-build
qmake .. "USE_WEBENGINE=$webengine" "FEEDLY_CLIENT_ID=$FEEDLY_CLIENT_ID" "FEEDLY_CLIENT_SECRET=$FEEDLY_CLIENT_SECRET" "GMAIL_CLIENT_ID=$GMAIL_CLIENT_ID" "GMAIL_CLIENT_SECRET=$GMAIL_CLIENT_SECRET" "INOREADER_CLIENT_ID=$INOREADER_CLIENT_ID" "INOREADER_CLIENT_SECRET=$INOREADER_CLIENT_SECRET"
make
make install
cd "src/rssguard"
  
if [ $is_linux = true ]; then
  # Obtain linuxdeployqt.
  wget -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
  chmod a+x linuxdeployqt-continuous-x86_64.AppImage 

  # Copy Gstreamer libs.
  install -v -Dm755 "/usr/lib/x86_64-linux-gnu/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner" "AppDir/usr/lib/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner"
  gst_executables="-executable=AppDir/usr/lib/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner"

  for plugin in $(ls /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgst*.so); do
    basen=$(basename "$plugin")
    install -v -Dm755 "$plugin" "AppDir/usr/lib/gstreamer-1.0/$basen"
    gst_executables="${gst_executables} -executable=AppDir/usr/lib/gstreamer-1.0/$basen"
  done

  echo "Gstream command line for AppImage is: $gst_executables"

  # Create AppImage.
  unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
  ./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -bundle-non-qt-libs -no-translations $gst_executables
  ./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -bundle-non-qt-libs -no-translations $gst_executables

  if [[ "$webengine" == "true" ]]; then
    # Copy some NSS3 files to prevent WebEngine crashes.
    cp /usr/lib/x86_64-linux-gnu/nss/* ./AppDir/usr/lib/ -v
  fi

  ./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -appimage -no-translations $gst_executables

  # Rename AppImaage.
  set -- R*.AppImage
  imagename="$1"
  
  if [[ "$webengine" == "true" ]]; then
    imagenewname="rssguard-${git_tag}-${git_revision}-linux64.AppImage"
  else
    imagenewname="rssguard-${git_tag}-${git_revision}-nowebengine-linux64.AppImage"
  fi
else
  # Fix .dylib linking.
  install_name_tool -change "librssguard.dylib" "@executable_path/librssguard.dylib" "RSS Guard.app/Contents/MacOS/rssguard"
  install_name_tool -change "librssguard.dylib" "@executable_path/librssguard.dylib" "rssguard"

  otool -L "RSS Guard.app/Contents/MacOS/rssguard"
  macdeployqt "./RSS Guard.app" -dmg

  # Rename DMG.
  set -- *.dmg
  imagename="$1"

  if [[ "$webengine" == "true" ]]; then
    imagenewname="rssguard-${git_tag}-${git_revision}-mac64.dmg"
  else
    imagenewname="rssguard-${git_tag}-${git_revision}-nowebengine-mac64.dmg"
  fi
fi

mv "$imagename" "$imagenewname"
ls
