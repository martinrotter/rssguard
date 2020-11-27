#!/bin/sh

# Install Qt. This needs to be done here, because some
# variables need to be set.
QTPATH="$(pwd)/Qt"
QTVERSION="5.15.2"
QTBIN="$QTPATH/$QTVERSION/clang_64/bin"

echo "Qt bin directory is: $QTBIN"
echo "Qt will be installed to: $QTPATH"

aqt install -O "$QTPATH" 5.15.2 mac desktop clang_64 -m qtwebengine

export QT_PLUGIN_PATH="$QTPATH/$QTVERSION/clang_64/plugins"
export PATH="$QTBIN:$PATH"

qmake --version

# Build application.
mkdir rssguard-build && cd rssguard-build
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
make
make install
cd "src/rssguard"

# Fix .dylib linking.
install_name_tool -change "librssguard.dylib" "@executable_path/librssguard.dylib" "RSS Guard.app/Contents/MacOS/rssguard"
install_name_tool -change "librssguard.dylib" "@executable_path/librssguard.dylib" "rssguard"

otool -L "RSS Guard.app/Contents/MacOS/rssguard"
macdeployqt "./RSS Guard.app" -dmg
#make dmg

# Rename DMG.
set -- *.dmg

rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi

dmgname="$1"
git_tag=$(git describe --tags `git rev-list --tags --max-count=1`)
git_revision=$(git rev-parse --short HEAD)

if [ "$USE_WEBENGINE" = true ]; then
  dmgnewname="rssguard-${git_tag}-${git_revision}-mac64.dmg"
else
  dmgnewname="rssguard-${git_tag}-${git_revision}-nowebengine-mac64.dmg"
fi

mv "$dmgname" "$dmgnewname"
ls