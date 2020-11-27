#!/bin/sh

git fetch --tags

if test "$TRAVIS_OS_NAME" = "osx"; then
  # Mac OS X.
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
fi