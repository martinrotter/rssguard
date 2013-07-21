cmake_minimum_required(VERSION 2.8.11)

# Setup basic variables.
project(rssguard)
set(APP_NAME "RSS Guard")
set(APP_UP_NAME "RSSGuard")
set(APP_LOW_NAME "rssguard")
set(APP_VERSION "2.0.0-prealpha-1")
set(APP_AUTHOR "Martin Rotter")
set(APP_URL "http://rssguard.sf.net")

message(STATUS "[${APP_LOW_NAME}] Welcome to ${APP_NAME} compilation process.")
message(STATUS "[${APP_LOW_NAME}] Compilation process begins right now.")

# Setup name for executable file.
if(APPLE)
  set(EXE_NAME ${APP_UP_NAME})
else(APPLE)
  set(EXE_NAME ${APP_LOW_NAME})
endif(APPLE)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

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
    set(APP_REVISION UNKNOWN-REVISION)
  endif(GIT_FOUND)
else(EXISTS "${PROJECT_SOURCE_DIR}/.git")
  set(APP_REVISION SEPARATE-BUILD)
endif(EXISTS "${PROJECT_SOURCE_DIR}/.git")

message(STATUS "[${APP_LOW_NAME}] Revision number obtained: " ${APP_REVISION} ".")

# Configure executable "properties" for Windows.
if(WIN32)
  message(STATUS "[${APP_LOW_NAME}] Generating executable file properties.")
  configure_file (
    ${PROJECT_SOURCE_DIR}/resources/executable_properties/rssguard_win.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/resources/executable_properties/rssguard_win.rc
  )
endif(WIN32)

# Configure desktop entry for Linux.
if(UNIX)
  message(STATUS "[${APP_LOW_NAME}] Generating desktop entry.")
  configure_file (
    ${PROJECT_SOURCE_DIR}/resources/desktop/rssguard.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/resources/desktop/rssguard.desktop
  )
  set(DESKTOP_ENTRY ${CMAKE_INSTALL_PREFIX}/share/applications)
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
  message(STATUS "[${APP_LOW_NAME}] A debug build is chosen. -DDEBUG is defined.")
  add_definitions(-DDEBUG)
  set(CMAKE_BUILD_TYPE Debug)

# Enable compiler warnings.
# See http://stackoverflow.com/questions/2368811/how-to-set-warning-level-in-cmake
message(STATUS "[${APP_LOW_NAME}] Enabling verbose makefile and full warning reports.")
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-pedantic -Wall -Wextra)
elseif(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
  add_definitions(/W2)
endif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

# Verbose compiling ouputs.
set(CMAKE_VERBOSE_MAKEFILE ON)

endif(CMAKE_BUILD_TYPE STREQUAL "release" OR CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RELEASE")

# Force Qt to use string builders.
add_definitions(-DQT_USE_QSTRINGBUILDER)

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

# Unicode settings.
add_definitions(-DUNICODE -D_UNICODE)

# Find all needed Qt modules.
find_package(Qt5Sql)
find_package(Qt5WebKit)
find_package(Qt5WebKitWidgets)
find_package(Qt5Widgets)
find_package(Qt5Xml)
find_package(Qt5Network)
find_package(Qt5LinguistTools)

# Compile application icon if compiling with MinGW on WIN32.
if(MINGW AND WIN32)
  set(WINDRES windres.exe)
  message(STATUS "[${APP_LOW_NAME}] MinGW compilation is selected. Icon file binary will be builded.")
  message(STATUS "[${APP_LOW_NAME}] Used windres tool is: " ${WINDRES} ".")
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources/executable_properties/rssguard_win.o
    COMMAND ${WINDRES}  -I ${CMAKE_CURRENT_BINARY_DIR}
                        -i ${CMAKE_CURRENT_BINARY_DIR}/resources/executable_properties/rssguard_win.rc
                        -o ${CMAKE_CURRENT_BINARY_DIR}/resources/executable_properties/rssguard_win.o
    )
    set(APP_SOURCES ${APP_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources/executable_properties/rssguard_win.o)

# MSVC takes care of this automatically no need to use windres.exe
# for MSVC compilers.
elseif(WIN32)
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
  src/gui/themefactory.cpp
  src/gui/formsettings.cpp
  src/gui/formwelcome.cpp
  src/gui/formabout.cpp
  src/gui/shortcutcatcher.cpp
  src/gui/shortcutbutton.cpp
  src/gui/dynamicshortcutswidget.cpp
  src/gui/webbrowser.cpp
  src/gui/basewebview.cpp
  src/gui/baselineedit.cpp
  src/gui/locationlineedit.cpp

  # CORE sources.
  src/core/debugging.cpp
  src/core/settings.cpp
  src/core/systemfactory.cpp
  src/core/datetime.cpp
  src/core/localization.cpp
  src/core/dynamicshortcuts.cpp
  src/core/basenetworkaccessmanager.cpp
  src/core/basewebpage.cpp
  src/core/webbrowsernetworkaccessmanager.cpp

  # Basic application sources.
  src/main.cpp
)

# Add headers.
set(APP_HEADERS
  ${APP_HEADERS}

  # QtSingleApplication suite headers.
  src/qtsingleapplication/qtlocalpeer.h
  src/qtsingleapplication/qtlockedfile.h
  src/qtsingleapplication/qtsingleapplication.h

  # GUI headers.
  src/gui/formmain.h
  src/gui/systemtrayicon.h
  src/gui/formsettings.h
  src/gui/formwelcome.h
  src/gui/formabout.h
  src/gui/shortcutcatcher.h
  src/gui/shortcutbutton.h
  src/gui/dynamicshortcutswidget.h
  src/gui/webbrowser.h
  src/gui/basewebview.h
  src/gui/baselineedit.h
  src/gui/locationlineedit.h

  # CORE headers.
  src/core/basenetworkaccessmanager.h
  src/core/webBrowsernetworkaccessmanager.h
  src/core/basewebpage.h
)

# Add form files.
set(APP_FORMS
  src/gui/formmain.ui
  src/gui/formsettings.ui
  src/gui/formwelcome.ui
  src/gui/formabout.ui
)

# Add resources.
set(APP_RESOURCES
  resources/rssguard_resources.qrc
)

# Add translations.
set(APP_TRANSLATIONS
  localization/rssguard_en.ts
  localization/rssguard_cs.ts
  localization/qt_cs.ts
)

set(APP_TRANSLATIONS_WO_QT
  localization/rssguard_en.ts
  localization/rssguard_cs.ts
)

# Add skins.
set(APP_SKIN_PLAIN
  # resources/skins/base/plain.qss
)

set(APP_MISC
  resources/text/CHANGELOG
  resources/text/COPYING_GNU_GPL
  resources/text/COPYING_GNU_GPL_HTML
  resources/text/COPYING_BSD
)

# Wrap files, create moc files.
qt5_wrap_cpp(APP_MOC ${APP_HEADERS})
qt5_wrap_ui(APP_UI ${APP_FORMS})
qt5_add_resources(APP_RCC ${APP_RESOURCES})

if(Qt5LinguistTools_FOUND)
  message(STATUS "[${APP_LOW_NAME}] Qt Linguist Tools found. Translations will get refreshed.")
  qt5_add_translation(APP_QM ${APP_TRANSLATIONS})
else(Qt5LinguistTools_FOUND)
  message(STATUS "[${APP_LOW_NAME}] Qt Linguist Tools NOT found. No refreshing for translations.")
endif(Qt5LinguistTools_FOUND)

# Include additional directory paths.
include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/src
)

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
  WebKit
  WebKitWidgets
)

# Installation stage.
if(WIN32)
  message(STATUS "[${APP_LOW_NAME}] You will probably install on Windows.")
  install(TARGETS ${EXE_NAME}
          RUNTIME DESTINATION ./)
  install(DIRECTORY resources/graphics/themes/mini-kfaenza
          DESTINATION ./themes)
  install(DIRECTORY resources/graphics/flags
          DESTINATION ./)
  install(FILES resources/graphics/${APP_LOW_NAME}_128.png
          DESTINATION ./
          RENAME ${APP_LOW_NAME}.png)
  install(FILES resources/graphics/${APP_LOW_NAME}_plain_128.png
          DESTINATION ./
          RENAME ${APP_LOW_NAME}_plain.png)
  install(FILES ${APP_QM}
          DESTINATION ./l10n)
  install(FILES ${APP_MISC}
          DESTINATION ./)
elseif(UNIX)
  message(STATUS "[${APP_LOW_NAME}] You will probably install on Linux.")
  install(TARGETS ${EXE_NAME}
          RUNTIME DESTINATION bin)
  install(DIRECTORY resources/graphics/themes/mini-kfaenza
          DESTINATION share/${APP_LOW_NAME}/themes)
  install(DIRECTORY resources/graphics/flags
          DESTINATION share/${APP_LOW_NAME}/)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources/desktop/${APP_LOW_NAME}.desktop
          DESTINATION share/applications)
  install(FILES resources/graphics/${APP_LOW_NAME}_128.png
          DESTINATION share/icons/hicolor/128x128/apps/
          RENAME ${APP_LOW_NAME}.png)
  install(FILES resources/graphics/${APP_LOW_NAME}_plain_128.png
          DESTINATION share/icons/hicolor/128x128/apps/
          RENAME ${APP_LOW_NAME}_plain.png)
  install(FILES ${APP_QM}
          DESTINATION share/${APP_LOW_NAME}/l10n)
  install(FILES ${APP_MISC}
          DESTINATION share/${APP_LOW_NAME}/information)
endif(WIN32)

# Custom target for packaging.
set(CPACK_PACKAGE_NAME ${APP_LOW_NAME})
set(CPACK_PACKAGE_VERSION ${APP_VERSION})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-src")
set(CPACK_IGNORE_FILES "/resources/aur/;\\\\.psd$;/resources/deployment;/CVS/;/\\\\.svn/;/\\\\.git/;\\\\.swp$;/CMakeLists.txt.user;\\\\.#;/#;\\\\.tar.gz$;/CMakeFiles/;CMakeCache.txt;\\\\.qm$;/build/;\\\\.diff$;.DS_Store'")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})

# Load packaging facilities.
include(CPack)

# make dist implementation.
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# make lupdate and make lrelease implementation.
add_custom_target(lupdate
  ${Qt5Core_QMAKE_EXECUTABLE} -project -o ${CMAKE_CURRENT_BINARY_DIR}/${APP_LOW_NAME}.pro
    COMMAND ${Qt5_LUPDATE_EXECUTABLE} -ts ${APP_TRANSLATIONS_WO_QT} -no-obsolete -pro ${CMAKE_CURRENT_BINARY_DIR}/${APP_LOW_NAME}.pro
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
