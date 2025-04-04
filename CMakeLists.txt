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
#   BUILD_WITH_QT6 - Build either with Qt 6 or Qt 5.
#   BUILD_MSYS2 - Build RSS Guard for MSYS2 distribution, this particularly
#                 enables relevant MSYS2 FHS tweaks.
#   USE_SYSTEM_SQLITE - Use system-wide SQLite3 library and header file. Defaults to "ON".
#   NO_UPDATE_CHECK - Disable automatic checking for new application updates.
#   IS_FLATPAK_BUILD - Set to "ON" when building RSS Guard with Flatpak.
#   FORCE_BUNDLE_ICONS - Forcibly bundles icons into executables.
#   ENABLE_MEDIAPLAYER_QTMULTIMEDIA - Enable media player (QtMultimedia/ffmpeg implementation).
#   ENABLE_MEDIAPLAYER_LIBMPV - Enable media player (libmpv implementation). Use "LibMPV_ROOT" variable to specify
#                               base libmpv directory.
#   MEDIAPLAYER_FORCE_OPENGL - When libmpv media player is enabled, then this forces it to use
#                              its OpenGL renderer. This renderer is potentially faster, more modern
#                              and supports wider range of platforms. Legacy "window-based" renderer
#                              should be likely avoided in new codebases.
#   ENABLE_COMPRESSED_SITEMAP - Set to "ON" if you want to enable support for "sitemap.xml.gz" format.
#                               This requires "zlib" library and if you want to use specific
#                               zlib location, then use "ZLIB_ROOT" variable, for example
#                               -DZLIB_ROOT="C:\\zlib"
#   NO_LITE - if specified, then QtWebEngine module for internal web browser is used
#             and also other more demanding parts of application are used.
#   {FEEDLY,GMAIL,INOREADER}_CLIENT_ID - preconfigured OAuth client ID.
#   {FEEDLY,GMAIL,INOREADER}_CLIENT_SECRET - preconfigured OAuth client SECRET.
#
# Other information:
#   - supports Windows, Linux, *BSD, macOS, OS/2, Android,
#   - Qt 5.14.0 or newer is required,
#   - Qt 6.3.0 or newer is required,
#   - cmake 3.14.0 or newer is required,
#   - if you wish to make packages for Windows, then you must initialize all submodules
#     within repository before compilation,
#   - C++ 17 is required.
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

cmake_minimum_required(VERSION 3.14.0)

# Global variables describing the project.
string(TIMESTAMP YEAR "%Y")
string(TIMESTAMP DATE "%Y-%m-%d")

set(APP_NAME "RSS Guard")
set(APP_EMAIL "rotter.martinos@gmail.com")
set(APP_AUTHOR "Martin Rotter")
set(APP_COPYRIGHT "\\251 2011-${YEAR} ${APP_AUTHOR}")
set(APP_REVERSE_NAME "io.github.martinrotter.rssguard")
set(APP_DONATE_URL "https://github.com/sponsors/martinrotter")
set(APP_VERSION "4.8.3")

set(APP_URL "https://github.com/martinrotter/rssguard")
set(APP_URL_DOCUMENTATION "https://rssguard.readthedocs.io")
set(APP_URL_ISSUES_NEW "https://github.com/martinrotter/rssguard/issues/new/choose")

set(TYPEINFO "????")

project(rssguard VERSION ${APP_VERSION} LANGUAGES CXX C)

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

# Global compilation switches.
option(BUILD_WITH_QT6 "Build application with Qt 6." ON)
option(BUILD_MSYS2 "Build application for MSYS2 ecosystem." OFF)
option(USE_SYSTEM_SQLITE "Use system-wide SQLite3 library." ON)
option(NO_LITE "Enable QtWebEngine and other more demanding components." ON)
option(UPDATE_TRANSLATIONS "Call lupdate to update translation files from source (Qt 6 only)." OFF)
option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GCC/Clang only)." OFF)
option(LOAD_PLUGINS_FROM_PARENT "Force loading of plugin libraries from parent folder." OFF)
option(REVISION_FROM_GIT "Get revision using `git rev-parse`." ON)
option(NO_UPDATE_CHECK "Disable automatic checking for new application updates." OFF)
option(IS_FLATPAK_BUILD "Set to 'ON' when building RSS Guard with Flatpak." OFF)
option(FORCE_BUNDLE_ICONS "Forcibly bundle icon themes into RSS Guard." OFF)
option(ENABLE_COMPRESSED_SITEMAP "Enable support for gzip-compressed sitemap feeds. Requires zlib." OFF)
option(ENABLE_MEDIAPLAYER_QTMULTIMEDIA "Enable built-in media player. Requires QtMultimedia FFMPEG plugin." OFF)
option(ENABLE_MEDIAPLAYER_LIBMPV "Enable built-in media player. Requires libmpv library." ON)
option(MEDIAPLAYER_FORCE_OPENGL "Use opengl-based render API with libmpv." ON)

# Import Qt libraries.
set(QT6_MIN_VERSION 6.3.0)
set(QT5_MIN_VERSION 5.14.0)

set(QT_COMPONENTS
  Core
  Gui
  LinguistTools
  Network
  Qml
  Sql
  Widgets
  Xml
  Concurrent
)

if(WIN32 AND NOT BUILD_WITH_QT6)
  list(APPEND QT_COMPONENTS WinExtras)
endif()

if(NOT OS2)
  list(APPEND QT_COMPONENTS Multimedia)
endif()

if(MINGW AND NO_LITE)
  message(WARNING "MinGW can only compile lite version of RSS Guard. Switching to it.")
  set(NO_LITE OFF)
endif()

if(FORCE_COLORED_OUTPUT)
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
  endif()
endif()

if(ENABLE_MEDIAPLAYER_QTMULTIMEDIA AND ENABLE_MEDIAPLAYER_LIBMPV)
  message(FATAL_ERROR "You can only enable 1 media player backend.")
endif()

if(ENABLE_MEDIAPLAYER_QTMULTIMEDIA)
  message(STATUS "Enabling QtMultimedia media player backend.")

  list(APPEND QT_COMPONENTS OpenGL MultimediaWidgets)
  add_compile_definitions(ENABLE_MEDIAPLAYER_QTMULTIMEDIA)
endif()

if(ENABLE_MEDIAPLAYER_LIBMPV)
  message(STATUS "Enabling libmpv media player backend.")

  if(WIN32 AND NOT LibMPV_ROOT)
    set(LibMPV_ROOT "${CMAKE_SOURCE_DIR}/resources/scripts/libmpv")
  endif()

  if(MEDIAPLAYER_FORCE_OPENGL)
    message(STATUS "Forcing OpenGL-based rendering for libmpv.")

    list(APPEND QT_COMPONENTS OpenGL)

    if(BUILD_WITH_QT6)
      list(APPEND QT_COMPONENTS OpenGLWidgets)
    endif()

    add_compile_definitions(MEDIAPLAYER_LIBMPV_OPENGL)
  endif()

  add_compile_definitions(ENABLE_MEDIAPLAYER_LIBMPV)
endif()

if(ENABLE_MEDIAPLAYER_QTMULTIMEDIA OR ENABLE_MEDIAPLAYER_LIBMPV)
  set(ENABLE_MEDIAPLAYER TRUE)
  add_compile_definitions(ENABLE_MEDIAPLAYER)
else()
  message(STATUS "Media player feature is disabled.")
endif()

if(NO_LITE)
  list(APPEND QT_COMPONENTS WebEngineWidgets)
  add_compile_definitions(NO_LITE)
endif()

if(UNIX AND NOT APPLE AND NOT ANDROID)
  list(APPEND QT_COMPONENTS DBus)
endif()

if(BUILD_WITH_QT6)
  find_package(QT NAMES Qt6)
  find_package(Qt6 ${QT6_MIN_VERSION} COMPONENTS ${QT_COMPONENTS} Core5Compat REQUIRED)
else()
  find_package(QT NAMES Qt5)
  find_package(Qt5 ${QT5_MIN_VERSION} COMPONENTS ${QT_COMPONENTS} REQUIRED)

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

# Load git commit hash.
if(REVISION_FROM_GIT AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(COMMAND "git" "rev-parse" "--short" "HEAD"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE APP_REVISION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  message(STATUS "Detected git revision: '${APP_REVISION}'.")
endif()

if(NOT NO_LITE)
  set(APP_REVERSE_NAME "io.github.martinrotter.rssguardlite")
  set(APP_REVISION "${APP_REVISION}-lite")
endif()

set(APP_LOW_NAME "${CMAKE_PROJECT_NAME}")

# Pass common defines.
add_compile_definitions(
  APP_NAME="${APP_NAME}"
  APP_VERSION="${CMAKE_PROJECT_VERSION}"
  APP_URL="${APP_URL}"
  APP_LONG_NAME="${APP_NAME} ${CMAKE_PROJECT_VERSION}"
  APP_LOW_NAME="${APP_LOW_NAME}"
  APP_REVERSE_NAME="${APP_REVERSE_NAME}"
  APP_AUTHOR="${APP_AUTHOR}"
  APP_DONATE_URL="${APP_DONATE_URL}"
  APP_EMAIL="${APP_EMAIL}"
  APP_LOW_H_NAME=".${CMAKE_PROJECT_NAME}"
  APP_REVISION="${APP_REVISION}"
  APP_SYSTEM_NAME="${CMAKE_SYSTEM_NAME}"
  APP_SYSTEM_VERSION="${CMAKE_SYSTEM_PROCESSOR}"
  APP_URL_DOCUMENTATION="${APP_URL_DOCUMENTATION}"
  APP_URL_ISSUES_NEW="${APP_URL_ISSUES_NEW}"
  APP_USERAGENT="${APP_NAME}/${CMAKE_PROJECT_VERSION}"

  QT_USE_QSTRINGBUILDER
  QT_USE_FAST_CONCATENATION
  QT_USE_FAST_OPERATOR_PLUS
  UNICODE
  _UNICODE
)

if(NO_UPDATE_CHECK)
  add_compile_definitions(NO_UPDATE_CHECK)
endif()

if(IS_FLATPAK_BUILD)
  add_compile_definitions(IS_FLATPAK_BUILD)
endif()

if(BUILD_MSYS2)
  add_compile_definitions(BUILD_MSYS2)
endif()

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
elseif(UNIX AND NOT ANDROID)
  add_subdirectory(resources/desktop)
  add_compile_definitions(
    APPDATA_NAME="${APPDATA_NAME}"
    APPDATA_SUMMARY="${APPDATA_SUMMARY}"
  )
endif()

# Common libraries.
add_subdirectory(src/librssguard)
add_subdirectory(localization)

# Plugins.
add_subdirectory(src/librssguard-standard)
add_subdirectory(src/librssguard-feedly)
add_subdirectory(src/librssguard-gmail)
add_subdirectory(src/librssguard-greader)
add_subdirectory(src/librssguard-ttrss)
add_subdirectory(src/librssguard-reddit)
add_subdirectory(src/librssguard-nextcloud)

# GUI executable.
add_subdirectory(src/rssguard)
