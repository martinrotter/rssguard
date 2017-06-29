#!/bin/sh

mkdir rssguard-build && cd rssguard-build
qmake ..
make
make install
make dmg

ls -lha
otool -L "RSS Guard.app/Contents/MacOS/rssguard"

cd "RSS Guard.app"
ls -lha

cd ..

git config --global user.email "rotter.martinos@gmail.com"
git config --global user.name "martinrotter"
git clone -q --depth=1 https://martinrotter:${GH_TOKEN}@github.com/martinrotter/rssguard.wiki.git ./build-wiki

set -- *.dmg
dmgname="$1"
dmgnamenospace="${dmgname// /-}"
echo "DMGNAME IS: $dmgname"
echo "DMGNAME NO SPACE IS: $dmgnamenospace"

curl --upload-file "./$dmgname" "https://transfer.sh/$dmgnamenospace" --silent >> ./build-wiki/Mac-OS-X-development-builds.md
echo >> ./build-wiki/Mac-OS-X-development-builds.md
cat ./build-wiki/Mac-OS-X-development-builds.md

cd ./build-wiki
git add *.*
git commit -m "New files."
git push origin master