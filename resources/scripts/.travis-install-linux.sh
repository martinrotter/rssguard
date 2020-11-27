#!/bin/bash

source /opt/qt514/bin/qt514-env.sh

# Build application.
mkdir rssguard-build && cd rssguard-build
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
make
make install
cd "src/rssguard"

# Obtain linuxdeployqt.
wget -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage 

# Create AppImage.
unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -bundle-non-qt-libs -no-translations

if [ "$USE_WEBENGINE" = true ]; then
  # Copy some NSS3 files to prevent WebEngine crashes.
  cp /usr/lib/x86_64-linux-gnu/nss/* ./AppDir/usr/lib/ -v
fi

./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -appimage -no-translations

# Rename AppImaage.
set -- R*.AppImage

rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi

imagename="$1"
git_tag=$(git describe --tags `git rev-list --tags --max-count=1`)
git_revision=$(git rev-parse --short HEAD)

if [ "$USE_WEBENGINE" = true ]; then
  imagenewname="rssguard-${git_tag}-${git_revision}-linux64.AppImage"
else
  imagenewname="rssguard-${git_tag}-${git_revision}-nowebengine-linux64.AppImage"
fi

mv "$imagename" "$imagenewname"
ls