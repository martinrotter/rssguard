#!/bin/bash

ls -lha

# Setup Qt build environment.
source /opt/qt512/bin/qt512-env.sh
mkdir rssguard-build && cd rssguard-build

# Build application.
#lrelease -compress ../rssguard.pro
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
make
make install

# Obtain linuxdeployqt.
wget -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage 

ls -lha

ls -lha "./AppDir/usr/share/applications/"

# Create AppImage.
unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -bundle-non-qt-libs -no-translations
./linuxdeployqt-continuous-x86_64.AppImage "./AppDir/usr/share/applications/com.github.rssguard.desktop" -appimage -no-translations

# Upload image.
git config --global user.email "rotter.martinos@gmail.com"
git config --global user.name "martinrotter"
git clone https://martinrotter:${GH_TOKEN}@github.com/martinrotter/rssguard.wiki.git ./build-wiki

set -- R*.AppImage

rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi

imagename="$1"
git_revision=$(git rev-parse --short HEAD)

if [ "$USE_WEBENGINE" = true ]; then
  imagenamenospace="rssguard-${git_revision}-linux64.AppImage"
else
  imagenamenospace="rssguard-${git_revision}-nowebengine-linux64.AppImage"
fi

mv "$imagename" "$imagenamenospace"
imagename="$imagenamenospace"

ls

echo "File to upload: $imagename"
echo "URL ending: $imagenamenospace"

url=$(curl --upload-file "./$imagename" "https://transfer.sh/$imagenamenospace" --silent)

rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi

wikiline="| Linux | $(date +'%m-%d-%Y %T') | [$git_revision](https\://github.com/martinrotter/rssguard/commit/$git_revision) | [transfer.sh]($url) | $(echo "$USE_WEBENGINE") |  "
wikifile="./build-wiki/Development-builds.md"
wikifilenew="./build-wiki/Development-builds.md.new"

echo "Line to add: $wikiline"
cat "$wikifile" | sed -e "s@| Linux | .\+$USE_WEBENGINE |  @$wikiline@g" > "$wikifilenew"

cat "$wikifilenew"
mv "$wikifilenew" "$wikifile"

ls
cd ./build-wiki
git commit -a -m "New files."
git pull origin master
git push origin master

rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi

echo "Current git commit hash is $(git rev-parse HEAD)."
echo "Travis branch $TRAVIS_BRANCH and Travis tag $TRAVIS_TAG."

if [[ $TRAVIS_BRANCH == $TRAVIS_TAG ]]; then
#if [[ true ]]; then
  # We will trigger stuff for Flathub.
  cd ../..
  chmod +x resources/scripts/.flathub-release.sh
  resources/scripts/.flathub-release.sh $TRAVIS_TAG $(git rev-parse HEAD)
fi