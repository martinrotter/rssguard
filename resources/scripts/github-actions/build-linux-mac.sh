#!/bin/bash

os="$1"
webengine="$2"

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
  sudo add-apt-repository ppa:beineri/opt-qt-5.15.2-xenial -y

  sudo apt-get update
  sudo apt-get -y install gcc-7 g++-7 qt515tools qt515base qt515webengine
  
  source /opt/qt515/bin/qt515-env.sh
else
  pip3 install aqtinstall
  
  QTPATH="$(pwd)/Qt"
  QTVERSION="5.15.2"
  QTBIN="$QTPATH/$QTVERSION/clang_64/bin"

  echo "Qt bin directory is: $QTBIN"
  echo "Qt will be installed to: $QTPATH"

  aqt install -O "$QTPATH" 5.15.2 mac desktop clang_64 -m qtwebengine

  export QT_PLUGIN_PATH="$QTPATH/$QTVERSION/clang_64/plugins"
  export PATH="$QTBIN:$PATH"

  qmake --version
fi

# Build application and package it.
if [ $is_linux = true ]; then
  mkdir rssguard-build && cd rssguard-build
  qmake .. "USE_WEBENGINE=$webengine"
  make
  make install
  cd "src/rssguard"
else
  mkdir rssguard-build && cd rssguard-build
  qmake .. "USE_WEBENGINE=$webengine"
  make
  make install
  cd "src/rssguard"

  # Fix .dylib linking.
  install_name_tool -change "librssguard.dylib" "@executable_path/librssguard.dylib" "RSS Guard.app/Contents/MacOS/rssguard"
  install_name_tool -change "librssguard.dylib" "@executable_path/librssguard.dylib" "rssguard"

  otool -L "RSS Guard.app/Contents/MacOS/rssguard"
  macdeployqt "./RSS Guard.app" -dmg

  # Rename DMG.
  set -- *.dmg
  
  dmgname="$1"
  git_tag=$(git describe --tags `git rev-list --tags --max-count=1`)
  git_revision=$(git rev-parse --short HEAD)

  #if [ "$USE_WEBENGINE" = true ]; then
  #  dmgnewname="rssguard-${git_tag}-${git_revision}-mac64.dmg"
  #else
  #  dmgnewname="rssguard-${git_tag}-${git_revision}-nowebengine-mac64.dmg"
  #fi

  #mv "$dmgname" "$dmgnewname"
fi