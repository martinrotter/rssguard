#################################################################
#
# This file is part of RSS Guard.
#
# Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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
#  WARNING: This script uses file(GLOB ...) function. YOU MUST
#  recompile this CMake script once any new file is added.
#
# Usage:
#   a) DEBUG build for testing.
#     cmake -DCMAKE_BUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX=/usr
#
#   b) RELEASE build for production use.
#     cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_INSTALL_PREFIX=/usr
#
# Arguments:
#   -DCMAKE_INSTALL_PREFIX=/usr
#     Installation path, replace with something like "C:\rssguard" on Windows.
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
#   - Qt 5.6 and higher is required,
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
set(APP_VERSION "3.2.2")
set(FILE_VERSION "3,2,2,0")
set(APP_AUTHOR "Martin Rotter")
set(APP_URL "http://bitbucket.org/skunkos/rssguard")
set(APP_URL_ISSUES "http://bitbucket.org/skunkos/rssguard/issues")
set(APP_URL_ISSUES_NEW_GITHUB "https://github.com/martinrotter/rssguard/issues/new")
set(APP_URL_ISSUES_NEW_BITBUCKET "http://bitbucket.org/skunkos/rssguard/issues/new")
set(APP_URL_WIKI "https://bitbucket.org/skunkos/rssguard/wiki/Home")
set(APP_EMAIL "rotter.martinos@gmail.com")
set(MINIMUM_QT_VERSION 5.2.0)
set(EXE_NAME ${APP_LOW_NAME})

if(APPLE)
  set(APP_URL_BACKWARDS "org.bitbucket.${APP_LOW_NAME}")
endif(APPLE)

if(WIN32)
  option(USE_NSIS "Use NSIS generator to produce installer" OFF)
endif(WIN32)

# Set appropriate CMake policies.
if(POLICY CMP0012)
  cmake_policy(SET CMP0012 NEW)
endif()

if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

cmake_minimum_required(VERSION 2.8.11)

message(STATUS "")
message(STATUS "[${APP_LOW_NAME}] ==== Options summary ====")

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

message(STATUS "[${APP_LOW_NAME}] Using Qt 5 library for building.")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

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
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION
    )
  if(${GCC_VERSION} VERSION_GREATER 4.7 OR ${GCC_VERSION} VERSION_EQUAL 4.7)
    add_definitions(-std=c++11)
  elseif(GCC_VERSION VERSION_GREATER 4.3 OR GCC_VERSION VERSION_EQUAL 4.3)
    add_definitions(-std=gnu++0x)
  else(${GCC_VERSION} VERSION_GREATER 4.7 OR ${GCC_VERSION} VERSION_EQUAL 4.7)
    message(FATAL_ERROR "[${APP_LOW_NAME}] Your C++ compiler does not support C++ 11.")
  endif(${GCC_VERSION} VERSION_GREATER 4.7 OR ${GCC_VERSION} VERSION_EQUAL 4.7)

elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  add_definitions(-std=c++11 -stdlib=libc++)
elseif(${MSVC_VERSION} VERSION_LESS 1600)
  message(FATAL_ERROR "[${APP_LOW_NAME}] Your C++ compiler does not support C++ 11.")
endif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")

# Setup libraries.
if(UNIX)
  # On Unices, enable D-Bus support.
  find_package(Qt5 REQUIRED DBus Sql Widgets Xml Network LinguistTools PrintSupport)
else(UNIX)
  find_package(Qt5 REQUIRED Sql Widgets Xml Network LinguistTools PrintSupport)
endif(UNIX)

message(STATUS "[${APP_LOW_NAME}] Found Qt version: '${Qt5_VERSION}'.")

if(Qt5_VERSION VERSION_LESS MINIMUM_QT_VERSION)
  message(FATAL_ERROR "[${APP_LOW_NAME}] Installed Qt has lower version than needed. You need at least '${MINIMUM_QT_VERSION}'.")
endif(Qt5_VERSION VERSION_LESS MINIMUM_QT_VERSION)

# Configure QStringBuilder behavior.
message(STATUS "[${APP_LOW_NAME}] Enabling fast QString concatenation.")

add_definitions(-DQT_USE_FAST_CONCATENATION)
add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)

# Configure executable "properties" for Windows.
if(WIN32)
  message(STATUS "[${APP_LOW_NAME}] Generating executable file properties for Windows.")
  configure_file (
    ${PROJECT_SOURCE_DIR}/resources/executable_properties/rssguard_win.rc.in
    ${CMAKE_BINARY_DIR}/resources/executable_properties/rssguard_win.rc
    )
endif(WIN32)

# Set special linker flag for Windows XP & MSVC++ 2013 support.
if(WIN32 AND MSVC)
  set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS,5.01" )
endif(WIN32 AND MSVC)

# Compile application icon if compiling with MinGW on WIN32.
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
elseif(WIN32 AND MSVC)
  # MSVC takes care of this automatically - no need to use windres.exe
  # for MSVC compilers.
  set(APP_SOURCES ${APP_SOURCES} ${CMAKE_BINARY_DIR}/resources/executable_properties/rssguard_win.rc)
endif(WIN32 AND MINGW)

# APP source files.
file( GLOB APP_SOURCES_GL
  "src/qtsingleapplication/qtsingleapplication.cpp"
  "src/qtsingleapplication/qtsinglecoreapplication.cpp"
  "src/qtsingleapplication/qtlocalpeer.cpp"

  "src/gui/dialogs/*.cpp"
  "src/gui/notifications/*.cpp"
  "src/gui/*.cpp"
  "src/dynamic-shortcuts/*.cpp"
  "src/miscellaneous/*.cpp"
  "src/miscellaneous/simplecrypt/*.cpp"
  "src/exceptions/*.cpp"
  "src/core/*.cpp"
  "src/services/abstract/*.cpp"
  "src/services/standard/*.cpp"
  "src/services/standard/gui/*.cpp"
  "src/services/tt-rss/*.cpp"
  "src/services/tt-rss/gui/*.cpp"
  "src/services/tt-rss/network/*.cpp"
  "src/services/owncloud/*.cpp"
  "src/services/owncloud/network/*.cpp"
  "src/services/owncloud/gui/*.cpp"
  "src/network-web/*.cpp"
  "src/*.cpp")

set(APP_SOURCES
  ${APP_SOURCES}
  ${APP_SOURCES_GL}
  )

# APP headers.
file( GLOB APP_HEADERS
  "src/qtsingleapplication/qtsingleapplication.h"
  "src/qtsingleapplication/qtsinglecoreapplication.h"
  "src/qtsingleapplication/qtlocalpeer.h"

  "src/gui/dialogs/*.h"
  "src/gui/notifications/*.h"
  "src/gui/*.h"
  "src/dynamic-shortcuts/*.h"
  "src/exceptions/*.h"
  "src/miscellaneous/*.h"
  "src/miscellaneous/simplecrypt/*.h"
  "src/core/*.h"
  "src/services/abstract/*.h"
  "src/services/standard/*.h"
  "src/services/standard/gui/*.h"
  "src/services/tt-rss/*.h"
  "src/services/tt-rss/gui/*.h"
  "src/services/tt-rss/network/*.h"
  "src/services/owncloud/*.h"
  "src/services/owncloud/network/*.h"
  "src/services/owncloud/gui/*.h"
  "src/network-web/*.h")

# APP forms.
file( GLOB APP_FORMS
  "src/gui/dialogs/*.ui"
  "src/gui/*.ui"
  "src/services/standard/gui/*.ui"
  "src/services/tt-rss/gui/*.ui"
  "src/services/owncloud/gui/*.ui"
  "src/network-web/*.ui")

# APP translations.
file( GLOB APP_TRANSLATIONS
  "localization/*.ts")

file( GLOB APP_TRANSLATIONS_WO_QT
  "localization/rssguard*.ts")

file( GLOB APP_TEXT
  "resources/text/C*")

if(WIN32)
  file( GLOB APP_DLLS_QT5_MSVC2013
    "resources/binaries/windows/qt5-msvc2013/*.dll")

  file( GLOB APP_DLLS_QT5_MSVC2013_BEARER
    "resources/binaries/windows/qt5-msvc2013/bearer/*.dll")

  file( GLOB APP_DLLS_QT5_MSVC2013_ICONENGINES
    "resources/binaries/windows/qt5-msvc2013/iconengines/*.dll")

  file( GLOB APP_DLLS_QT5_MSVC2013_IMAGEFORMATS
    "resources/binaries/windows/qt5-msvc2013/imageformats/*.dll")

  file( GLOB APP_DLLS_QT5_MSVC2013_SQLDRIVERS
    "resources/binaries/windows/qt5-msvc2013/sqldrivers/*.dll")

  file( GLOB APP_DLLS_QT5_MSVC2013_PLATFORMS
    "resources/binaries/windows/qt5-msvc2013/platforms/*.dll")
endif(WIN32)

# Add custom icon on Mac OS X.
if(APPLE)
  set(APP_SOURCES ${APP_SOURCES} resources/macosx/rssguard.icns)
endif(APPLE)

# Wrap files, create moc files.
qt5_wrap_ui(APP_UI ${APP_FORMS})

# Load translations.
if(${Qt5LinguistTools_FOUND})
  message(STATUS "[${APP_LOW_NAME}] Qt Linguist Tools found. Translations will get compiled.")
  qt5_add_translation(APP_QM ${APP_TRANSLATIONS})
else(${Qt5LinguistTools_FOUND})
  message(STATUS "[${APP_LOW_NAME}] Qt Linguist Tools NOT found. No compilation for translations.")
endif(${Qt5LinguistTools_FOUND})

# Include additional directory paths.
include_directories (
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/gui
  ${CMAKE_SOURCE_DIR}/src/gui/dialogs
  ${CMAKE_SOURCE_DIR}/src/gui/notifications
  ${CMAKE_SOURCE_DIR}/src/network-web
  ${CMAKE_SOURCE_DIR}/src/dynamic-shortcuts
  ${CMAKE_BINARY_DIR}/src
  )

# Compile "rssguard"
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
    PrintSupport
    )
endif(UNIX)

set(FAENZA_FOLDERS
  "resources/graphics/Faenza/actions"
  "resources/graphics/Faenza/categories"
  "resources/graphics/Faenza/devices"
  "resources/graphics/Faenza/emblems"
  "resources/graphics/Faenza/mimetypes"
  "resources/graphics/Faenza/places"
  "resources/graphics/Faenza/status"
  )

file( GLOB MISC_ICONS
  "resources/graphics/*.png")

# Installation stage.
if(WIN32)
  message(STATUS "[${APP_LOW_NAME}] You will probably install on Windows or OS/2.")
  install(TARGETS ${EXE_NAME} RUNTIME DESTINATION ./)

  # Copy DLLs and other binary files for main installation.
  install(FILES ${APP_DLLS_QT5_MSVC2013} DESTINATION ./)
  install(FILES ${APP_DLLS_QT5_MSVC2013_BEARER} DESTINATION ./bearer)
  install(FILES ${APP_DLLS_QT5_MSVC2013_ICONENGINES} DESTINATION ./iconengines)
  install(FILES ${APP_DLLS_QT5_MSVC2013_IMAGEFORMATS} DESTINATION ./imageformats)
  install(FILES ${APP_DLLS_QT5_MSVC2013_PLATFORMS} DESTINATION ./platforms)
  install(FILES ${APP_DLLS_QT5_MSVC2013_SQLDRIVERS} DESTINATION ./sqldrivers)

  install(FILES ${MISC_ICONS} DESTINATION ./icons/)
  install(FILES resources/graphics/Faenza/index.theme DESTINATION ./icons/Faenza)
  install(DIRECTORY ${FAENZA_FOLDERS} DESTINATION ./icons/Faenza)

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

  install(FILES ${MISC_ICONS} DESTINATION ${APPLE_PREFIX}/icons/)
  install(FILES resources/graphics/Faenza/index.theme DESTINATION ${APPLE_PREFIX}/icons/Faenza)
  install(DIRECTORY ${FAENZA_FOLDERS} DESTINATION ${APPLE_PREFIX}/icons/Faenza)

  install(DIRECTORY resources/initial_feeds DESTINATION ${APPLE_PREFIX})
  install(DIRECTORY resources/skins DESTINATION ${APPLE_PREFIX})
  install(DIRECTORY resources/misc DESTINATION ${APPLE_PREFIX})
  install(FILES resources/graphics/${APP_LOW_NAME}_128.png DESTINATION ${APPLE_PREFIX} RENAME ${APP_LOW_NAME}.png)
  install(FILES resources/graphics/${APP_LOW_NAME}_plain_128.png DESTINATION ${APPLE_PREFIX} RENAME ${APP_LOW_NAME}_plain.png)
  install(FILES ${APP_QM} DESTINATION ${APPLE_PREFIX}/l10n)
  install(FILES ${APP_TEXT} DESTINATION ${APPLE_PREFIX}/information)
elseif(UNIX)
  message(STATUS "[${APP_LOW_NAME}] You will probably install on Linux.")

  install(TARGETS ${EXE_NAME} RUNTIME DESTINATION bin)

  install(FILES ${MISC_ICONS} DESTINATION share/${APP_LOW_NAME}/icons)
  install(DIRECTORY resources/initial_feeds DESTINATION share/${APP_LOW_NAME})
  install(DIRECTORY resources/skins/base DESTINATION share/${APP_LOW_NAME}/skins)
  install(DIRECTORY resources/misc  DESTINATION share/${APP_LOW_NAME})
  install(FILES ${CMAKE_BINARY_DIR}/resources/desktop/${APP_LOW_NAME}.desktop  DESTINATION share/applications)
  install(FILES resources/graphics/${APP_LOW_NAME}_128.png DESTINATION share/pixmaps RENAME ${APP_LOW_NAME}.png)
  install(FILES resources/graphics/${APP_LOW_NAME}_plain_128.png DESTINATION share/rssguard/icons RENAME ${APP_LOW_NAME}_plain.png)
  install(FILES ${APP_QM} DESTINATION share/${APP_LOW_NAME}/l10n)
  install(FILES ${APP_TEXT} DESTINATION share/${APP_LOW_NAME}/information)
endif(WIN32)

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
  add_custom_target(lupdate
    ${Qt5Core_QMAKE_EXECUTABLE} -project -o ${CMAKE_BINARY_DIR}/${APP_LOW_NAME}.pro
    COMMAND ${Qt5_LUPDATE_EXECUTABLE} -no-obsolete -ts ${APP_TRANSLATIONS_WO_QT} -pro ${CMAKE_BINARY_DIR}/${APP_LOW_NAME}.pro
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
