#################################################################
#
# This file is part of RSS Guard.
#
# Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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
#   -DBUNDLE_ICON_THEMES=ON
#     If "ON", then custom icons theme(s) will be bundled with application
#     installation. If "OFF", then no icon theme(s) will be available for
#     the application and application will run in iconless mode.
#     Default and recommended value is "ON".
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
#   - Elbert Pol (huge OS/2-related contributions)
#
#################################################################


# Setup basic variables.
project(rssguard)

set(APP_NAME "RSS Guard")
set(APP_LOW_NAME "rssguard")
set(APP_VERSION "2.0.0.1")
set(FILE_VERSION "2,0,0,1")
set(APP_AUTHOR "Martin Rotter")
set(APP_URL "http://bitbucket.org/skunkos/rssguard")
set(APP_URL_ISSUES "http://bitbucket.org/skunkos/rssguard/issues")
set(APP_URL_ISSUES_NEW "http://bitbucket.org/skunkos/rssguard/issues/new")
set(APP_EMAIL "rotter.martinos@gmail.com")
set(MINIMUM_QT_VERSION 4.7.3)
set(EXE_NAME ${APP_LOW_NAME})

if(APPLE)
  set(APP_URL_BACKWARDS "org.bitbucket.rssguard")
endif(APPLE)

# Options declaration.
option(USE_QT_5 "Use Qt 5 for building" OFF)
option(ENABLE_OS2_RC "Compile application icon on OS/2" OFF)
option(BUNDLE_ICON_THEMES "Equip installation with custom icon themes" ON)

if(WIN32)
  option(USE_NSIS "Use NSIS generator to produce installer" OFF)
endif(WIN32)

# Set appropriate CMake policies.
if(POLICY CMP0012)
  cmake_policy(SET CMP0012 NEW)
endif()

if(${USE_QT_5})
  cmake_minimum_required(VERSION 2.8.10)
else(${USE_QT_5})
  cmake_minimum_required(VERSION 2.8.0)
endif(${USE_QT_5})

if(APPLE AND NOT USE_QT_5)
  message(FATAL_ERROR "[${APP_LOW_NAME}] Only Qt 5.x.x is supported on Mac OS X.")
endif(APPLE AND NOT USE_QT_5)

message(STATUS "")
message(STATUS "[${APP_LOW_NAME}] ==== Options summary ====")
message(STATUS "[${APP_LOW_NAME}] Use Qt 5 for building -> ${USE_QT_5}")
message(STATUS "[${APP_LOW_NAME}] Equip installation with custom icon themes -> ${BUNDLE_ICON_THEMES}")
message(STATUS "[${APP_LOW_NAME}] Compile application icon on OS/2 -> ${ENABLE_OS2_RC}")

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
endif()

# Select which Qt version to use.
if(${USE_QT_5})
  message(STATUS "[${APP_LOW_NAME}] Using Qt 5 library for building.")
  
  # Instruct CMake to run moc automatically when needed.
  set(CMAKE_AUTOMOC ON)
else(${USE_QT_5})
  message(STATUS "[${APP_LOW_NAME}] Using Qt 4 library for building.")
endif(${USE_QT_5})

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
  add_definitions(-pedantic -Wall -Wextra)
endif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

# Verbose compiling outputs.
set(CMAKE_VERBOSE_MAKEFILE ON)
endif(CMAKE_BUILD_TYPE STREQUAL "release" OR CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RELEASE")

# Force Qt to use string builders.
add_definitions(-DQT_USE_QSTRINGBUILDER)

# Force Unicode.
add_definitions(-DUNICODE -D_UNICODE)

if(${USE_QT_5})
  # Check for C++ 11 features availability.
  # See http://stackoverflow.com/questions/10984442/how-to-detect-c11-support-of-a-compiler-with-cmake
  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION
    )

    if(NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
      message(FATAL_ERROR "[${APP_LOW_NAME}] Your C++ compiler does not support C++ 11.")
    else(NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
      add_definitions(-std=c++11)
    endif(NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))

  elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    add_definitions(-std=c++11 -stdlib=libc++)
  elseif(${MSVC_VERSION} VERSION_LESS 1600)
    message(FATAL_ERROR "[${APP_LOW_NAME}] Your C++ compiler does not support C++ 11.")
  endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
endif(${USE_QT_5})

# Setup librar  ies.
if(${USE_QT_5})
  find_package(Qt5 REQUIRED Sql WebKit WebKitWidgets Widgets Xml XmlPatterns Network LinguistTools)
else(${USE_QT_5})
  set(QT_MIN_VERSION ${MINIMUM_QT_VERSION})
  find_package(Qt4 REQUIRED QtCore QtGui QtSql QtNetwork QtWebkit QtXml QtXmlPatterns)
  include(${QT_USE_FILE})
endif(${USE_QT_5})

# Configure QStringBuilder behavior.
if(${USE_QT_5})
  message(STATUS "[${APP_LOW_NAME}] Enabling fast QString concatenation.")

  add_definitions(-DQT_USE_FAST_CONCATENATION)
  add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)
elseif(NOT ${QTVERSION} VERSION_LESS 4.8.0)
  message(STATUS "[${APP_LOW_NAME}] Enabling fast QString concatenation.")

  add_definitions(-DQT_USE_FAST_CONCATENATION)
  add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)
else(NOT ${QTVERSION} VERSION_LESS 4.8.0)
  message(STATUS "[${APP_LOW_NAME}] Disabling fast QString concatenation.")
endif(${USE_QT_5})

# Configure executable "properties" for Windows or OS2.
if(WIN32)
  message(STATUS "[${APP_LOW_NAME}] Generating executable file properties for Windows.")
  configure_file (
    ${PROJECT_SOURCE_DIR}/resources/executable_properties/rssguard_win.rc.in
    ${CMAKE_BINARY_DIR}/resources/executable_properties/rssguard_win.rc
  )
elseif(OS2 AND ${ENABLE_OS2_RC})
  message(STATUS "[${APP_LOW_NAME}] Generating executable file properties for OS2.")
  configure_file (
    ${PROJECT_SOURCE_DIR}/resources/executable_properties/rssguard_os2.rc.in
    ${CMAKE_BINARY_DIR}/resources/executable_properties/rssguard_os2.rc
  )
endif(WIN32)

# Compile application icon if compiling with MinGW on WIN32 or with OS2.
if(MINGW AND WIN32)
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
endif(MINGW AND WIN32)

# APP source files.
set(APP_SOURCES
  ${APP_SOURCES}

  # QTSINGLEAPPLICATION sources.
  src/qtsingleapplication/qtlocalpeer.cpp
  src/qtsingleapplication/qtsinglecoreapplication.cpp
  src/qtsingleapplication/qtsingleapplication.cpp

  # GUI sources.
  src/gui/formmain.cpp
  src/gui/systemtrayicon.cpp
  src/gui/formsettings.cpp
  src/gui/formabout.cpp
  src/gui/baselineedit.cpp
  src/gui/locationlineedit.cpp
  src/gui/tabwidget.cpp
  src/gui/tabbar.cpp
  src/gui/tabcontent.cpp
  src/gui/feedmessageviewer.cpp
  src/gui/feedsview.cpp
  src/gui/messagesview.cpp
  src/gui/statusbar.cpp
  src/gui/formcategorydetails.cpp
  src/gui/formfeeddetails.cpp
  src/gui/plaintoolbutton.cpp
  src/gui/lineeditwithstatus.cpp
  src/gui/widgetwithstatus.cpp
  src/gui/labelwithstatus.cpp
  src/gui/messagebox.cpp
  src/gui/formupdate.cpp
  src/gui/comboboxwithstatus.cpp
  src/gui/basetoolbar.cpp
  src/gui/messagestoolbar.cpp
  src/gui/feedstoolbar.cpp
  src/gui/toolbareditor.cpp
  src/gui/messagessearchlineedit.cpp
  src/gui/formimportexport.cpp

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

  # CORE sources.
  src/core/messagesmodel.cpp
  src/core/messagesproxymodel.cpp
  src/core/feedsmodel.cpp
  src/core/feedsproxymodel.cpp
  src/core/feedsmodelcategory.cpp
  src/core/feedsmodelrootitem.cpp
  src/core/feedsmodelfeed.cpp
  src/core/parsingfactory.cpp
  src/core/feeddownloader.cpp
  src/core/feedsimportexportmodel.cpp

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
  src/gui/formmain.h
  src/gui/systemtrayicon.h
  src/gui/formsettings.h
  src/gui/formabout.h
  src/gui/baselineedit.h
  src/gui/locationlineedit.h
  src/gui/tabwidget.h
  src/gui/tabbar.h
  src/gui/tabcontent.h
  src/gui/feedmessageviewer.h
  src/gui/feedsview.h
  src/gui/messagesview.h
  src/gui/statusbar.h
  src/gui/formcategorydetails.h
  src/gui/formfeeddetails.h
  src/gui/plaintoolbutton.h
  src/gui/lineeditwithstatus.h
  src/gui/widgetwithstatus.h
  src/gui/labelwithstatus.h
  src/gui/messagebox.h
  src/gui/formupdate.h
  src/gui/comboboxwithstatus.h
  src/gui/basetoolbar.h
  src/gui/messagestoolbar.h
  src/gui/feedstoolbar.h
  src/gui/toolbareditor.h
  src/gui/messagessearchlineedit.h
  src/gui/formimportexport.h

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

  # CORE headers.
  src/core/messagesmodel.h
  src/core/messagesproxymodel.h
  src/core/feedsmodel.h
  src/core/feedsproxymodel.h
  src/core/feeddownloader.h
  src/core/feedsimportexportmodel.h

  # NETWORK-WEB headers.
  src/network-web/webpage.h
  src/network-web/basenetworkaccessmanager.h
  src/network-web/webbrowsernetworkaccessmanager.h
  src/network-web/silentnetworkaccessmanager.h
  src/network-web/webfactory.h
  src/network-web/webbrowser.h
  src/network-web/webview.h
  src/network-web/downloader.h
)

# APP form files.
set(APP_FORMS
  src/gui/formupdate.ui
  src/gui/formmain.ui
  src/gui/formsettings.ui
  src/gui/formabout.ui
  src/gui/formcategorydetails.ui
  src/gui/formfeeddetails.ui
  src/gui/toolbareditor.ui
  src/gui/formimportexport.ui
)

# APP translations.
set(APP_TRANSLATIONS
  localization/rssguard-cs_CZ.ts
  localization/rssguard-de_DE.ts
  localization/rssguard-en_GB.ts
  localization/rssguard-fr_FR.ts
  localization/rssguard-it_IT.ts
  localization/rssguard-nl_NL.ts
  localization/qt-cs_CZ.ts
  localization/qt-de_DE.ts
  localization/qt-fr_FR.ts
  localization/qt-nl_NL.ts
)

set(APP_TRANSLATIONS_WO_QT
  localization/rssguard-cs_CZ.ts
  localization/rssguard-de_DE.ts
  localization/rssguard-en_GB.ts
  localization/rssguard-fr_FR.ts
  localization/rssguard-it_IT.ts
  localization/rssguard-nl_NL.ts
)

set(APP_TEXT
  resources/text/CHANGELOG
  resources/text/COPYING_GNU_GPL
  resources/text/COPYING_GNU_GPL_HTML
  resources/text/COPYING_BSD
)

if(WIN32)
  set(APP_DLLS_QT4_MSVC2010
    resources/binaries/windows/deployment/qt4-msvc2010/libeay32.dll
    resources/binaries/windows/deployment/qt4-msvc2010/msvcp100.dll
    resources/binaries/windows/deployment/qt4-msvc2010/msvcr100.dll
    resources/binaries/windows/deployment/qt4-msvc2010/QtCore4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/QtGui4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/QtNetwork4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/QtSql4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/QtWebKit4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/QtXml4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/ssleay32.dll
    resources/binaries/windows/deployment/qt4-msvc2010/libintl.dll
    resources/binaries/windows/deployment/qt4-msvc2010/libmysql.dll
    resources/binaries/windows/deployment/qt4-msvc2010/libpq.dll
  )

  set(APP_DLLS_QT4_MSVC2010_IMAGEFORMATS
    resources/binaries/windows/deployment/qt4-msvc2010/imageformats/qgif4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/imageformats/qico4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/imageformats/qjpeg4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/imageformats/qmng4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/imageformats/qsvg4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/imageformats/qtga4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/imageformats/qtiff4.dll
  )

  set(APP_DLLS_QT4_MSVC2010_SQLDRIVERS
    resources/binaries/windows/deployment/qt4-msvc2010/sqldrivers/qsqlite4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/sqldrivers/qsqlmysql4.dll
    resources/binaries/windows/deployment/qt4-msvc2010/sqldrivers/qsqlpsql4.dll
  )
endif(WIN32)

# Add custom icon on Mac OS X.
if(APPLE)
  SET (APP_SOURCES ${APP_SOURCES} resources/macosx/rssguard.icns)
endif(APPLE)

# Wrap files, create moc files.
if(${USE_QT_5})
  qt5_wrap_ui(APP_UI ${APP_FORMS})
  
  # Load translations.
  if(Qt5LinguistTools_FOUND)
    message(STATUS "[${APP_LOW_NAME}] Qt Linguist Tools found. Translations will get compiled.")
    qt5_add_translation(APP_QM ${APP_TRANSLATIONS})
  else(Qt5LinguistTools_FOUND)
    message(STATUS "[${APP_LOW_NAME}] Qt Linguist Tools NOT found. No compilation for translations.")
  endif(Qt5LinguistTools_FOUND)
else(${USE_QT_5})
  qt4_wrap_cpp(APP_MOC ${APP_HEADERS})
  qt4_wrap_ui(APP_UI ${APP_FORMS})
  
  # Load translations.
  qt4_add_translation(APP_QM ${APP_TRANSLATIONS})

  # Setup Qt for "rssguard_updater".
  if(WIN32 OR OS2)
    qt4_wrap_cpp(UPDATER_MOC ${UPDATER_HEADERS})
  endif(WIN32 OR OS2)
endif(${USE_QT_5})

# Include additional directory paths.
include_directories (
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/gui
  ${CMAKE_SOURCE_DIR}/src/network-web
  ${CMAKE_SOURCE_DIR}/src/dynamic-shortcuts
  ${CMAKE_BINARY_DIR}/src
)

# Compile "rssguard"
if(${USE_QT_5})
  add_executable(${EXE_NAME} WIN32 MACOSX_BUNDLE
    ${APP_SOURCES}
    ${APP_FORMS}
    ${APP_RCC}
    ${APP_QM}
  )

  if(WIN32)
    target_link_libraries(${EXE_NAME} Qt5::WinMain)
  endif(WIN32)
  
  # Use modules from Qt.
  qt5_use_modules(${EXE_NAME}
    Core
    Widgets
    Sql
    Network
    Xml
    WebKit
    WebKitWidgets
  )
# Setup compilation for Qt 4.
else(${USE_QT_5})
  add_executable(${EXE_NAME} WIN32 MACOSX_BUNDLE
    ${APP_SOURCES}
    ${APP_FORMS}
    ${APP_MOC}
    ${APP_RCC}
    ${APP_QM}
  )

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
endif(${USE_QT_5})

# Installation stage.
if(WIN32 OR OS2)
  message(STATUS "[${APP_LOW_NAME}] You will probably install on Windows or OS/2.")
  install(TARGETS ${EXE_NAME}
          RUNTIME DESTINATION ./)
  install(TARGETS ${UPDATER_EXE_NAME}
          RUNTIME DESTINATION ./${UPDATER_SUBFOLDER})

  # Copy DLLs and other binary files for main installation and updater.
  if(WIN32 AND NOT ${USE_QT_5})
    install(FILES ${APP_DLLS_QT4_MSVC2010}
            DESTINATION ./)
    install(FILES ${APP_DLLS_QT4_MSVC2010_IMAGEFORMATS}
            DESTINATION ./imageformats)
    install(FILES ${APP_DLLS_QT4_MSVC2010_SQLDRIVERS}
            DESTINATION ./sqldrivers)

    install(FILES ${APP_DLLS_QT4_MSVC2010}
            DESTINATION ./${UPDATER_SUBFOLDER})
  endif(WIN32 AND NOT ${USE_QT_5})

  if(BUNDLE_ICON_THEMES)
    install(DIRECTORY resources/graphics/icons/mini-kfaenza
            DESTINATION ./icons)
  endif(BUNDLE_ICON_THEMES)

  install(DIRECTORY resources/skins/base
          DESTINATION ./skins)
  install(DIRECTORY resources/misc
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

  if(BUNDLE_ICON_THEMES)
    install(DIRECTORY resources/graphics/icons/mini-kfaenza
            DESTINATION ${APPLE_PREFIX}/icons)
  endif(BUNDLE_ICON_THEMES)

  install(DIRECTORY resources/skins
          DESTINATION ${APPLE_PREFIX})
  install(DIRECTORY resources/misc
          DESTINATION ${APPLE_PREFIX})
  install(FILES resources/graphics/${APP_LOW_NAME}_128.png
          DESTINATION ${APPLE_PREFIX}
          RENAME ${APP_LOW_NAME}.png)
  install(FILES resources/graphics/${APP_LOW_NAME}_plain_128.png
          DESTINATION ${APPLE_PREFIX}
          RENAME ${APP_LOW_NAME}_plain.png)
  install(FILES ${APP_QM}
          DESTINATION ${APPLE_PREFIX}/l10n)
  install(FILES ${APP_TEXT}
          DESTINATION ${APPLE_PREFIX}/information)
elseif(UNIX)
  message(STATUS "[${APP_LOW_NAME}] You will probably install on Linux.")
  install(TARGETS ${EXE_NAME}
          RUNTIME DESTINATION bin)

  if(BUNDLE_ICON_THEMES)
    install(DIRECTORY resources/graphics/icons/mini-kfaenza
            DESTINATION share/${APP_LOW_NAME}/icons)
  endif(BUNDLE_ICON_THEMES)

  install(DIRECTORY resources/skins/base
          DESTINATION share/${APP_LOW_NAME}/skins)
  install(DIRECTORY resources/misc
          DESTINATION share/${APP_LOW_NAME})
  install(FILES ${CMAKE_BINARY_DIR}/resources/desktop/${APP_LOW_NAME}.desktop
          DESTINATION share/applications)
  install(FILES resources/graphics/${APP_LOW_NAME}_128.png
          DESTINATION share/pixmaps
          RENAME ${APP_LOW_NAME}.png)
  install(FILES resources/graphics/${APP_LOW_NAME}_plain_128.png
          DESTINATION share/rssguard/icons
          RENAME ${APP_LOW_NAME}_plain.png)
  install(FILES ${APP_QM}
          DESTINATION share/${APP_LOW_NAME}/l10n)
  install(FILES ${APP_TEXT}
          DESTINATION share/${APP_LOW_NAME}/information)
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
if(${USE_QT_5})
  add_custom_target(lupdate
    ${Qt5Core_QMAKE_EXECUTABLE} -project -o ${CMAKE_BINARY_DIR}/${APP_LOW_NAME}.pro
      COMMAND ${Qt5_LUPDATE_EXECUTABLE} -no-obsolete -ts ${APP_TRANSLATIONS_WO_QT} -pro ${CMAKE_BINARY_DIR}/${APP_LOW_NAME}.pro
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
else(${USE_QT_5})
  add_custom_target(lupdate
    ${QT_QMAKE_EXECUTABLE} -project -o ${CMAKE_BINARY_DIR}/${APP_LOW_NAME}.pro
      COMMAND ${QT_LUPDATE_EXECUTABLE} -no-obsolete -ts ${APP_TRANSLATIONS_WO_QT} -pro ${CMAKE_BINARY_DIR}/${APP_LOW_NAME}.pro
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif(${USE_QT_5})
