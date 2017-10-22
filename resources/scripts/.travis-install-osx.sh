#!/bin/sh

# Build application.
mkdir rssguard-build && cd rssguard-build
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
make
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
make install

# Make DMG image.
make dmg
otool -L "RSS Guard.app/Contents/MacOS/rssguard"

set -- *.dmg
dmgname="$1"

if [ "$USE_WEBENGINE" = true ]; then
  dmgnamenospace="rssguard-$(git rev-parse --short HEAD)-mac64.dmg"
else
  dmgnamenospace="rssguard-$(git rev-parse --short HEAD)-nowebengine-mac64.dmg"
fi

mv "$dmgname" "$dmgnamenospace"
dmgname="$dmgnamenospace"

echo "File to upload: $dmgname"
echo "URL ending: $dmgnamenospace"

git config --global user.email "rotter.martinos@gmail.com"
git config --global user.name "martinrotter"
git clone -q --depth=1 https://martinrotter:${GH_TOKEN}@github.com/martinrotter/rssguard.wiki.git ./build-wiki

curl --upload-file "./$dmgname" "https://transfer.sh/$dmgnamenospace" --silent >> ./build-wiki/Mac-OS-X-development-builds.md
echo "\n" >> ./build-wiki/Mac-OS-X-development-builds.md

cd ./build-wiki
git add *.*
git commit -m "New files."
git pull origin master
git push origin master