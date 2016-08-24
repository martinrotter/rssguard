#!/bin/sh

mkdir rssguard-build && cd rssguard-build
qmake ..
make
make install
macdeployqt rssguard.app -dmg