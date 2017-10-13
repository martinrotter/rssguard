#!/bin/bash

source /opt/qt59/bin/qt59-env.sh
mkdir rssguard-build && cd rssguard-build
qmake .. "$qmake_args"
make lrelease
make
make install

ls "./usr/bin"
curl -o "linuxdeployqt" 'https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage'
chmod +x "linuxdeployqt"
linuxdeployqt "./usr/bin/rssguard" -appimage
ls