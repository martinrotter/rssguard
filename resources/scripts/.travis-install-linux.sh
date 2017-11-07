#!/bin/bash

# Setup Qt build environment.
source /opt/qt59/bin/qt59-env.sh
mkdir rssguard-build && cd rssguard-build

# Build application.
lrelease -compress ../rssguard.pro
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
make
make install

# Obtain linuxdeployqt.
wget -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage 

# Create AppImage.
unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -bundle-non-qt-libs -no-translations
./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -appimage -no-translations

# Upload image.
git config --global user.email "rotter.martinos@gmail.com"
git config --global user.name "martinrotter"
git clone https://martinrotter:${GH_TOKEN}@github.com/martinrotter/rssguard.wiki.git ./build-wiki

set -- R*.AppImage
imagename="$1"
git_revision=$(git rev-parse --short HEAD)

if [ "$USE_WEBENGINE" = true ]; then
  imagenamenospace="rssguard-${git_revision}-linux64.AppImage"
else
  imagenamenospace="rssguard-${git_revision}-nowebengine-linux64.AppImage"
fi

mv "$imagename" "$imagenamenospace"
imagename="$imagenamenospace"

echo "File to upload: $imagename"
echo "URL ending: $imagenamenospace"

url=$(curl --upload-file "./$imagename" "https://transfer.sh/$imagenamenospace" --silent)
echo "| $(date +'%m-%d-%Y %T') | [$git_revision](https://github.com/martinrotter/rssguard/commit/$git_revision) | [transfer.sh]($url) | $(echo "$USE_WEBENGINE") |  "$'\r' >> ./build-wiki/Linux-development-builds.md

cd ./build-wiki
git commit -a -m "New files."
git pull origin master
git push origin master