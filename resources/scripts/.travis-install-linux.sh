#!/bin/bash

source /opt/qt57/bin/qt57-env.sh
mkdir rssguard-build && cd rssguard-build
qmake ..
make
make install