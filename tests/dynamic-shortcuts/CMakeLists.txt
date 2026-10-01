# Also usable independently of RSS Guard, WebEngine and MPV:
# cmake -S tests/dynamic-shortcuts -B build/shortcut-tests -DQT_MAJOR_VERSION=6 -DCMAKE_PREFIX_PATH=...
cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(rssguard_shortcut_tests LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_AUTOMOC ON)
  set(QT_MAJOR_VERSION 6 CACHE STRING "Qt major version for standalone tests")
  find_package(Qt${QT_MAJOR_VERSION} REQUIRED COMPONENTS Widgets Test)
  set(QT_VERSION_MAJOR ${QT_MAJOR_VERSION})
  enable_testing()
endif()

get_filename_component(RSSGUARD_SHORTCUT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src/librssguard" ABSOLUTE)
add_executable(test_shortcutpriority
  test_shortcutpriority.cpp
  ${RSSGUARD_SHORTCUT_SOURCE_DIR}/dynamic-shortcuts/shortcutpriority.cpp
  ${RSSGUARD_SHORTCUT_SOURCE_DIR}/dynamic-shortcuts/shortcutsequenceedit.h
)
target_include_directories(test_shortcutpriority PRIVATE ${RSSGUARD_SHORTCUT_SOURCE_DIR})
target_link_libraries(test_shortcutpriority PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Test)
target_compile_options(test_shortcutpriority PRIVATE
  "$<$<CXX_COMPILER_ID:MSVC>:/permissive-;/Zc:__cplusplus;/utf-8>")

add_executable(test_shortcutsettings
  test_shortcutsettings.cpp
  ${RSSGUARD_SHORTCUT_SOURCE_DIR}/dynamic-shortcuts/dynamicshortcutswidget.cpp
  ${RSSGUARD_SHORTCUT_SOURCE_DIR}/dynamic-shortcuts/dynamicshortcutswidget.h
  ${RSSGUARD_SHORTCUT_SOURCE_DIR}/dynamic-shortcuts/shortcutcatcher.cpp
  ${RSSGUARD_SHORTCUT_SOURCE_DIR}/dynamic-shortcuts/shortcutcatcher.h
  ${RSSGUARD_SHORTCUT_SOURCE_DIR}/dynamic-shortcuts/shortcutpriority.cpp
)
# Only icon/button services and modal decisions are doubled; the recorders and
# settings-widget implementation are the production sources.
target_include_directories(test_shortcutsettings PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs ${RSSGUARD_SHORTCUT_SOURCE_DIR})
target_link_libraries(test_shortcutsettings PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Test)
target_compile_options(test_shortcutsettings PRIVATE
  "$<$<CXX_COMPILER_ID:MSVC>:/permissive-;/Zc:__cplusplus;/utf-8>")

foreach(SHORTCUT_TEST_TARGET test_shortcutpriority test_shortcutsettings)
  if(WIN32)
    add_test(NAME ${SHORTCUT_TEST_TARGET}
      COMMAND ${CMAKE_COMMAND} -E env
        "QT_QPA_PLATFORM=minimal"
        "PATH=$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>$<SEMICOLON>$ENV{PATH}"
        $<TARGET_FILE:${SHORTCUT_TEST_TARGET}>
    )
  else()
    add_test(NAME ${SHORTCUT_TEST_TARGET}
      COMMAND ${CMAKE_COMMAND} -E env "QT_QPA_PLATFORM=offscreen" $<TARGET_FILE:${SHORTCUT_TEST_TARGET}>
    )
  endif()
endforeach()
