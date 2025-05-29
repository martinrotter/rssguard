# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.7)
project(qlitehtml VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(QLITEHTML_EXPORT qlitehtml)
set(QLITEHTML_DEVEL_COMPONENT Devel)
set(QLITEHTML_DEVEL_EXCLUDE_FROM_ALL NO)

if(UNIX OR MINGW)
    include(GNUInstallDirs)
    set(QLITEHTML_BIN_PATH ${CMAKE_INSTALL_BINDIR})
    set(QLITEHTML_LIBRARY_PATH  ${CMAKE_INSTALL_LIBDIR})
    set(QLITEHTML_HEADER_PATH ${CMAKE_INSTALL_INCLUDEDIR}/qlitehtml)
else()
    set(QLITEHTML_BIN_PATH bin)
    set(QLITEHTML_LIBRARY_PATH lib)
    set(QLITEHTML_HEADER_PATH include/qlitehtml)
endif()

find_package(QT 5.15 NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
set(Qt Qt${QT_VERSION_MAJOR})
if(QT_VERSION_MAJOR VERSION_GREATER_EQUAL "6")
    find_package(${Qt} ${Qt${QT_VERSION_MAJOR}_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS
        PrintSupport)
endif()

add_subdirectory(src)
add_subdirectory(tests/manual/browser)
