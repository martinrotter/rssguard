#################################################################
#
# For license of this file, see <project-root-folder>/LICENSE.md.
#
# This is RSS Guard compilation script for qmake.
#
# Usage (out of source build type, we have two side by side folders:
# empty "build-dir" and RSS Guard repository "rssguard-dir"):
#   a) DEBUG build for testing.
#     cd build-dir
#     qmake ../rssguard-dir/build.pro -r CONFIG+=debug PREFIX=./usr
#     make
#     make install
#
#   b) RELEASE build for production use.
#     cd build-dir
#     qmake ../rssguard-dir/build.pro -r CONFIG+=release PREFIX=./usr
#     make
#     make install
#
# Variables:
#   USE_WEBENGINE - if specified, then QtWebEngine module for internal web browser is used.
#                   Otherwise simple text component is used and some features will be disabled.
#                   Default value is "false". If QtWebEngine is installed during compilation, then
#                   value of this variable is tweaked automatically.
#   PREFIX - specifies base folder to which files are copied during "make install"
#            step, defaults to "$$OUT_PWD/usr" on Linux and to "$$OUT_PWD/app" on Windows. Behavior
#            of this variable can be mimicked with $INSTALL_ROOT variable on Linux. Note that
#            RSS Guard's installation is automatically relocatable, in other words, no
#            absolute OS-dependent paths are used.
#
# Other information:
#   - supports Windows, Linux, Mac OS X, Android,
#   - Qt 5.9.0 or higher is required,
#   - if you wish to make packages for Windows, then you must initialize all submodules within repository before compilation,
#   - C++ 11/17 is required.
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
