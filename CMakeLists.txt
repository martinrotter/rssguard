#################################################################
#
# This file is part of RSS Guard.
#
# Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
#
# RSS Guard is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RSS Guard is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with RSS Guard. If not, see <http:# www.gnu.org/licenses/>.
#
#
#  This is RSS Guard compilation script for CMake.
#
# Usage:
#   a) DEBUG build for testing.
#     cmake -DCMAKE_BUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX=/usr -DUSE_QT_5=ON
#
#   b) RELEASE build for production use.
#     cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_INSTALL_PREFIX=/usr -DUSE_QT_5=ON
#
# Arguments:
#   -DCMAKE_INSTALL_PREFIX=/usr
#     Installation path, replace with something like "C:\rssguard" on Windows.
#
#   -DUSE_QT_5=OFF
#     Specifies which major Qt version to use. Qt 4 and Qt 5 are supported.
#     If "OFF" is passed as an argument, then Qt 4 is used. Default is "OFF".
#
#   -DENABLE_OS2_RC=OFF
#     If "ON" then icon is compiled into executable file on OS/2. Defaults to "OFF".
#
# Refreshing translation sources:
#   make lupdate
#
# Generating source tarballs:
#   make dist or make package_source
#
# Generating binary tarballs:
#   make package
#
# Other information:
#   - supports Windows, Linux, OS/2 (eComStation),
#   - Qt 4.7.3 and higher is required,
#   - resource compiler (windres.exe) is needed on MinGW,
#   - resource compiler (rc.exe) is recommended on OS/2.
#
# Authors and contributors:
#   - Martin Rotter (project leader),
#   - Elbert Pol (huge OS/2-related contributions).
#
#################################################################


# Setup basic variables.
project(rssguard)

set(APP_NAME "RSS Guard")
set(APP_LOW_NAME "rssguard")
set(APP_VERSION "3.0.1")
set(FILE_VERSION "3,0,1,0")
set(APP_AUTHOR "Martin Rotter")
set(APP_URL "http://bitbucket.org/skunkos/rssguard")
set(APP_URL_ISSUES "http://bitbucket.org/skunkos/rssguard/issues")
set(APP_URL_ISSUES_NEW_GITHUB "https://github.com/martinrotter/rssguard/issues/new")
set(APP_URL_ISSUES_NEW_BITBUCKET "http://bitbucket.org/skunkos/rssguard/issues/new")
set(APP_URL_WIKI "https://bitbucket.org/skunkos/rssguard/wiki/Home")
set(APP_EMAIL "rotter.martinos@gmail.com")
set(MINIMUM_QT_VERSION 4.7.3)
set(EXE_NAME ${APP_LOW_NAME})

if(APPLE)
  set(APP_URL_BACKWARDS "org.bitbucket.${APP_LOW_NAME}")
endif(APPLE)

# Options declaration.
option(USE_QT_5 "Use Qt 5 for building" OFF)
option(ENABLE_NUMIX "Install Numix icon theme" ON)

if(OS2)
  option(ENABLE_OS2_RC "Compile application icon on OS/2" OFF)
endif(OS2)

if(WIN32)
  option(USE_NSIS "Use NSIS generator to produce installer" OFF)
endif(WIN32)

# Set appropriate CMake policies.
if(POLICY CMP0012)
  cmake_policy(SET CMP0012 NEW)
endif()

if(USE_QT_5)
  cmake_minimum_required(VERSION 2.8.10)
else(USE_QT_5)
  cmake_minimum_required(VERSION 2.8.0)
endif(USE_QT_5)

if(${APPLE})
  if(NOT USE_QT_5)
    message(FATAL_ERROR "[${APP_LOW_NAME}] Only Qt 5.x.x is supported on Mac OS X.")
  endif(NOT USE_QT_5)
endif(${APPLE})

message(STATUS "")
message(STATUS "[${APP_LOW_NAME}] ==== Options summary ====")
message(STATUS "[${APP_LOW_NAME}] Use Qt 5 for building -> ${USE_QT_5}")
message(STATUS "[${APP_LOW_NAME}] Install Numix icon theme -> ${ENABLE_NUMIX}")

if(OS2)
  message(STATUS "[${APP_LOW_NAME}] Compile application icon on OS/2 -> ${ENABLE_OS2_RC}")
endif(OS2)

if(WIN32)
  message(STATUS "[${APP_LOW_NAME}] Use NSIS generator to produce installer -> ${USE_NSIS}")
endif(WIN32)

message(STATUS "[${APP_LOW_NAME}] ==== Options summary ====")
message(STATUS "")

message(STATUS "[${APP_LOW_NAME}] Welcome to ${APP_NAME} compilation process.")
message(STATUS "[${APP_LOW_NAME}] Compilation process begins right now.")

# Setup misc options.
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/resources/nsis
  ${CMAKE_MODULE_PATH})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Eliminate a warning when building in Windows that relates
# to static linking of Qt executables to qtmain.lib.
# This policy was introduced in CMake version 2.8.11.
# CMake version 2.8.11.2 warns when the policy is not set
# and uses OLD behavior.
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif(POLICY CMP0020)

# Select which Qt version to use.
if(USE_QT_5)
  message(STATUS "[${APP_LOW_NAME}] Using Qt 5 library for building.")
  
  # Instruct CMake to run moc automatically when needed.
  set(CMAKE_AUTOMOC ON)
else(USE_QT_5)
  message(STATUS "[${APP_LOW_NAME}] Using Qt 4 library for building.")
endif(USE_QT_5)

# Obtain revision number if available.
message(STATUS "[${APP_LOW_NAME}] Obtaining revision number.")

if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
  find_package(Git)

  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_REVISION
    )

    string(REGEX REPLACE "\n" "" GIT_REVISION "${GIT_REVISION}")
    set(APP_REVISION ${GIT_REVISION})
  else(GIT_FOUND)
    # Git was not found, we cannot determine current revision.
    set(APP_REVISION "-")
  endif(GIT_FOUND)
else(EXISTS "${PROJECT_SOURCE_DIR}/.git")
  # Not information for Git are available, this is stripped
  # build environment
  set(APP_REVISION "-")
endif(EXISTS "${PROJECT_SOURCE_DIR}/.git")

message(STATUS "[${APP_LOW_NAME}] Revision number obtained: ${APP_REVISION}.")

# Configure desktop entry for Linux.
if(UNIX)
  message(STATUS "[${APP_LOW_NAME}] Generating desktop entry for Linux.")
  configure_file (
    ${PROJECT_SOURCE_DIR}/resources/desktop/rssguard.desktop.in
    ${CMAKE_BINARY_DIR}/resources/desktop/rssguard.desktop
  )
  set(DESKTOP_ENTRY_PATH ${CMAKE_INSTALL_PREFIX}/share/applications)
endif(UNIX)

# Configure internal C++ defines.
configure_file (
  ${PROJECT_SOURCE_DIR}/src/definitions/definitions.h.in
  ${CMAKE_BINARY_DIR}/src/definitions/definitions.h
)

# Define some useful DEBUG for, ehrm, debug build.
if(CMAKE_BUILD_TYPE STREQUAL "release" OR CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RELEASE")
  message(STATUS "[${APP_LOW_NAME}] A release build (non-debug) is chosen. Debugging outputs are silently ignored.")
  set(CMAKE_BUILD_TYPE MinSizeRel)
  message(STATUS "[${APP_LOW_NAME}] Output executable file is optimized for minimum size.")
else(CMAKE_BUILD_TYPE STREQUAL "release" OR CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RELEASE")
  message(STATUS "[${APP_LOW_NAME}] A debug build is chosen.")
  add_definitions(-DDEBUG)
  set(CMAKE_BUILD_TYPE Debug)

# Enable compiler warnings.
# See http://stackoverflow.com/questions/2368811/how-to-set-warning-level-in-cmake
message(STATUS "[${APP_LOW_NAME}] Enabling verbose makefile and full warning reports.")
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-pedantic -Wall)  # -Wextra -Wno-unused-variable
endif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

# Verbose compiling outputs.
set(CMAKE_VERBOSE_MAKEFILE ON)
endif(CMAKE_BUILD_TYPE STREQUAL "release" OR CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RELEASE")

# Force Qt to use string builders.
add_definitions(-DQT_USE_QSTRINGBUILDER)

# Force Unicode.
add_definitions(-DUNICODE -D_UNICODE)

# Check for C++ 11 features availability.
# See http://stackoverflow.com/questions/10984442/how-to-detect-c11-support-of-a-compiler-with-cmake
if(USE_QT_5)
  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION
    )

    if(${GCC_VERSION} VERSION_GREATER 4.7 OR ${GCC_VERSION} VERSION_EQUAL 4.7)
      add_definitions(-std=c++11)
    else(${GCC_VERSION} VERSION_GREATER 4.7 OR ${GCC_VERSION} VERSION_EQUAL 4.7)
      message(FATAL_ERROR "[${APP_LOW_NAME}] Your C++ compiler does not support C++ 11.")
    endif(${GCC_VERSION} VERSION_GREATER 4.7 OR ${GCC_VERSION} VERSION_EQUAL 4.7)

  elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    add_definitions(-std=c++11 -stdlib=libc++)
  elseif(${MSVC_VERSION} VERSION_LESS 1600)
    message(FATAL_ERROR "[${APP_LOW_NAME}] Your C++ compiler does not support C++ 11.")
  endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
endif(USE_QT_5)

# Setup libraries.
if(UNIX)
  # On Unices, enable D-Bus support.
  if(USE_QT_5)
    find_package(Qt5 REQUIRED DBus Sql WebKit WebKitWidgets Widgets Xml XmlPatterns Network LinguistTools PrintSupport)
  else(USE_QT_5)
    set(QT_MIN_VERSION ${MINIMUM_QT_VERSION})
    find_package(Qt4 REQUIRED QtDBus QtCore QtGui QtSql QtNetwork QtWebkit QtXml QtXmlPatterns)
    include(${QT_USE_FILE})
  endif(USE_QT_5)
else(UNIX)
  if(USE_QT_5)
    find_package(Qt5 REQUIRED Sql WebKit WebKitWidgets Widgets Xml XmlPatterns Network LinguistTools PrintSupport)
  else(USE_QT_5)
    set(QT_MIN_VERSION ${MINIMUM_QT_VERSION})
    find_package(Qt4 REQUIRED QtCore QtGui QtSql QtNetwork QtWebkit QtXml QtXmlPatterns)
    include(${QT_USE_FILE})
  endif(USE_QT_5)
endif(UNIX)

# Configure QStringBuilder behavior.
if(USE_QT_5)
  message(STATUS "[${APP_LOW_NAME}] Enabling fast QString concatenation.")

  add_definitions(-DQT_USE_FAST_CONCATENATION)
  add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)
elseif(NOT ${QTVERSION} VERSION_LESS 4.8.0)
  message(STATUS "[${APP_LOW_NAME}] Enabling fast QString concatenation.")

  add_definitions(-DQT_USE_FAST_CONCATENATION)
  add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)
else(NOT ${QTVERSION} VERSION_LESS 4.8.0)
  message(STATUS "[${APP_LOW_NAME}] Disabling fast QString concatenation.")
endif(USE_QT_5)

# Configure executable "properties" for Windows or OS2.
if(WIN32)
  message(STATUS "[${APP_LOW_NAME}] Generating executable file properties for Windows.")
  configure_file (
    ${PROJECT_SOURCE_DIR}/resources/executable_properties/rssguard_win.rc.in
    ${CMAKE_BINARY_DIR}/resources/executable_properties/rssguard_win.rc
  )
elseif(OS2 AND ENABLE_OS2_RC)
  message(STATUS "[${APP_LOW_NAME}] Generating executable file properties for OS2.")
  configure_file (
    ${PROJECT_SOURCE_DIR}/resources/executable_properties/rssguard_os2.rc.in
    ${CMAKE_BINARY_DIR}/resources/executable_properties/rssguard_os2.rc
  )
endif(WIN32)

# Set special linker flag for Windows XP & MSVC++ 2013 support.
if(WIN32 AND MSVC)
  set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS,5.01" )
endif(WIN32 AND MSVC)

# Compile application icon if compiling with MinGW on WIN32 or with OS2.
if(WIN32 AND MINGW)
  enable_language(RC)
  set(CMAKE_RC_COMPILER_INIT windres)
  message(STATUS "[${APP_LOW_NAME}] Icon file binary will be builded for Windows.")
  message(STATUS "[${APP_LOW_NAME}] Used tool is: ${CMAKE_RC_COMPILER}")
  set(CMAKE_RC_COMPILE_OBJECT
      "<CMAKE_RC_COMPILER> -i <SOURCE> -o <OBJECT>")

  set(APP_SOURCES
      ${APP_SOURCES}
      ${CMAKE_BINARY_DIR}/resources/executable_properties/rssguard_win.rc)
elseif(OS2)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Zstack 0x2000 -Zlinker \"DISABLE 1121\" -s -Zlinker /PM:PM -Zno-fork -Zhigh-mem -Zmap")

  if(${ENABLE_OS2_RC})
    enable_language(RC)
    set(CMAKE_RC_COMPILER_INIT rc)
    message(STATUS "[${APP_LOW_NAME}] Icon file binary will be builded for OS2.")
    message(STATUS "[${APP_LOW_NAME}] Used tool is: ${CMAKE_RC_COMPILER}")
    set(CMAKE_RC_COMPILE_OBJECT
        "<CMAKE_RC_COMPILER> -n -r <SOURCE> <OBJECT>")

    set(APP_SOURCES
        ${APP_SOURCES}
        ${CMAKE_BINARY_DIR}/resources/executable_properties/rssguard_os2.rc)
  endif(${ENABLE_OS2_RC})
elseif(WIN32 AND MSVC)
  # MSVC takes care of this automatically - no need to use windres.exe
  # for MSVC compilers.
  set(APP_SOURCES ${APP_SOURCES} ${CMAKE_BINARY_DIR}/resources/executable_properties/rssguard_win.rc)
endif(WIN32 AND MINGW)

# APP source files.
set(APP_SOURCES
  ${APP_SOURCES}

  # QTSINGLEAPPLICATION sources.
  src/qtsingleapplication/qtlocalpeer.cpp
  src/qtsingleapplication/qtsinglecoreapplication.cpp
  src/qtsingleapplication/qtsingleapplication.cpp

  # QT-JSON sources.
  src/qt-json/json.cpp

  # GUI sources.
  src/gui/dialogs/formmain.cpp
  src/gui/dialogs/formsettings.cpp
  src/gui/dialogs/formabout.cpp
  src/gui/dialogs/formupdate.cpp
  src/gui/dialogs/formdatabasecleanup.cpp
  src/gui/dialogs/formbackupdatabasesettings.cpp
  src/gui/dialogs/formrestoredatabasesettings.cpp
  src/gui/dialogs/formaddaccount.cpp
  src/gui/notifications/notification.cpp
  src/gui/systemtrayicon.cpp
  src/gui/baselineedit.cpp
  src/gui/locationlineedit.cpp
  src/gui/tabwidget.cpp
  src/gui/tabbar.cpp
  src/gui/tabcontent.cpp
  src/gui/feedmessageviewer.cpp
  src/gui/feedsview.cpp
  src/gui/messagesview.cpp
  src/gui/statusbar.cpp
  src/gui/plaintoolbutton.cpp
  src/gui/lineeditwithstatus.cpp
  src/gui/widgetwithstatus.cpp
  src/gui/labelwithstatus.cpp
  src/gui/messagebox.cpp
  src/gui/comboboxwithstatus.cpp
  src/gui/basetoolbar.cpp
  src/gui/messagestoolbar.cpp
  src/gui/feedstoolbar.cpp
  src/gui/toolbareditor.cpp
  src/gui/messagessearchlineedit.cpp
  src/gui/styleditemdelegatewithoutfocus.cpp
  src/gui/edittableview.cpp
  src/gui/squeezelabel.cpp
  src/gui/discoverfeedsbutton.cpp
  src/gui/colorlabel.cpp

  # DYNAMIC-SHORTCUTS sources.
  src/dynamic-shortcuts/shortcutcatcher.cpp
  src/dynamic-shortcuts/shortcutbutton.cpp
  src/dynamic-shortcuts/dynamicshortcutswidget.cpp
  src/dynamic-shortcuts/dynamicshortcuts.cpp

  # MISCELLANEOUS sources.
  src/miscellaneous/application.cpp
  src/miscellaneous/debugging.cpp
  src/miscellaneous/settings.cpp
  src/miscellaneous/systemfactory.cpp
  src/miscellaneous/localization.cpp
  src/miscellaneous/textfactory.cpp
  src/miscellaneous/databasefactory.cpp
  src/miscellaneous/skinfactory.cpp
  src/miscellaneous/iconfactory.cpp
  src/miscellaneous/iofactory.cpp
  src/miscellaneous/autosaver.cpp
  src/miscellaneous/mutex.cpp
  src/miscellaneous/databasecleaner.cpp
  src/miscellaneous/simplecrypt/simplecrypt.cpp

  # EXCEPTIONS sources.
  src/exceptions/applicationexception.cpp
  src/exceptions/ioexception.cpp

  # CORE sources.
  src/core/messagesmodel.cpp
  src/core/messagesproxymodel.cpp
  src/core/feedsmodel.cpp
  src/core/feedsproxymodel.cpp
  src/core/parsingfactory.cpp
  src/core/feeddownloader.cpp
  src/core/message.cpp

  # ABSTRACT service sources.
  src/services/abstract/rootitem.cpp
  src/services/abstract/serviceentrypoint.cpp
  src/services/abstract/feed.cpp
  src/services/abstract/category.cpp
  src/services/abstract/serviceroot.cpp
  src/services/abstract/recyclebin.cpp

  # STANDARD feed service sources.
  src/services/standard/gui/formstandardcategorydetails.cpp
  src/services/standard/gui/formstandardfeeddetails.cpp
  src/services/standard/gui/formstandardimportexport.cpp
  src/services/standard/standardfeedsimportexportmodel.cpp
  src/services/standard/standardserviceentrypoint.cpp
  src/services/standard/standardcategory.cpp
  src/services/standard/standardfeed.cpp
  src/services/standard/standardserviceroot.cpp
  src/services/standard/standardrecyclebin.cpp

  # TT-RSS feed service sources.
  src/services/tt-rss/ttrssserviceentrypoint.cpp
  src/services/tt-rss/ttrssserviceroot.cpp
  src/services/tt-rss/ttrssfeed.cpp
  src/services/tt-rss/ttrsscategory.cpp
  src/services/tt-rss/ttrssrecyclebin.cpp
  src/services/tt-rss/gui/formeditaccount.cpp
  src/services/tt-rss/network/ttrssnetworkfactory.cpp

  # NETWORK-WEB sources.
  src/network-web/basenetworkaccessmanager.cpp
  src/network-web/webpage.cpp
  src/network-web/webbrowsernetworkaccessmanager.cpp
  src/network-web/silentnetworkaccessmanager.cpp
  src/network-web/networkfactory.cpp
  src/network-web/webfactory.cpp
  src/network-web/webbrowser.cpp
  src/network-web/webview.cpp
  src/network-web/downloader.cpp
  src/network-web/downloadmanager.cpp
  src/network-web/googlesuggest.cpp

  src/network-web/adblock/adblockaddsubscriptiondialog.cpp
  src/network-web/adblock/adblockblockednetworkreply.cpp
  src/network-web/adblock/adblockdialog.cpp
  src/network-web/adblock/adblockicon.cpp
  src/network-web/adblock/adblockmanager.cpp
  src/network-web/adblock/adblockmatcher.cpp
  src/network-web/adblock/adblockrule.cpp
  src/network-web/adblock/adblocksearchtree.cpp
  src/network-web/adblock/adblocksubscription.cpp
  src/network-web/adblock/adblocktreewidget.cpp

  # MAIN sources.
  src/main.cpp
)

# APP headers.
set(APP_HEADERS
  ${APP_HEADERS}

  # QTSINGLEAPPLICATION headers.
  src/qtsingleapplication/qtlocalpeer.h
  src/qtsingleapplication/qtsinglecoreapplication.h
  src/qtsingleapplication/qtsingleapplication.h

  # GUI headers.
  src/gui/dialogs/formmain.h
  src/gui/dialogs/formsettings.h
  src/gui/dialogs/formabout.h
  src/gui/dialogs/formbackupdatabasesettings.h
  src/gui/dialogs/formrestoredatabasesettings.h
  src/gui/dialogs/formdatabasecleanup.h
  src/gui/dialogs/formupdate.h
  src/gui/dialogs/formaddaccount.h
  src/gui/notifications/notification.h
  src/gui/systemtrayicon.h
  src/gui/baselineedit.h
  src/gui/locationlineedit.h
  src/gui/tabwidget.h
  src/gui/tabbar.h
  src/gui/tabcontent.h
  src/gui/feedmessageviewer.h
  src/gui/feedsview.h
  src/gui/messagesview.h
  src/gui/statusbar.h
  src/gui/plaintoolbutton.h
  src/gui/lineeditwithstatus.h
  src/gui/widgetwithstatus.h
  src/gui/labelwithstatus.h
  src/gui/messagebox.h
  src/gui/comboboxwithstatus.h
  src/gui/basetoolbar.h
  src/gui/messagestoolbar.h
  src/gui/feedstoolbar.h
  src/gui/toolbareditor.h
  src/gui/messagessearchlineedit.h
  src/gui/edittableview.h
  src/gui/squeezelabel.h
  src/gui/discoverfeedsbutton.h
  src/gui/colorlabel.h

  # DYNAMIC-SHORTCUTS headers.
  src/dynamic-shortcuts/dynamicshortcutswidget.h
  src/dynamic-shortcuts/shortcutcatcher.h
  src/dynamic-shortcuts/shortcutbutton.h

  # MISCELLANEOUS headers.
  src/miscellaneous/application.h
  src/miscellaneous/settings.h
  src/miscellaneous/localization.h
  src/miscellaneous/systemfactory.h
  src/miscellaneous/databasefactory.h
  src/miscellaneous/iconfactory.h
  src/miscellaneous/skinfactory.h
  src/miscellaneous/autosaver.h
  src/miscellaneous/mutex.h
  src/miscellaneous/databasecleaner.h

  # CORE headers.
  src/core/messagesmodel.h
  src/core/messagesproxymodel.h
  src/core/feedsmodel.h
  src/core/feedsproxymodel.h
  src/core/feeddownloader.h

  # ABSTRACT service headers.
  src/services/abstract/rootitem.h
  src/services/abstract/feed.h
  src/services/abstract/category.h
  src/services/abstract/serviceroot.h
  src/services/abstract/recyclebin.h

  # STANDARD service headers.
  src/services/standard/standardfeedsimportexportmodel.h
  src/services/standard/gui/formstandardcategorydetails.h
  src/services/standard/gui/formstandardfeeddetails.h
  src/services/standard/gui/formstandardimportexport.h
  src/services/standard/standardcategory.h
  src/services/standard/standardfeed.h
  src/services/standard/standardserviceroot.h
  src/services/standard/standardrecyclebin.h

  # TT-RSS service headers.
  src/services/tt-rss/ttrssserviceroot.h
  src/services/tt-rss/ttrssrecyclebin.h;
  src/services/tt-rss/ttrssfeed.h
  src/services/tt-rss/ttrsscategory.h
  src/services/tt-rss/gui/formeditaccount.h

  # NETWORK-WEB headers.
  src/network-web/webpage.h
  src/network-web/basenetworkaccessmanager.h
  src/network-web/webbrowsernetworkaccessmanager.h
  src/network-web/silentnetworkaccessmanager.h
  src/network-web/webfactory.h
  src/network-web/webbrowser.h
  src/network-web/webview.h
  src/network-web/downloader.h
  src/network-web/downloadmanager.h
  src/network-web/googlesuggest.h

  src/network-web/adblock/adblockaddsubscriptiondialog.h
  src/network-web/adblock/adblockblockednetworkreply.h
  src/network-web/adblock/adblockdialog.h
  src/network-web/adblock/adblockicon.h
  src/network-web/adblock/adblockmanager.h
  src/network-web/adblock/adblockmatcher.h
  src/network-web/adblock/adblocksubscription.h
  src/network-web/adblock/adblocktreewidget.h
)

# APP form files.
set(APP_FORMS
  src/gui/dialogs/formupdate.ui
  src/gui/dialogs/formmain.ui
  src/gui/dialogs/formsettings.ui
  src/gui/dialogs/formabout.ui
  src/gui/dialogs/formbackupdatabasesettings.ui
  src/gui/dialogs/formrestoredatabasesettings.ui
  src/gui/dialogs/formdatabasecleanup.ui
  src/gui/dialogs/formaddaccount.ui
  src/gui/toolbareditor.ui

  # STANDARD service forms.
  src/services/standard/gui/formstandardcategorydetails.ui
  src/services/standard/gui/formstandardfeeddetails.ui
  src/services/standard/gui/formstandardimportexport.ui

  # TT-RSS service forms.
  src/services/tt-rss/gui/formeditaccount.ui

  # NETWORK forms.
  src/network-web/downloadmanager.ui
  src/network-web/downloaditem.ui

  src/network-web/adblock/adblockaddsubscriptiondialog.ui
  src/network-web/adblock/adblockdialog.ui
)

# APP translations.
set(APP_TRANSLATIONS
  localization/rssguard-cs_CZ.ts
  localization/rssguard-da_DK.ts
  localization/rssguard-de_DE.ts
  localization/rssguard-en_GB.ts
  localization/rssguard-en_US.ts
  localization/rssguard-fr_FR.ts
  localization/rssguard-it_IT.ts
  localization/rssguard-nl_NL.ts
  localization/rssguard-sv_SE.ts
  localization/qtbase-cs_CZ.ts
  localization/qtbase-da_DK.ts
  localization/qtbase-de_DE.ts
  localization/qtbase-fr_FR.ts
  localization/qtbase-it_IT.ts
  localization/qtbase-sv_SE.ts
)

set(APP_TRANSLATIONS_WO_QT
  localization/rssguard-cs_CZ.ts
  localization/rssguard-de_DE.ts
  localization/rssguard-en_GB.ts
  localization/rssguard-en_US.ts
  localization/rssguard-fr_FR.ts
  localization/rssguard-it_IT.ts
  localization/rssguard-nl_NL.ts
  localization/rssguard-sv_SE.ts
)

set(APP_TEXT
  resources/text/CHANGELOG
  resources/text/COPYING_GNU_GPL
  resources/text/COPYING_GNU_GPL_HTML
  resources/text/COPYING_BSD
)

if(WIN32)
  set(APP_DLLS_QT4_MSVC2013
    resources/binaries/windows/qt4-msvc2013/libeay32.dll
    resources/binaries/windows/qt4-msvc2013/msvcp120.dll
    resources/binaries/windows/qt4-msvc2013/msvcr120.dll
    resources/binaries/windows/qt4-msvc2013/QtCore4.dll
    resources/binaries/windows/qt4-msvc2013/QtGui4.dll
    resources/binaries/windows/qt4-msvc2013/QtNetwork4.dll
    resources/binaries/windows/qt4-msvc2013/QtSql4.dll
    resources/binaries/windows/qt4-msvc2013/QtWebKit4.dll
    resources/binaries/windows/qt4-msvc2013/QtXml4.dll
    resources/binaries/windows/qt4-msvc2013/ssleay32.dll
    resources/binaries/windows/qt4-msvc2013/libmysql.dll
  )

  set(APP_DLLS_QT4_MSVC2013_IMAGEFORMATS
    resources/binaries/windows/qt4-msvc2013/imageformats/qgif4.dll
    resources/binaries/windows/qt4-msvc2013/imageformats/qico4.dll
    resources/binaries/windows/qt4-msvc2013/imageformats/qjpeg4.dll
    resources/binaries/windows/qt4-msvc2013/imageformats/qmng4.dll
    resources/binaries/windows/qt4-msvc2013/imageformats/qsvg4.dll
    resources/binaries/windows/qt4-msvc2013/imageformats/qtga4.dll
    resources/binaries/windows/qt4-msvc2013/imageformats/qtiff4.dll
  )

  set(APP_DLLS_QT4_MSVC2013_SQLDRIVERS
    resources/binaries/windows/qt4-msvc2013/sqldrivers/qsqlite4.dll
    resources/binaries/windows/qt4-msvc2013/sqldrivers/qsqlmysql4.dll
  )

  set(APP_DLLS_QT5_MSVC2013
    resources/binaries/windows/qt5-msvc2013/icudt55.dll
    resources/binaries/windows/qt5-msvc2013/icuin55.dll
    resources/binaries/windows/qt5-msvc2013/icuuc55.dll
    resources/binaries/windows/qt5-msvc2013/libeay32.dll
    resources/binaries/windows/qt5-msvc2013/libEGL.dll
    resources/binaries/windows/qt5-msvc2013/libGLESv2.dll
    resources/binaries/windows/qt5-msvc2013/libmysql.dll
    resources/binaries/windows/qt5-msvc2013/msvcp120.dll
    resources/binaries/windows/qt5-msvc2013/msvcr120.dll
    resources/binaries/windows/qt5-msvc2013/Qt5Core.dll
    resources/binaries/windows/qt5-msvc2013/Qt5Gui.dll
    resources/binaries/windows/qt5-msvc2013/Qt5Multimedia.dll
    resources/binaries/windows/qt5-msvc2013/Qt5MultimediaWidgets.dll
    resources/binaries/windows/qt5-msvc2013/Qt5Network.dll
    resources/binaries/windows/qt5-msvc2013/Qt5OpenGL.dll
    resources/binaries/windows/qt5-msvc2013/Qt5Positioning.dll
    resources/binaries/windows/qt5-msvc2013/Qt5PrintSupport.dll
    resources/binaries/windows/qt5-msvc2013/Qt5Qml.dll
    resources/binaries/windows/qt5-msvc2013/Qt5Quick.dll
    resources/binaries/windows/qt5-msvc2013/Qt5Sensors.dll
    resources/binaries/windows/qt5-msvc2013/Qt5Sql.dll
    resources/binaries/windows/qt5-msvc2013/Qt5WebChannel.dll
    resources/binaries/windows/qt5-msvc2013/Qt5WebKit.dll
    resources/binaries/windows/qt5-msvc2013/Qt5WebKitWidgets.dll
    resources/binaries/windows/qt5-msvc2013/Qt5Widgets.dll
    resources/binaries/windows/qt5-msvc2013/Qt5Xml.dll
    resources/binaries/windows/qt5-msvc2013/ssleay32.dll
  )

  set(APP_DLLS_QT5_MSVC2013_IMAGEFORMATS
    resources/binaries/windows/qt5-msvc2013/imageformats/qdds.dll
    resources/binaries/windows/qt5-msvc2013/imageformats/qgif.dll
    resources/binaries/windows/qt5-msvc2013/imageformats/qicns.dll
    resources/binaries/windows/qt5-msvc2013/imageformats/qico.dll
    resources/binaries/windows/qt5-msvc2013/imageformats/qjpeg.dll
    resources/binaries/windows/qt5-msvc2013/imageformats/qmng.dll
    resources/binaries/windows/qt5-msvc2013/imageformats/qsvg.dll
    resources/binaries/windows/qt5-msvc2013/imageformats/qtga.dll
    resources/binaries/windows/qt5-msvc2013/imageformats/qtiff.dll
    resources/binaries/windows/qt5-msvc2013/imageformats/qwbmp.dll
  )

  set(APP_DLLS_QT5_MSVC2013_SQLDRIVERS
    resources/binaries/windows/qt5-msvc2013/sqldrivers/qsqlite.dll
    resources/binaries/windows/qt5-msvc2013/sqldrivers/qsqlmysql.dll
  )

  set(APP_DLLS_QT5_MSVC2013_PLATFORMS
    resources/binaries/windows/qt5-msvc2013/platforms/qminimal.dll
    resources/binaries/windows/qt5-msvc2013/platforms/qwindows.dll
  )

  set(APP_DLLS_QT5_MSVC2013_PRINTSUPPORT
    resources/binaries/windows/qt5-msvc2013/printsupport/windowsprintersupport.dll
  )
endif(WIN32)

# Add custom icon on Mac OS X.
if(APPLE)
  SET (APP_SOURCES ${APP_SOURCES} resources/macosx/rssguard.icns)
endif(APPLE)

# Wrap files, create moc files.
if(USE_QT_5)
  qt5_wrap_ui(APP_UI ${APP_FORMS})
  
  # Load translations.
  if(${Qt5LinguistTools_FOUND})
    message(STATUS "[${APP_LOW_NAME}] Qt Linguist Tools found. Translations will get compiled.")
    qt5_add_translation(APP_QM ${APP_TRANSLATIONS})
  else(${Qt5LinguistTools_FOUND})
    message(STATUS "[${APP_LOW_NAME}] Qt Linguist Tools NOT found. No compilation for translations.")
  endif(${Qt5LinguistTools_FOUND})
else(USE_QT_5)
  qt4_wrap_cpp(APP_MOC ${APP_HEADERS})
  qt4_wrap_ui(APP_UI ${APP_FORMS})
  
  # Load translations.
  qt4_add_translation(APP_QM ${APP_TRANSLATIONS})
endif(USE_QT_5)

# Include additional directory paths.
include_directories (
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/gui
  ${CMAKE_SOURCE_DIR}/src/gui/dialogs
  ${CMAKE_SOURCE_DIR}/src/gui/notifications
  ${CMAKE_SOURCE_DIR}/src/network-web
  ${CMAKE_SOURCE_DIR}/src/network-web/adblock
  ${CMAKE_SOURCE_DIR}/src/dynamic-shortcuts
  ${CMAKE_BINARY_DIR}/src
)

# Compile "rssguard"
if(USE_QT_5)
  add_executable(${EXE_NAME} WIN32 MACOSX_BUNDLE
    ${APP_SOURCES}
    ${APP_FORMS}
    ${APP_RCC}
    ${APP_QM}
  )

  if(WIN32)
    target_link_libraries(${EXE_NAME} Qt5::WinMain)
  endif(WIN32)

  if(UNIX)
    # Use modules from Qt.
    qt5_use_modules(${EXE_NAME}
      DBus
      Core
      Widgets
      Sql
      Network
      Xml
      WebKit
      WebKitWidgets
      PrintSupport
    )
  else(UNIX)
  # Use modules from Qt.
    qt5_use_modules(${EXE_NAME}
      Core
      Widgets
      Sql
      Network
      Xml
      WebKit
      WebKitWidgets
      PrintSupport
    )
  endif(UNIX)
# Setup compilation for Qt 4.
else(USE_QT_5)
  add_executable(${EXE_NAME} WIN32 MACOSX_BUNDLE
    ${APP_SOURCES}
    ${APP_FORMS}
    ${APP_MOC}
    ${APP_RCC}
    ${APP_QM}
  )

  if(UNIX)
    # Link modules from Qt.
    target_link_libraries(${EXE_NAME}
      ${QT_QTDBUS_LIBRARY}
      ${QT_QTCORE_LIBRARY}
      ${QT_QTGUI_LIBRARY}
      ${QT_QTNETWORK_LIBRARY}
      ${QT_QTSQL_LIBRARY}
      ${QT_QTXML_LIBRARY}
      ${QT_QTMAIN_LIBRARY}
      ${QT_QTWEBKIT_LIBRARY}
    )
  else(UNIX)
    # Link modules from Qt.
    target_link_libraries(${EXE_NAME}
      ${QT_QTCORE_LIBRARY}
      ${QT_QTGUI_LIBRARY}
      ${QT_QTNETWORK_LIBRARY}
      ${QT_QTSQL_LIBRARY}
      ${QT_QTXML_LIBRARY}
      ${QT_QTMAIN_LIBRARY}
      ${QT_QTWEBKIT_LIBRARY}
    )
  endif(UNIX)
endif(USE_QT_5)

# Installation stage.
if(WIN32 OR OS2)
  message(STATUS "[${APP_LOW_NAME}] You will probably install on Windows or OS/2.")
  install(TARGETS ${EXE_NAME} RUNTIME DESTINATION ./)

  # Copy DLLs and other binary files for main installation.
  if(WIN32 AND USE_QT_5)
    install(FILES ${APP_DLLS_QT5_MSVC2013} DESTINATION ./)
    install(FILES ${APP_DLLS_QT5_MSVC2013_IMAGEFORMATS} DESTINATION ./imageformats)
    install(FILES ${APP_DLLS_QT5_MSVC2013_SQLDRIVERS} DESTINATION ./sqldrivers)
    install(FILES ${APP_DLLS_QT5_MSVC2013_PLATFORMS} DESTINATION ./platforms)
    install(FILES ${APP_DLLS_QT5_MSVC2013_PRINTSUPPORT} DESTINATION ./printsupport)
  endif(WIN32 AND USE_QT_5)

  if(WIN32 AND NOT USE_QT_5)
    install(FILES ${APP_DLLS_QT4_MSVC2013} DESTINATION ./)
    install(FILES ${APP_DLLS_QT4_MSVC2013_IMAGEFORMATS} DESTINATION ./imageformats)
    install(FILES ${APP_DLLS_QT4_MSVC2013_SQLDRIVERS} DESTINATION ./sqldrivers)
  endif(WIN32 AND NOT USE_QT_5)

  install(DIRECTORY resources/graphics/icons/mini-kfaenza DESTINATION ./icons)

  if(ENABLE_NUMIX)
    install(DIRECTORY resources/graphics/icons/numix DESTINATION ./icons)
  endif(ENABLE_NUMIX)

  install(DIRECTORY resources/skins/base
          DESTINATION ./skins)
  install(DIRECTORY resources/misc
          DESTINATION ./)
  install(DIRECTORY resources/initial_feeds
          DESTINATION ./)
  install(FILES resources/graphics/${APP_LOW_NAME}_128.png
          DESTINATION ./
          RENAME ${APP_LOW_NAME}.png)
  install(FILES resources/graphics/${APP_LOW_NAME}_plain_128.png
          DESTINATION ./
          RENAME ${APP_LOW_NAME}_plain.png)
  install(FILES resources/graphics/${APP_LOW_NAME}.ico
          DESTINATION ./)
  install(FILES ${APP_QM}
          DESTINATION ./l10n)
  install(FILES ${APP_TEXT}
          DESTINATION ./)
elseif(APPLE)
  message(STATUS "[${APP_LOW_NAME}] You will probably install on Mac OS X.")

  set_target_properties(${EXE_NAME}
                        PROPERTIES
                        MACOSX_BUNDLE_INFO_PLIST
                        "${CMAKE_SOURCE_DIR}/resources/macosx/Info.plist.in")

  set_source_files_properties(${CMAKE_SOURCE_DIR}/resources/macosx/${APP_LOW_NAME}.icns
                              PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  set(MACOSX_BUNDLE_ICON_FILE ${APP_LOW_NAME}.icns)
  set(MACOSX_BUNDLE_INFO_STRING "${EXE_NAME} ${APP_VERSION}")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "${APP_URL_BACKWARDS}")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${APP_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_NAME "${EXE_NAME}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}")
  set(MACOSX_BUNDLE_COPYRIGHT "(c) ${APP_AUTHOR}")

  install(TARGETS ${EXE_NAME} BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX})

  # Setup custom "bundle" prefix.
  set(APPLE_PREFIX ${CMAKE_INSTALL_PREFIX}/${EXE_NAME}.app/Contents/Resources)

  install(DIRECTORY resources/initial_feeds DESTINATION ${APPLE_PREFIX})
  install(DIRECTORY resources/graphics/icons/mini-kfaenza DESTINATION ${APPLE_PREFIX}/icons)

  if(ENABLE_NUMIX)
    install(DIRECTORY resources/graphics/icons/numix DESTINATION ${APPLE_PREFIX}/icons)
  endif(ENABLE_NUMIX)

  install(DIRECTORY resources/skins DESTINATION ${APPLE_PREFIX})
  install(DIRECTORY resources/misc DESTINATION ${APPLE_PREFIX})
  install(FILES resources/graphics/${APP_LOW_NAME}_128.png DESTINATION ${APPLE_PREFIX} RENAME ${APP_LOW_NAME}.png)
  install(FILES resources/graphics/${APP_LOW_NAME}_plain_128.png DESTINATION ${APPLE_PREFIX} RENAME ${APP_LOW_NAME}_plain.png)
  install(FILES ${APP_QM} DESTINATION ${APPLE_PREFIX}/l10n)
  install(FILES ${APP_TEXT} DESTINATION ${APPLE_PREFIX}/information)
elseif(UNIX)
  message(STATUS "[${APP_LOW_NAME}] You will probably install on Linux.")

  install(TARGETS ${EXE_NAME} RUNTIME DESTINATION bin)
  install(DIRECTORY resources/initial_feeds DESTINATION share/${APP_LOW_NAME})
  install(DIRECTORY resources/graphics/icons/mini-kfaenza DESTINATION share/${APP_LOW_NAME}/icons)

  if(ENABLE_NUMIX)
    install(DIRECTORY resources/graphics/icons/numix DESTINATION share/${APP_LOW_NAME}/icons)
  endif(ENABLE_NUMIX)

  install(DIRECTORY resources/skins/base DESTINATION share/${APP_LOW_NAME}/skins)
  install(DIRECTORY resources/misc  DESTINATION share/${APP_LOW_NAME})
  install(FILES ${CMAKE_BINARY_DIR}/resources/desktop/${APP_LOW_NAME}.desktop  DESTINATION share/applications)
  install(FILES resources/graphics/${APP_LOW_NAME}_128.png DESTINATION share/pixmaps RENAME ${APP_LOW_NAME}.png)
  install(FILES resources/graphics/${APP_LOW_NAME}_plain_128.png DESTINATION share/rssguard/icons RENAME ${APP_LOW_NAME}_plain.png)
  install(FILES ${APP_QM} DESTINATION share/${APP_LOW_NAME}/l10n)
  install(FILES ${APP_TEXT} DESTINATION share/${APP_LOW_NAME}/information)
endif(WIN32 OR OS2)

# Custom target for packaging.
if(WIN32)
  if(USE_NSIS)
    set(CPACK_GENERATOR "NSIS")
  else(USE_NSIS)
    set(CPACK_GENERATOR "ZIP")
  endif(USE_NSIS)
else(WIN32)
  set(CPACK_GENERATOR "ZIP")
endif(WIN32)

set(CPACK_PACKAGE_NAME ${APP_LOW_NAME})
set(CPACK_PACKAGE_VERSION ${APP_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_VENDOR ${APP_AUTHOR})
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY ${APP_NAME})
set(CPACK_IGNORE_FILES "/resources/aur/;\\\\.psd$;/resources/deployment;/CVS/;/\\\\.svn/;/\\\\.git/;\\\\.swp$;/CMakeLists.txt.user;\\\\.#;/#;\\\\.tar.gz$;/CMakeFiles/;CMakeCache.txt;\\\\.qm$;/build/;\\\\.diff$;.DS_Store'")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})

set(CPACK_NSIS_INSTALLED_ICON_NAME "${APP_LOW_NAME}.ico")
set(CPACK_NSIS_HELP_LINK ${APP_URL})
set(CPACK_NSIS_URL_INFO_ABOUT ${APP_URL})
set(CPACK_NSIS_CONTACT ${APP_EMAIL})

# Load packaging facilities.
include(CPack)

# Configure file with custom definitions for NSIS.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/nsis/NSIS.definitions.nsh.in
  ${CMAKE_CURRENT_BINARY_DIR}/resources/nsis/NSIS.definitions.nsh
)

# make dist implementation.
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# make lupdate implementation.
if(USE_QT_5)
  add_custom_target(lupdate
    ${Qt5Core_QMAKE_EXECUTABLE} -project -o ${CMAKE_BINARY_DIR}/${APP_LOW_NAME}.pro
      COMMAND ${Qt5_LUPDATE_EXECUTABLE} -no-obsolete -ts ${APP_TRANSLATIONS_WO_QT} -pro ${CMAKE_BINARY_DIR}/${APP_LOW_NAME}.pro
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
else(USE_QT_5)
  add_custom_target(lupdate
    ${QT_QMAKE_EXECUTABLE} -project -o ${CMAKE_BINARY_DIR}/${APP_LOW_NAME}.pro
      COMMAND ${QT_LUPDATE_EXECUTABLE} -no-obsolete -ts ${APP_TRANSLATIONS_WO_QT} -pro ${CMAKE_BINARY_DIR}/${APP_LOW_NAME}.pro
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif(USE_QT_5)
