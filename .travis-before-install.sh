#!/bin/sh

if test "$TRAVIS_OS_NAME" = 'osx'; then
  # Mac OS X.
  brew update
  brew install qt5
else
  # Linux.
  apt-get update
  apt-get -y install qt57tools qt57base qt57webengine
fi