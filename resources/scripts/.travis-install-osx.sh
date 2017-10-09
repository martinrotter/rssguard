#!/bin/sh

mkdir rssguard-build && cd rssguard-build
qmake ..
make
make install

rm -rfv "RSS Guard.app/Contents/Frameworks"

ls "RSS Guard.app/Contents"

make zip
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
echo "\n" >> ./build-wiki/Mac-OS-X-development-builds.md

set -- *.zip
zipname="$1"
zipnamenospace="${zipname// /-}"
echo "ZIPNAME IS: $zipname"
echo "ZIPNAME NO SPACE IS: $zipnamenospace"

curl --upload-file "./$zipname" "https://transfer.sh/$zipnamenospace" --silent >> ./build-wiki/Mac-OS-X-development-builds.md
echo "\n" >> ./build-wiki/Mac-OS-X-development-builds.md

cat ./build-wiki/Mac-OS-X-development-builds.md

cd ./build-wiki
git add *.*
git commit -m "New files."
git pull origin master
git push origin master