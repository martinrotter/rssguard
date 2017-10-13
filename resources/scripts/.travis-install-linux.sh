#!/bin/bash

source /opt/qt59/bin/qt59-env.sh
mkdir rssguard-build && cd rssguard-build
qmake .. "$qmake_args"
make lrelease
make
make install

ls "./usr/bin"
wget -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage 

unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
./linuxdeployqt-continuous-x86_64.AppImage "./usr/bin/rssguard" -appimage
ls