#!/bin/bash

# Setup Qt build environment.
source /opt/qt59/bin/qt59-env.sh
mkdir rssguard-build && cd rssguard-build

# Build application.
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
make
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
make install

# Obtain linuxdeployqt.
wget -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage 

# Create AppImage.
unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/rssguard.desktop" -bundle-non-qt-libs -no-translations
./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/rssguard.desktop" -appimage -no-translations

# Upload image.
git config --global user.email "rotter.martinos@gmail.com"
git config --global user.name "martinrotter"
git clone https://martinrotter:${GH_TOKEN}@github.com/martinrotter/rssguard.wiki.git ./build-wiki

set -- R*.AppImage
imagename="$1"

if [ "$USE_WEBENGINE" = true ]; then
  imagenamenospace="rssguard-$(git rev-parse --short HEAD)-linux.AppImage"
else
  imagenamenospace="rssguard-$(git rev-parse --short HEAD)-nowebengine-linux.AppImage"
fi

echo "File to upload: $imagename"
echo "URL ending: $imagenamenospace"

curl --upload-file "./$imagename" "https://transfer.sh/$imagenamenospace" --silent >> ./build-wiki/Linux-development-builds.md
echo "\n" >> ./build-wiki/Linux-development-builds.md

cd ./build-wiki
git commit -a -m "New files."
git pull origin master
git push origin master