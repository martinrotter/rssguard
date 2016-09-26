#!/bin/sh

mkdir rssguard-build && cd rssguard-build
qmake ..
make
make dmg

ls -lha

otool -L rssguard.app/Contents/MacOS/rssguard

git config --global user.email "rotter.martinos@gmail.com"
git config --global user.name "martinrotter"
git clone -q --depth=1 https://martinrotter:${GH_TOKEN}@github.com/martinrotter/rssguard.wiki.git ./build-wiki

dmgname=*-osx.dmg

curl --upload-file ./$dmgname https://transfer.sh/$dmgname --silent >> ./build-wiki/Mac-OS-X-development-builds.md
echo >> ./build-wiki/Mac-OS-X-development-builds.md
cat ./build-wiki/Mac-OS-X-development-builds.md

cd ./build-wiki
git add *.*
git commit -m "New files."
git push origin master