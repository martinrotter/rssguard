#################################################################
#
# For license of this file, see <project-root-folder>/LICENSE.md.
#
# This is RSS Guard compilation script for cmake.
#
# Usage (out of source build type, we have two side by side folders:
# empty "build-dir" and RSS Guard repository "rssguard-dir"):
#   a) DEBUG build for testing.
#     cd build-dir
#     cmake -DCMAKE_BUILD_TYPE="Debug" ../rssguard-dir/
#     cmake --build .
#     cmake --install .
#
#   b) RELEASE build for production use.
#     cd build-dir
#     cmake -DCMAKE_BUILD_TYPE="Release" ../rssguard-dir/
#     cmake --build .
#     cmake --install .
#
# Variables:
#   USE_WEBENGINE - if specified, then QtWebEngine module for internal web browser is used.
#                   Otherwise simple text component is used and some features will be disabled.
#                   Default value is "false". If QtWebEngine is installed during compilation, then
#                   value of this variable is tweaked automatically.
#   {FEEDLY,GMAIL,INOREADER}_CLIENT_ID - preconfigured OAuth cliend ID.
#   {FEEDLY,GMAIL,INOREADER}_CLIENT_SECRET - preconfigured OAuth cliend SECRET.
#
# Other information:
#   - supports Windows, Linux, *BSD, macOS, OS/2, Android,
#   - Qt 5.9.0 or newer is required,
#   - cmake 3.9.0 or newer is required,
#   - if you wish to make packages for Windows, then you must initialize all submodules within repository before compilation,
#   - C++ 11/17 is required.
#
# Building on OS/2:
#   RSS Guard can run on OS/2 and if you want to compile it by yourself, you need to make sure that
#   your OS/2 distro is up-to-date and you have all dependencies installed: os2-base, all gcc-* packages,
#   libc and libcx up-to-date, kbuild-make, ash, binutils, all relevant qt5-* packages.
#
#   After your dependecies are installed, then you can compile via standard `cmake -> make -> make install` steps
#   and package with: 7z.exe a -t7z -mmt -mx9 "rssguard.7z" "<build-folder\src\rssguard\app\*" command.
#
# Authors and contributors:
#   - Martin Rotter (project leader),
#   - Elbert Pol (OS/2-related contributions),
#   - Anna Vyalkova (cmake-related contributions).
#
#################################################################

cmake_minimum_required(VERSION 3.9.0)

# Global variables describing the project.
set(APP_NAME "RSS Guard")
set(APP_EMAIL "rotter.martinos@gmail.com")
set(APP_AUTHOR "Martin Rotter")
set(APP_COPYRIGHT "\\251 2011-2022 ${APP_AUTHOR}")
set(APP_REVERSE_NAME "com.github.rssguard")
set(APP_DONATE_URL "https://github.com/sponsors/martinrotter")
set(APP_VERSION "4.2.0")

set(APP_URL "https://github.com/martinrotter/rssguard")
set(APP_URL_DOCUMENTATION "https://github.com/martinrotter/rssguard/blob/master/resources/docs/Documentation.md")
set(APP_URL_ISSUES "https://github.com/martinrotter/rssguard/issues")
set(APP_URL_ISSUES_NEW "https://github.com/martinrotter/rssguard/issues/new")

set(TYPEINFO "????")

project(rssguard VERSION ${APP_VERSION} LANGUAGES CXX)

# Basic C++ related behavior of cmake.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(UNIX)
  add_compile_options(-fPIC)
endif()

if(APPLE)
  add_compile_options(-stdlib=libc++)
  add_link_options(-stdlib=libc++)
endif()

if(${FORCE_COLORED_OUTPUT})
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
  endif()
endif()

# Aux files not used for build but include in project.
add_custom_target(AuxFiles SOURCES "resources/scripts/uncrustify/uncrustify.cfg")

# Global compilation switches.
option(BUILD_WITH_QT6 "Build application with Qt 6" OFF)
option(USE_WEBENGINE "Use QtWebEngine for embedded web browser" ON)
option(UPDATE_TRANSLATIONS "Call lupdate to update translation files from source" OFF)
option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GCC/Clang only)" OFF)
option(REVISION_FROM_GIT "Get revision using `git rev-parse`" ON)

set(FEEDLY_CLIENT_ID "" CACHE STRING "Feedly client ID")
set(FEEDLY_CLIENT_SECRET "" CACHE STRING "Feedly client secret")

set(GMAIL_CLIENT_ID "" CACHE STRING "GMail client ID")
set(GMAIL_CLIENT_SECRET "" CACHE STRING "GMail client secret")

set(INOREADER_CLIENT_ID "" CACHE STRING "Inoreader client ID")
set(INOREADER_CLIENT_SECRET "" CACHE STRING "Inoreader client secret")

# Import Qt libraries.
set(QT_COMPONENTS
  Core
  Gui
  LinguistTools
  Network
  Qml
  Sql
  Widgets
  Xml
)

if(NOT OS2)
  list(APPEND QT_COMPONENTS Multimedia)
endif()

if(USE_WEBENGINE)
  list(APPEND QT_COMPONENTS WebEngineWidgets)
  add_compile_definitions(USE_WEBENGINE)
endif()

if(UNIX AND NOT APPLE AND NOT ANDROID)
  list(APPEND QT_COMPONENTS DBus)
endif()

if(BUILD_WITH_QT6)
  find_package(QT NAMES Qt6)
  find_package(Qt6 COMPONENTS ${QT_COMPONENTS} Core5Compat REQUIRED)
else()
  find_package(QT NAMES Qt5)
  find_package(Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)

  if(Qt5Core_VERSION VERSION_LESS 5.15.0)
    # Compatibility macros.
    macro(qt_wrap_ui)
      qt5_wrap_ui(${ARGN})
    endmacro()

    macro(qt_add_resources)
      qt5_add_resources(${ARGN})
    endmacro()

    macro(qt_add_big_resources)
      qt5_add_big_resources(${ARGN})
    endmacro()

    macro(qt_create_translation)
      qt5_create_translation(${ARGN})
    endmacro()

    macro(qt_add_translation)
      qt5_add_translation(${ARGN})
    endmacro()
  endif()
endif()

#if(NOT OS2)
#  macro(qt_add_resources)
#    qt_add_big_resources(${ARGN})
#  endmacro()
#endif()

# Process oauth service secrets.
if(NOT FEEDLY_CLIENT_ID STREQUAL "" AND NOT FEEDLY_CLIENT_SECRET STREQUAL "")
  add_compile_definitions(
    FEEDLY_OFFICIAL_SUPPORT
    FEEDLY_CLIENT_ID="${FEEDLY_CLIENT_ID}"
    FEEDLY_CLIENT_SECRET="${FEEDLY_CLIENT_SECRET}"
  )
else()
  message(STATUS "Feedly client ID/secret variables are not set. Disabling official support.")
endif()

if(NOT GMAIL_CLIENT_ID STREQUAL "" AND NOT GMAIL_CLIENT_SECRET STREQUAL "")
  add_compile_definitions(
    GMAIL_OFFICIAL_SUPPORT
    GMAIL_CLIENT_ID="${GMAIL_CLIENT_ID}"
    GMAIL_CLIENT_SECRET="${GMAIL_CLIENT_SECRET}"
  )
else()
  message(STATUS "GMail client ID/secret variables are not set. Disabling official support.")
endif()

if(NOT INOREADER_CLIENT_ID STREQUAL "" AND NOT INOREADER_CLIENT_SECRET STREQUAL "")
  add_compile_definitions(
    INOREADER_OFFICIAL_SUPPORT
    INOREADER_CLIENT_ID="${INOREADER_CLIENT_ID}"
    INOREADER_CLIENT_SECRET="${INOREADER_CLIENT_SECRET}"
  )
else()
  message(STATUS "Inoreader client ID/secret variables are not set. Disabling official support.")
endif()

# Load git commit hash.
if(REVISION_FROM_GIT AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(COMMAND "git" "rev-parse" "--short" "HEAD"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE APP_REVISION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  message(STATUS "Detected git revision: '${APP_REVISION}'.")
else()
  set(APP_REVISION "")
endif()

if(NOT USE_WEBENGINE)
  set(APP_REVISION "${APP_REVISION}-nowebengine")
endif()

# Pass needed defines.
add_compile_definitions(
  APP_AUTHOR="${APP_AUTHOR}"
  APP_DONATE_URL="${APP_DONATE_URL}"
  APP_EMAIL="${APP_EMAIL}"
  APP_LONG_NAME="${APP_NAME} ${CMAKE_PROJECT_VERSION}"
  APP_LOW_H_NAME=".${CMAKE_PROJECT_NAME}"
  APP_LOW_NAME="${CMAKE_PROJECT_NAME}"
  APP_NAME="${APP_NAME}"
  APP_REVISION="${APP_REVISION}"
  APP_SYSTEM_NAME="${CMAKE_SYSTEM_NAME}"
  APP_SYSTEM_VERSION="${CMAKE_SYSTEM_PROCESSOR}"
  APP_URL="${APP_URL}"
  APP_URL_DOCUMENTATION="${APP_URL_DOCUMENTATION}"
  APP_URL_ISSUES="${APP_URL_ISSUES}"
  APP_URL_ISSUES_NEW="${APP_URL_ISSUES_NEW}"
  APP_USERAGENT="${APP_NAME}/${CMAKE_PROJECT_VERSION}"
  APP_VERSION="${CMAKE_PROJECT_VERSION}"

  QT_USE_QSTRINGBUILDER
  QT_USE_FAST_CONCATENATION
  QT_USE_FAST_OPERATOR_PLUS
  UNICODE
  _UNICODE
)

# Configure and copy some needed files.
if(WIN32)
  configure_file(
    resources/rssguard.rc.in
    ${CMAKE_BINARY_DIR}/rssguard.rc
    NEWLINE_STYLE WIN32
  )

  configure_file(
    resources/nsis/NSIS.definitions.nsh.in
    ${CMAKE_BINARY_DIR}/NSIS.definitions.nsh
  )

  file(COPY resources/nsis/NSIS.template.in DESTINATION ${CMAKE_BINARY_DIR})
  file(COPY resources/graphics/${CMAKE_PROJECT_NAME}.ico DESTINATION ${CMAKE_BINARY_DIR})
elseif(APPLE)
  configure_file(
    resources/macosx/Info.plist.in
    ${CMAKE_BINARY_DIR}/Info.plist
  )
endif()

add_subdirectory(localization)
add_subdirectory(src/librssguard)
add_subdirectory(src/rssguard)
