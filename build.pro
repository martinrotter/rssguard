#################################################################
#
# For license of this file, see <project-root-folder>/LICENSE.md.
#
# This is RSS Guard compilation script for qmake.
#
# Usage:
#   a) DEBUG build for testing. (out of source build type)
#     cd ../build-dir
#     qmake ../rssguard-dir/rssguard.pro -r CONFIG+=debug PREFIX=./usr
#     make
#     make install
#
#   b) RELEASE build for production use. (out of source build type)
#     cd ../build-dir
#     qmake ../rssguard-dir/rssguard.pro -r CONFIG+=release PREFIX=./usr
#     make
#     make install
#
# Variables:
#   USE_WEBENGINE - if specified, then QtWebEngine module for internal web browser is used.
#                   Otherwise simple text component is used and some features will be disabled.
#                   Default value is "false". If QtWebEngine is installed during compilation, then
#                   value of this variable is tweaked automatically.
#   PREFIX - specifies base folder to which files are copied during "make install"
#            step, defaults to "$$OUT_PWD/usr" on Linux and to "$$OUT_PWD/app" on Windows.
#
# Other information:
#   - supports Windows, Linux, Mac OS X, Android,
#   - Qt 5.9.0 or higher is required,
#   - C++ 11 is required.
#
# Authors and contributors:
#   - Martin Rotter (project leader).
#
#################################################################

TEMPLATE = subdirs

CONFIG += ordered
SUBDIRS = librssguard rssguard

librssguard.subdir  = src/librssguard

rssguard.subdir  = src/rssguard
rssguard.depends = libtextosaurus
