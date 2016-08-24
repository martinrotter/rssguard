#!/bin/sh

if test "$TRAVIS_OS_NAME" = "osx"; then
  # Mac OS X.
  brew update
  brew install qt5
else
  # Linux.
  sudo add-apt-repository ppa:beineri/opt-qt57-trusty -y
  sudo apt-get update
  sudo apt-get -y install qt57tools qt57base qt57webengine
  . /opt/qt57/bin/qt57-env.sh
fi