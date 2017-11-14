#!/bin/sh

if test "$TRAVIS_OS_NAME" = "osx"; then
  # Mac OS X.
  brew update
  brew install p7zip
  brew link --force p7zip
  brew install qt5
  brew link --force qt5
  brew install curl
  brew link --force curl
  #brew uninstall gnu-sed
  #brew install gnu-sed --with-default-names
  #brew link --force gnu-sed
else
  # Linux.
  sudo add-apt-repository ppa:beineri/opt-qt591-trusty -y
  sudo apt-get update
  sudo apt-get -y install qt59tools qt59base qt59webengine
fi