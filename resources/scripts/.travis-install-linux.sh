#!/bin/bash

source /opt/qt59/bin/qt59-env.sh
mkdir rssguard-build && cd rssguard-build
qmake .. "$qmake_args"
make lrelease
make
make install

ls "./usr/bin"
curl -o "linuxdeployqt" 'https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage'
chmod a+x "linuxdeployqt"
unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
linuxdeployqt "./usr/bin/rssguard" -appimage
ls