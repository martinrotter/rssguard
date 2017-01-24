#!/bin/bash

source /opt/qt58/bin/qt58-env.sh
mkdir rssguard-build && cd rssguard-build
qmake ..
make
make install