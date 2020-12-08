#!/bin/bash

os="$1"
qmake_args="$2"

if [[ "$os" == *"ubuntu"* ]]; then
  echo "We are building for GNU/Linux on Ubuntu."
  is_linux=true
else
  echo "We are building for Mac OS X."
  is_linux=false
fi

echo "OS: $os; qmake args: $qmake_args"

# Prepare environment.
if [ $is_linux = true ]; then
  sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
  sudo add-apt-repository ppa:beineri/opt-qt-5.15.2-xenial -y

  sudo apt-get update
  sudo apt-get -y install gcc-7 g++-7 qt515tools qt515base qt515webengine
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