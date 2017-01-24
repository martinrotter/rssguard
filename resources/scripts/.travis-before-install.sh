#!/bin/sh

if test "$TRAVIS_OS_NAME" = "osx"; then
  # Mac OS X.
  brew update
  brew install qt5
  brew link --force qt5
  brew install curl
  brew link --force curl
else
  # Linux.
  sudo add-apt-repository ppa:beineri/opt-qt58-trusty -y
  sudo apt-get update
  sudo apt-get -y install qt58tools qt58base qt58webengine
fi