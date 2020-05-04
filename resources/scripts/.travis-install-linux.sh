#!/bin/bash

ls -lha

# Setup Qt build environment.
source /opt/qt514/bin/qt514-env.sh
mkdir rssguard-build && cd rssguard-build

# Build application.
#lrelease -compress ../rssguard.pro
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
make
make install
cd src/rssguard

# Obtain linuxdeployqt.
wget -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage 

# Create AppImage.
unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -bundle-non-qt-libs -no-translations
./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -appimage -no-translations

# Rename AppImaage.
set -- R*.AppImage

rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi

imagename="$1"
git_tag=$(git describe --abbrev=0)
git_revision=$(git rev-parse --short HEAD)

if [ "$USE_WEBENGINE" = true ]; then
  imagenewname="rssguard-${git_tag}-${git_revision}-linux64.AppImage"
else
  imagenewname="rssguard-${git_tag}-${git_revision}-nowebengine-linux64.AppImage"
fi

mv "$imagename" "$imagenewname"

ls