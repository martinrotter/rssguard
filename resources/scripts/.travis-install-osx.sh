#!/bin/sh

mkdir rssguard-build && cd rssguard-build
qmake ..
make
make install
macdeployqt rssguard.app -dmg
mv rssguard.dmg rssguard-osx.dmg

ls -lha

git config --global user.email "rotter.martinos@gmail.com"
git config --global user.name "martinrotter"
git clone -q --depth=1 https://martinrotter:${GH_TOKEN}@github.com/martinrotter/rssguard.wiki.git ./build-wiki
curl --upload-file ./rssguard-osx.dmg https://transfer.sh/rssguard-osx.dmg --silent >> ./build-wiki/Mac-OS-X-development-builds.md
echo >> ./build-wiki/Mac-OS-X-development-builds.md
cat ./build-wiki/Mac-OS-X-development-builds.md

cd ./build-wiki
git add *.*
git commit -m "New files."
git push origin master