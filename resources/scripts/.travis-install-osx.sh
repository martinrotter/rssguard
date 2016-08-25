#!/bin/sh

mkdir rssguard-build && cd rssguard-build
qmake ..
make
make install
macdeployqt rssguard.app -dmg
ls -lha
curl --upload-file ./*.dmg https://transfer.sh/rssguard.dmg