#!/bin/sh

git fetch --tags

if test "$TRAVIS_OS_NAME" = "osx"; then
  # Mac OS X.
  pip3 install aqtinstall
  
  # Install Qt.
  QTPATH="$(pwd)/Qt"
  QTVERSION="5.15.2"
  QTBIN="$QTPATH/$QTVERSION/clang_64/bin"

  echo "Qt bin directory is: $QTBIN"
  echo "Qt will be installed to: $QTPATH"

  aqt install -O "$QTPATH" 5.15.2 mac desktop clang_64 -m qtwebengine

  export QT_PLUGIN_PATH="$QTPATH/$QTVERSION/clang_64/plugins"
  export PATH="$QTBIN:$PATH"

  qmake --version
elif test "$TRAVIS_OS_NAME" = "linux"; then
  # Linux.
  sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
  sudo add-apt-repository ppa:beineri/opt-qt-5.14.2-xenial -y

  sudo apt-get update
  sudo apt-get -y install gcc-7 g++-7 qt514tools qt514base qt514webengine qt514svg
  sudo apt-get -y install openssl libssl-dev libgl1-mesa-dev 

  sudo update-alternatives --remove-all gcc 
  sudo update-alternatives --remove-all g++
  sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 50
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 50
fi