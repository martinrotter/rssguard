#!/bin/sh

git fetch --tags

if test "$TRAVIS_OS_NAME" = "osx"; then
  # Mac OS X.
  brew update
#  brew install p7zip
#  brew link --force p7zip
  brew install curl
  brew link --force curl
  brew uninstall gnu-sed
  brew install gnu-sed
  brew link --force gnu-sed

  pip3 install aqtinstall
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
else
  git submodule update --init --recursive --remote
  qt_version="5.15.1"
  qt_stub="qt-$qt_version-dynamic-msvc2019-x86_64"
  qt_link="https://github.com/martinrotter/qt5-minimalistic-builds/releases/download/$qt_version/$qt_stub.7z"
  qt_output="qt.7z"
  
  curl -L $qt_link --output $qt_output
  "./resources/scripts/7za/7za.exe" x $qt_output
  qt_bin=$(dirname $(readlink -f $(find ./ -name "qmake.exe")))
  export PATH="$qt_bin:$PATH"
fi