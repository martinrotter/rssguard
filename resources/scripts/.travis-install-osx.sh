#!/bin/sh

# Build application.
ls
mkdir rssguard-build && cd rssguard-build
lrelease -compress ../rssguard.pro
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
make
make install

# Make DMG image.
make dmg
otool -L "RSS Guard.app/Contents/MacOS/rssguard"

set -- *.dmg
dmgname="$1"
git_revision=$(git rev-parse --short HEAD)

if [ "$USE_WEBENGINE" = true ]; then
  dmgnamenospace="rssguard-${git_revision}-mac64.dmg"
else
  dmgnamenospace="rssguard-${git_revision}-nowebengine-mac64.dmg"
fi

mv "$dmgname" "$dmgnamenospace"
dmgname="$dmgnamenospace"

echo "File to upload: $dmgname"
echo "URL ending: $dmgnamenospace"

git config --global user.email "rotter.martinos@gmail.com"
git config --global user.name "martinrotter"
git clone -q --depth=1 https://martinrotter:${GH_TOKEN}@github.com/martinrotter/rssguard.wiki.git ./build-wiki

url=$(curl --upload-file "./$dmgname" "https://transfer.sh/$dmgnamenospace" --silent)

wikiline="| Mac OS | $(date +'%m-%d-%Y %T') | [$git_revision](https\://github.com/martinrotter/rssguard/commit/$git_revision) | [transfer.sh]($url) | $(echo "$USE_WEBENGINE") |  "
wikifile="./build-wiki/Development-builds.md"
wikifilenew="./build-wiki/Development-builds.md.new"

echo "Line to add: $wikiline"
cat "$wikifile" | sed -e "s@| Mac OS | .\+$USE_WEBENGINE |  @$wikiline@g" > "$wikifilenew"

cat "$wikifilenew"
#mv "$wikifilenew" "$wikifile"

cd ./build-wiki
git add *.*
git commit -m "New files."
git pull origin master
git push origin master