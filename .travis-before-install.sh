#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
  # Mac OS X.
  brew update
  brew install https://raw.githubusercontent.com/LRFLEW/homebrew-core/981fa2e8f824b068077e7df47f81bdb8d93a8ea1/Formula/qt5.rb
else
  # Linux.
  sudo apt-get update
  sudo apt-get -y install qt57tools qt57base qt57webengine
fi