cmake_minimum_required(VERSION 2.8.11)

# Setup basic variables.
project(rssguard)
set(APP_NAME "RSS Guard")
set(APP_LOW_NAME "rssguard")
set(APP_VERSION "2.0.0-prealpha-1")
set(APP_AUTHOR "Martin Rotter")
set(APP_URL "http://rssguard.sf.net")

message(STATUS "[rssguard] Welcome to RSS Guard compilation process.")
message(STATUS "[rssguard] Compilation process begins right now.")

# Setup name for executable file.
if(APPLE)
  set(EXE_NAME "RSSGuard")
else(APPLE)
  set(EXE_NAME ${APP_LOW_NAME})
endif(APPLE)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Obtain revision number if available.
message(STATUS "[rssguard] Obtaining revision number.")

if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      OUTPUT_VARIABLE _git_revision
    )
    string(REGEX REPLACE "\n" "" _git_revision "${_git_revision}")
    set(APP_REVISION ${_git_revision})
  else(GIT_FOUND)
    set(APP_REVISION UNKNOWN-REVISION)
  endif(GIT_FOUND)
else(EXISTS "${PROJECT_SOURCE_DIR}/.git")
  set(APP_REVISION STABLE)
endif(EXISTS "${PROJECT_SOURCE_DIR}/.git")

message(STATUS "[rssguard] Revision number obtained: " ${APP_REVISION} ".")

# Configure internal C++ defines.
configure_file (
  ${PROJECT_SOURCE_DIR}/src/defs.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/defs.h
)

# Configure executable "properties" for Windows.
if(WIN32)
configure_file (
 ${PROJECT_SOURCE_DIR}/resources/executable_properties/rssguard_win.rc.in
 ${CMAKE_CURRENT_BINARY_DIR}/resources/rssguard_win.rc
)
endif(WIN32)

# Configure desktop entry for Linux.
if(UNIX)
  message(STATUS "[rssguard] Generating desktop entry.")
  # configure_file (
    # ${PROJECT_SOURCE_DIR}/resources/desktop/qonverter.desktop.in
    # ${CMAKE_CURRENT_BINARY_DIR}/resources/desktop/qonverter.desktop
  # )
endif(UNIX)

# Define some useful DEBUG for, ehrm, debug build.
if(CMAKE_BUILD_TYPE STREQUAL "release" OR CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RELEASE")
  message(STATUS "[rssguard] A release build (non-debug) is chosen. Debugging outputs are silently ignored.")
  set(CMAKE_BUILD_TYPE MinSizeRel)
else(CMAKE_BUILD_TYPE STREQUAL "release" OR CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RELEASE")
  message(STATUS "[rssguard] A debug build is chosen. -DDEBUG is defined")
  add_definitions(-DDEBUG)
  set(CMAKE_BUILD_TYPE Debug)

# Enable compiler warnings.
# See http://stackoverflow.com/questions/2368811/how-to-set-warning-level-in-cmake
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-pedantic -Wall -Wextra)
elseif(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
  add_definitions(/W2)
endif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

# Verbose compiling ouputs.
set(CMAKE_VERBOSE_MAKEFILE ON)

endif(CMAKE_BUILD_TYPE STREQUAL "release" OR CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RELEASE")

# Check for C++ 11 features availability.
# See http://stackoverflow.com/questions/10984442/how-to-detect-c11-support-of-a-compiler-with-cmake
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION
  )

  if(NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
    message(FATAL_ERROR "[rssguard] Your C++ compiler does not support C++ 11.")
  else(NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
    add_definitions(-std=c++11)
  endif(NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))

elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
elseif(${MSVC_VERSION} VERSION_LESS 1600)
  message(FATAL_ERROR "[rssguard] Your C++ compiler does not support C++ 11.")
endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")

# Unicode settings.
add_definitions(-DUNICODE -D_UNICODE)

# Find all needed Qt modules.
find_package(Qt5Sql)
find_package(Qt5Widgets)
find_package(Qt5Xml)
find_package(Qt5Network)
find_package(Qt5LinguistTools)

# Compile application icon if compiling with MinGW on WIN32.
if(MINGW AND WIN32)
  set(WINDRES windres.exe)
  message(STATUS "[rssguard] MinGW compilation is selected. Icon file binary will be builded.")
  message(STATUS "[rssguard] Used windres tool is: " ${WINDRES} ".")
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/qonverter.o
    COMMAND ${WINDRES}  -I ${CMAKE_CURRENT_BINARY_DIR}
                        -i ${CMAKE_CURRENT_BINARY_DIR}/resources/qonverter_win.rc
                        -o ${CMAKE_CURRENT_BINARY_DIR}/qonverter.o
    )
    set(APP_SOURCES ${APP_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/qonverter.o)

# MSVC takes care of this automatically no need to use windres.exe
# for MSVC compilers.
elseif(WIN32)
  set(APP_SOURCES ${APP_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources/qonverter_win.rc)
endif(MINGW AND WIN32)

# Add source files.
set(APP_SOURCES
  ${APP_SOURCES}
  src/main.cpp
)

# Add headers.
set(APP_HEADERS
  ${APP_HEADERS}
)

# Add form files.
set(APP_FORMS
  # ui/formmain.ui
)

# Add resources.
set(APP_RESOURCES
 resources/rssguard_resources.qrc
)

# Add translations.
set(APP_TRANSLATIONS
  # localization/rssguard_en.ts
  # localization/rssguard_cs.ts
  # localization/qt_cs.ts
)

set(APP_TRANSLATIONS_WO_QT
  # localization/rssguard_en.ts
  # localization/rssguard_cs.ts
)

# Add skins.
set(APP_SKIN_PLAIN
  # resources/skins/base/plain.qss
)

set(APP_SKIN_DARK
  # resources/skins/fancy/dark.qss
)

set(APP_SKIN_DARK_IMAGES
  # resources/skins/fancy/images/down_arrow.png
  # resources/skins/fancy/images/handle.png
  # resources/skins/fancy/images/checkbox.png
)

# Add needed text files.
set(APP_MISC
  # resources/text/COPYING_GNU_GPL_HTML
  # resources/text/AUTHORS
  # resources/text/COPYING_GNU_GPL
  # resources/text/COPYING_BSD
  # resources/text/COPYING_FLAGS
  # resources/text/COPYING_GNU_LGPL_HTML
  # resources/text/CHANGELOG
)

# Wrap files, create moc files.
qt5_wrap_cpp(APP_MOC ${APP_HEADERS})
qt5_wrap_ui(APP_UI ${APP_FORMS})
qt5_add_resources(APP_RCC ${APP_RESOURCES})

if(Qt5LinguistTools_FOUND)
  message(STATUS "[rssguard] Qt Linguist Tools found. Translations will get refreshed.")
  qt5_add_translation(APP_QM ${APP_TRANSLATIONS})
else(Qt5LinguistTools_FOUND)
  message(STATUS "[rssguard] Qt Linguist Tools NOT found. No refreshing for translations.")
endif(Qt5LinguistTools_FOUND)

# Include additional directory paths.
include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  # ${CMAKE_CURRENT_SOURCE_DIR}/src/muparserx
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/src
  ${Qt5Xml_INCLUDE_DIRS}
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
)

# Installation stage.
#if(APPLE)
#    # Mac's bundle install.
#    message(STATUS "[qonverter] You will probably install on Mac OS.")
#    set_target_properties(${EXE_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/macosx/Info.plist.in")
#    set(MACOSX_BUNDLE_ICON_FILE qonverter.icns)
#    set(MACOSX_BUNDLE_INFO_STRING "${EXE_NAME} ${APP_VERSION}")
#    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.googlecode.qonverter")
#    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${APP_VERSION}")
#    set(MACOSX_BUNDLE_BUNDLE_NAME "${EXE_NAME}")
#    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}")
#    set(MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}")
#    set(MACOSX_BUNDLE_COPYRIGHT "(c) Martin Rotter")
#    set_source_files_properties(${CMAKE_SOURCE_DIR}/resources/macosx/qonverter.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
#    install(TARGETS ${EXE_NAME} BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX})
#    install(FILES ${APP_QM} DESTINATION ${CMAKE_INSTALL_PREFIX}/${EXE_NAME}.app/Contents/Resources/l10n)
#    install(FILES ${APP_SKINS} DESTINATION ${CMAKE_INSTALL_PREFIX}/${EXE_NAME}.app/Contents/Resources/skins)
#    install(FILES ${APP_MISC} DESTINATION ${CMAKE_INSTALL_PREFIX}/${EXE_NAME}.app/Contents/Resources/information)
#    configure_file(resources/macosx/bundle.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/bundle.cmake @ONLY)
#    add_custom_target(bundle ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/bundle.cmake)
#elseif(WIN32)
if(WIN32)
  message(STATUS "[rssguard] You will probably install on Windows.")
  install(TARGETS ${EXE_NAME} RUNTIME DESTINATION ./)
  install(FILES ${APP_QM} DESTINATION ./l10n)
  install(FILES ${APP_SKIN_PLAIN} DESTINATION ./skins/base)
  install(FILES ${APP_SKIN_MODERN} DESTINATION ./skins/base)
  install(FILES ${APP_SKIN_DARK} DESTINATION ./skins/fancy)
  install(FILES ${APP_SKIN_DARK_IMAGES} DESTINATION ./skins/fancy/images)
  install(FILES ${APP_MISC} DESTINATION ./)
elseif(OS2)
  message(STATUS "[rssguard] You will probably install on OS/2.")
  install(TARGETS ${EXE_NAME} RUNTIME DESTINATION ./)
  install(FILES ${APP_QM} DESTINATION ./l10n)
  install(FILES ${APP_SKIN_PLAIN} DESTINATION ./skins/base)
  install(FILES ${APP_SKIN_MODERN} DESTINATION ./skins/base)
  install(FILES ${APP_SKIN_DARK} DESTINATION ./skins/fancy)
  install(FILES ${APP_SKIN_DARK_IMAGES} DESTINATION ./skins/fancy/images)
  install(FILES ${APP_MISC} DESTINATION ./)
elseif(UNIX)
  message(STATUS "[rssguard] You will probably install on Linux.")
  install(TARGETS ${EXE_NAME} RUNTIME DESTINATION bin)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources/desktop/qonverter.desktop DESTINATION share/applications)
  install(FILES resources/graphics/qonverter.png DESTINATION share/icons/hicolor/256x256/apps/)
  install(FILES ${APP_QM} DESTINATION share/qonverter/l10n)
  install(FILES ${APP_SKIN_PLAIN} DESTINATION share/qonverter/skins/base)
  install(FILES ${APP_SKIN_MODERN} DESTINATION share/qonverter/skins/base)
  install(FILES ${APP_SKIN_DARK} DESTINATION share/qonverter/skins/fancy)
  install(FILES ${APP_SKIN_DARK_IMAGES} DESTINATION share/qonverter/skins/fancy/images)
  install(FILES ${APP_MISC} DESTINATION share/qonverter/information)
endif(WIN32)

# Custom target for packaging.
set(CPACK_PACKAGE_NAME ${APP_LOW_NAME})
set(CPACK_PACKAGE_VERSION ${APP_VERSION})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-src")
set(CPACK_IGNORE_FILES "/ideas/;/resources/aur/;\\\\.psd$;/resources/deployment;/CVS/;/\\\\.svn/;/\\\\.git/;\\\\.swp$;/CMakeLists.txt.user;\\\\.#;/#;\\\\.tar.gz$;/CMakeFiles/;CMakeCache.txt;\\\\.qm$;/build/;\\\\.diff$;.DS_Store'")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})

# Load packaging facilities.
include(CPack)

# make dist implementation.
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# make lupdate and make lrelease implementation.
add_custom_target(lupdate
  ${Qt5Core_QMAKE_EXECUTABLE} -project -o ${CMAKE_CURRENT_BINARY_DIR}/qonverter.pro
    COMMAND ${Qt5_LUPDATE_EXECUTABLE} -ts ${APP_TRANSLATIONS_WO_QT} -no-obsolete -pro ${CMAKE_CURRENT_BINARY_DIR}/qonverter.pro
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
