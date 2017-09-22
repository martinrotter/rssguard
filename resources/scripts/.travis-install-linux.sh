#!/bin/bash

source /opt/qt59/bin/qt59-env.sh
mkdir rssguard-build && cd rssguard-build
qmake ..
make
make install