#!/bin/sh

mkdir rssguard-build && cd rssguard-build
qmake ..
make
make install
macdeployqt rssguard-osx.app -dmg
ls -lha

git clone -q --depth=1 --branch=build-artifacts https://github.com/martinrotter/rssguard.git
git config --global credential.helper "store --file=github_credentials"
echo "https://${GH_TOKEN}:@github.com" > github_credentials
git config --global user.email "rotter.martinos@gmail.com"
git config --global user.name "martinrotter"

curl --upload-file ./*.dmg https://transfer.sh/rssguard.dmg --silent >> ./build-artifacts/macosx-builds.txt

cat ./build-artifacts/macosx-builds.txt

cd ./rssguard-artifacts
git add *.*
git commit -m "New files."
git push origin build-artifacts