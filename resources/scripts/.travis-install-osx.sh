#!/bin/sh

mkdir rssguard-build && cd rssguard-build
qmake ..
make
make install
macdeployqt rssguard.app -dmg
mv rssguard.dmg rssguard-osx.dmg

ls -lha

git clone -q --depth=1 --branch=build-artifacts https://martinrotter:${GH_TOKEN}@github.com/martinrotter/rssguard.git ./build-artifacts
curl --upload-file ./rssguard-osx.dmg https://transfer.sh/rssguard-osx.dmg --silent >> ./build-artifacts/macosx-builds.txt
cat ./build-artifacts/macosx-builds.txt

cd ./build-artifacts
git add *.txt
git commit -m "New files."
git push origin build-artifacts