#!/bin/sh

# Build application.
ls
mkdir rssguard-build && cd rssguard-build
lrelease -compress ../rssguard.pro
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
make
make install

# Make DMG image.
cd "src/rssguard"
make dmg
otool -L "RSS Guard.app/Contents/MacOS/rssguard"