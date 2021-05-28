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
  sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
  sudo add-apt-repository ppa:beineri/opt-qt-5.14.2-xenial -y

  sudo apt-get update
  sudo apt-get -y install gcc-7 g++-7 qt514tools qt514base qt514webengine qt514svg qt514multimedia 
  sudo apt-get -y install openssl libssl-dev libgl1-mesa-dev 

  sudo update-alternatives --remove-all gcc 
  sudo update-alternatives --remove-all g++
  sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 50
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 50
  
  source /opt/qt514/bin/qt514-env.sh
else
  pip3 install aqtinstall
  
  QTPATH="$(pwd)/Qt"
  QTVERSION="5.15.2"
  QTBIN="$QTPATH/$QTVERSION/clang_64/bin"

  echo "Qt bin directory is: $QTBIN"
  echo "Qt will be installed to: $QTPATH"

  aqt install -O "$QTPATH" "5.15.2" "mac" "desktop" "clang_64" -m "qtwebengine" "qtmultimedia"

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

  # Create AppImage.
  unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
  ./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -bundle-non-qt-libs -no-translations

  if [[ "$webengine" == "true" ]]; then
    # Copy some NSS3 files to prevent WebEngine crashes.
    cp /usr/lib/x86_64-linux-gnu/nss/* ./AppDir/usr/lib/ -v
  fi

  ./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -appimage -no-translations

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