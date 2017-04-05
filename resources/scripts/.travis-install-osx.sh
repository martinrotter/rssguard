#!/bin/sh

mkdir rssguard-build && cd rssguard-build
qmake ..
make
make install
make zip

ls -lha
otool -L rssguard.app/Contents/MacOS/rssguard

cd rssguard.app
ls -lha

git config --global user.email "rotter.martinos@gmail.com"
git config --global user.name "martinrotter"
git clone -q --depth=1 https://martinrotter:${GH_TOKEN}@github.com/martinrotter/rssguard.wiki.git ./build-wiki

set -- *.zip
dmgname="$1"
echo $dmgname

curl --upload-file ./$dmgname https://transfer.sh/$dmgname --silent >> ./build-wiki/Mac-OS-X-development-builds.md
echo >> ./build-wiki/Mac-OS-X-development-builds.md
cat ./build-wiki/Mac-OS-X-development-builds.md

cd ./build-wiki
git add *.*
git commit -m "New files."
git push origin master