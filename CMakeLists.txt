#################################################################
#
# RSS Guard compilation script for CMake.
#
# Usage:
#   a) DEBUG build for testing.
#     cmake -DCMAKE_BUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX=/usr -DUSE_QT_5=ON
#
#   b) RELEASE build for production use.
#     cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_INSTALL_PREFIX=/usr -DUSE_QT_5=ON
#
# Arguments:
#   "-DCMAKE_INSTALL_PREFIX=/usr"
#     Installation path, replace with something like "C:\rssguard" on Windows.
#
#   "-DUSE_QT_5=OFF"
#     Specifies which major Qt version to use. Qt 4 and Qt 5 are supported.
#     If "OFF" is passed as an argument, then Qt 4 is used. Default is "OFF".
#
#   "-DBUNDLE_ICON_THEMES=ON"
#     If "ON", then custom icons theme(s) will be bundled with application
#     installation. If "OFF", then no icon theme(s) will be available for
#     the application and application will run in iconless mode.
#     Default and recommended value is "ON".
#
# Other information:
#   - supports Windows, Linux, OS/2 (eComStation),
#   - Qt 4.7.3 and higher is required,
#   - resource compiler (windres.exe) is needed on MinGW,
#   - resource compiler (rc.exe) is needed on OS/2.
#
# Authors and contributors:
#   - Martin Rotter (project leader)
#   - Elbert Pol (huge OS/2-related contributions)
#
#################################################################


# Setup basic variables.
project(rssguard)
set(APP_NAME "RSS Guard")
set(APP_LOW_NAME "rssguard")
set(APP_VERSION "1.9.9.4")
set(FILE_VERSION "1,9,9,4")
set(APP_AUTHOR "Martin Rotter")
set(APP_URL "http://www.rssguard.comehere.cz")
set(APP_URL_ISSUES "http://github.com/martinrotter/rssguard/issues")
set(APP_EMAIL "rotter.martinos@gmail.com")
set(MINIMUM_QT_VERSION 4.7.3)
set(EXE_NAME ${APP_LOW_NAME})

# Options declaration.
option(USE_QT_5 "Use Qt 5 for building" OFF)
option(BUNDLE_ICON_THEMES "Equip installation with custom icon themes" ON)

if(POLICY CMP0012)
  cmake_policy(SET CMP0012 NEW)
endif()

if(${USE_QT_5})
  cmake_minimum_required(VERSION 2.8.10)
else(${USE_QT_5})
  cmake_minimum_required(VERSION 2.8.0)
endif(${USE_QT_5})

message(STATUS "[${APP_LOW_NAME}] Welcome to ${APP_NAME} compilation process.")
message(STATUS "[${APP_LOW_NAME}] Compilation process begins right now.")

if(BUNDLE_ICON_THEMES)
  message(STATUS "[${APP_LOW_NAME}] Custom icon themes will be installed.")
else(BUNDLE_ICON_THEMES)
  message(STATUS "[${APP_LOW_NAME}] Custom icon themes will NOT be installed.")
endif(BUNDLE_ICON_THEMES)

# Setup misc options.
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/resources/cmake ${CMAKE_MODULE_PATH})
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
    # TODO: https://wiki.archlinux.org/index.php/VCS_PKGBUILD_Guidelines#Git
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_REVISION
    )
    string(REGEX REPLACE "\n" "" GIT_REVISION "${GIT_REVISION}")
    set(APP_REVISION ${GIT_REVISION})
  else(GIT_FOUND)
    set(APP_REVISION -)
  endif(GIT_FOUND)
else(EXISTS "${PROJECT_SOURCE_DIR}/.git")
  set(APP_REVISION -)
endif(EXISTS "${PROJECT_SOURCE_DIR}/.git")

message(STATUS "[${APP_LOW_NAME}] Revision number obtained: " ${APP_REVISION} ".")

# Configure desktop entry for Linux.
if(UNIX)
  message(STATUS "[${APP_LOW_NAME}] Generating desktop entry for Linux.")
  configure_file (
    ${PROJECT_SOURCE_DIR}/resources/desktop/rssguard.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/resources/desktop/rssguard.desktop
  )
  set(DESKTOP_ENTRY_PATH ${CMAKE_INSTALL_PREFIX}/share/applications)
endif(UNIX)

# Configure internal C++ defines.
configure_file (
  ${PROJECT_SOURCE_DIR}/src/core/defs.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/core/defs.h
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
elseif(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
  #add_definitions(/W3)
endif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

# Verbose compiling outputs.
set(CMAKE_VERBOSE_MAKEFILE ON)

endif(CMAKE_BUILD_TYPE STREQUAL "release" OR CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RELEASE")

# Force Qt to use string builders.
add_definitions(-DQT_USE_QSTRINGBUILDER)

# Unicode settings.
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

# Setup libraries.
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
    ${CMAKE_CURRENT_BINARY_DIR}/resources/executable_properties/rssguard_win.rc
  )
elseif(OS2)
  message(STATUS "[${APP_LOW_NAME}] Generating executable file properties for OS2.")
  configure_file (
    ${PROJECT_SOURCE_DIR}/resources/executable_properties/rssguard_os2.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/resources/executable_properties/rssguard_os2.rc
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
      ${CMAKE_CURRENT_BINARY_DIR}/resources/executable_properties/rssguard_win.rc)
elseif(OS2)
  enable_language(RC)
  set(CMAKE_RC_COMPILER_INIT rc)
  message(STATUS "[${APP_LOW_NAME}] Icon file binary will be builded for OS2.")
  message(STATUS "[${APP_LOW_NAME}] Used tool is: ${CMAKE_RC_COMPILER}")
  set(CMAKE_RC_COMPILE_OBJECT
      "<CMAKE_RC_COMPILER> -n -r <SOURCE> <OBJECT>")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Zstack 0x2000 -Zlinker \"DISABLE 1121\" -s -Zlinker /PM:PM -Zno-fork -Zhigh-mem -Zmap")

  set(APP_SOURCES
      ${APP_SOURCES}
      ${CMAKE_CURRENT_BINARY_DIR}/resources/executable_properties/rssguard_os2.rc)
elseif(WIN32 AND MSVC)
  # MSVC takes care of this automatically - no need to use windres.exe
  # for MSVC compilers.
  set(APP_SOURCES ${APP_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources/executable_properties/rssguard_win.rc)
endif(MINGW AND WIN32)

# Add source files.
set(APP_SOURCES
  ${APP_SOURCES}

  # QtSingleApplication suite sources.
  src/qtsingleapplication/qtlocalpeer.cpp
  src/qtsingleapplication/qtsingleapplication.cpp

  # GUI sources.
  src/gui/formmain.cpp
  src/gui/systemtrayicon.cpp
  src/gui/iconthemefactory.cpp
  src/gui/skinfactory.cpp
  src/gui/formsettings.cpp
  src/gui/formwelcome.cpp
  src/gui/formabout.cpp
  src/gui/shortcutcatcher.cpp
  src/gui/shortcutbutton.cpp
  src/gui/dynamicshortcutswidget.cpp
  src/gui/webbrowser.cpp
  src/gui/webview.cpp
  src/gui/baselineedit.cpp
  src/gui/locationlineedit.cpp
  src/gui/tabwidget.cpp
  src/gui/tabbar.cpp
  src/gui/tabcontent.cpp
  src/gui/cornerbutton.cpp
  src/gui/feedmessageviewer.cpp
  src/gui/feedsview.cpp
  src/gui/messagesview.cpp
  src/gui/statusbar.cpp
  src/gui/iconfactory.cpp
  src/gui/formstandardcategorydetails.cpp
  src/gui/formstandardfeeddetails.cpp
  src/gui/plaintoolbutton.cpp
  src/gui/lineeditwithstatus.cpp
  src/gui/messagebox.cpp

  # CORE sources.
  src/core/debugging.cpp
  src/core/settings.cpp
  src/core/systemfactory.cpp
  src/core/localization.cpp
  src/core/dynamicshortcuts.cpp
  src/core/basenetworkaccessmanager.cpp
  src/core/webpage.cpp
  src/core/webbrowsernetworkaccessmanager.cpp
  src/core/silentnetworkaccessmanager.cpp
  src/core/textfactory.cpp
  src/core/databasefactory.cpp
  src/core/messagesmodel.cpp
  src/core/messagesproxymodel.cpp
  src/core/feedsmodel.cpp
  src/core/feedsproxymodel.cpp
  src/core/feedsmodelcategory.cpp
  src/core/feedsmodelrootitem.cpp
  src/core/feedsmodelfeed.cpp
  src/core/feedsmodelstandardcategory.cpp
  src/core/feedsmodelstandardfeed.cpp
  src/core/parsingfactory.cpp
  src/core/feeddownloader.cpp
  src/core/networkfactory.cpp

  # Basic application sources.
  src/main.cpp
)

# Add headers.
set(APP_HEADERS
  ${APP_HEADERS}

  # QtSingleApplication suite headers.
  src/qtsingleapplication/qtlocalpeer.h
  src/qtsingleapplication/qtsingleapplication.h

  # GUI headers.
  src/gui/formmain.h
  src/gui/systemtrayicon.h
  src/gui/iconthemefactory.h
  src/gui/skinfactory.h
  src/gui/formsettings.h
  src/gui/formwelcome.h
  src/gui/formabout.h
  src/gui/shortcutcatcher.h
  src/gui/shortcutbutton.h
  src/gui/dynamicshortcutswidget.h
  src/gui/webbrowser.h
  src/gui/webview.h
  src/gui/baselineedit.h
  src/gui/locationlineedit.h
  src/gui/tabwidget.h
  src/gui/tabbar.h
  src/gui/tabcontent.h
  src/gui/cornerbutton.h
  src/gui/feedmessageviewer.h
  src/gui/feedsview.h
  src/gui/messagesview.h
  src/gui/statusbar.h
  src/gui/formstandardcategorydetails.h
  src/gui/formstandardfeeddetails.h
  src/gui/plaintoolbutton.h
  src/gui/lineeditwithstatus.h
  src/gui/messagebox.h

  # CORE headers.
  src/core/settings.h
  src/core/basenetworkaccessmanager.h
  src/core/webbrowsernetworkaccessmanager.h
  src/core/silentnetworkaccessmanager.h
  src/core/webpage.h
  src/core/systemfactory.h
  src/core/databasefactory.h
  src/core/messagesmodel.h
  src/core/messagesproxymodel.h
  src/core/feedsmodel.h
  src/core/feedsproxymodel.h
  src/core/feeddownloader.h
)

# Add form files.
set(APP_FORMS
  src/gui/formmain.ui
  src/gui/formsettings.ui
  src/gui/formwelcome.ui
  src/gui/formabout.ui
  src/gui/formstandardcategorydetails.ui
  src/gui/formstandardfeeddetails.ui
)

# Add translations.
set(APP_TRANSLATIONS
  localization/rssguard_en.ts
  localization/rssguard_cs.ts
  localization/rssguard_nl.ts
  localization/qt_cs.ts
  localization/qt_nl.ts
)

set(APP_TRANSLATIONS_WO_QT
  localization/rssguard_en.ts
  localization/rssguard_cs.ts
  localization/rssguard_nl.ts
)

set(APP_TEXT
  resources/text/CHANGELOG
  resources/text/COPYING_GNU_GPL
  resources/text/COPYING_GNU_GPL_HTML
  resources/text/COPYING_BSD
  resources/text/AUTHORS
)

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
endif(${USE_QT_5})

# Include additional directory paths.
include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/src
)

# Setup compilation for Qt 5.
if(${USE_QT_5})
  if(WIN32)
    add_executable(${EXE_NAME} WIN32
      ${APP_SOURCES}
      ${APP_FORMS}
      ${APP_RCC}
      ${APP_QM}
    )
    target_link_libraries(${EXE_NAME} Qt5::WinMain)
  else(WIN32)
    add_executable(${EXE_NAME}
      ${APP_SOURCES}
      ${APP_FORMS}
      ${APP_RCC}
      ${APP_QM}
    )
  endif(WIN32)
  
  # Use modules from Qt.
  qt5_use_modules(${EXE_NAME}
    Core
    Widgets
    Sql
    Network
    Xml
    #XmlPatterns
    WebKit
    WebKitWidgets
  )
# Setup compilation for Qt 4.
else(${USE_QT_5})
  add_executable(${EXE_NAME} WIN32
    ${APP_SOURCES}
    ${APP_FORMS}
    ${APP_MOC}
    ${APP_RCC}
    ${APP_QM}
  )

  # Link modules from Qt.
  target_link_libraries( ${EXE_NAME}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTSQL_LIBRARY}
    ${QT_QTXML_LIBRARY}
    #${QT_QTXMLPATTERNS_LIBRARY}
    ${QT_QTMAIN_LIBRARY}
    ${QT_QTWEBKIT_LIBRARY}
  )
endif(${USE_QT_5})

# Installation stage.
if(WIN32 OR OS2)
  message(STATUS "[${APP_LOW_NAME}] You will probably install on Windows or OS/2.")
  install(TARGETS ${EXE_NAME}
          RUNTIME DESTINATION ./)

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
          DESTINATION share/${APP_LOW_NAME}/)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources/desktop/${APP_LOW_NAME}.desktop
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
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME ${APP_LOW_NAME})
set(CPACK_PACKAGE_VERSION ${APP_VERSION})
set(CPACK_IGNORE_FILES "/resources/aur/;\\\\.psd$;/resources/deployment;/CVS/;/\\\\.svn/;/\\\\.git/;\\\\.swp$;/CMakeLists.txt.user;\\\\.#;/#;\\\\.tar.gz$;/CMakeFiles/;CMakeCache.txt;\\\\.qm$;/build/;\\\\.diff$;.DS_Store'")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})

# Load packaging facilities.
include(CPack)

# make dist implementation.
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# make lupdate implementation.
if(${USE_QT_5})
  add_custom_target(lupdate
    ${Qt5Core_QMAKE_EXECUTABLE} -project -o ${CMAKE_CURRENT_BINARY_DIR}/${APP_LOW_NAME}.pro
      COMMAND ${Qt5_LUPDATE_EXECUTABLE} -no-obsolete -ts ${APP_TRANSLATIONS_WO_QT} -pro ${CMAKE_CURRENT_BINARY_DIR}/${APP_LOW_NAME}.pro
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
else(${USE_QT_5})
  add_custom_target(lupdate
    ${QT_QMAKE_EXECUTABLE} -project -o ${CMAKE_CURRENT_BINARY_DIR}/${APP_LOW_NAME}.pro
      COMMAND ${QT_LUPDATE_EXECUTABLE} -no-obsolete -ts ${APP_TRANSLATIONS_WO_QT} -pro ${CMAKE_CURRENT_BINARY_DIR}/${APP_LOW_NAME}.pro
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif(${USE_QT_5})
