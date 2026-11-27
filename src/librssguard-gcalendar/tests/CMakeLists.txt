function(add_calendar_test target)
  add_executable(${target} ${target}.cpp ${ARGN})
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_SOURCE_DIR}/src)
  target_compile_definitions(${target} PRIVATE RSSGUARD_DLLSPEC=Q_DECL_IMPORT)
  target_link_libraries(${target} PRIVATE rssguard Qt${QT_VERSION_MAJOR}::Test)

  set(platform offscreen)
  if(WIN32 AND QT_VERSION_MAJOR EQUAL 5 AND NOT TARGET Qt5::QOffscreenIntegrationPlugin
      AND TARGET Qt5::QMinimalIntegrationPlugin)
    set(platform minimal)
  endif()
  if(WIN32)
    add_test(NAME ${target} COMMAND ${CMAKE_COMMAND} -E env "QT_QPA_PLATFORM=${platform}"
      "PATH=$<TARGET_FILE_DIR:rssguard>$<SEMICOLON>$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>$<SEMICOLON>$ENV{PATH}"
      $<TARGET_FILE:${target}>)
  else()
    add_test(NAME ${target} COMMAND ${CMAKE_COMMAND} -E env "QT_QPA_PLATFORM=${platform}" $<TARGET_FILE:${target}>)
  endif()
endfunction()

add_calendar_test(test_calendarevent ../src/calendarevent.cpp ../src/gcalendarfeed.cpp ../src/gcalendarfeed.h)
add_calendar_test(test_calendareventdraft ../src/calendarevent.cpp ../src/calendareventdraft.cpp)
add_calendar_test(test_calendarreminders ../src/calendarevent.cpp ../src/calendarreminders.cpp)
add_calendar_test(test_gcalendarnetwork ../src/gcalendarnetworkfactory.cpp)

set(calendar_fetch_sources)
foreach(source IN LISTS SOURCES)
  if(NOT source MATCHES "gcalendarentrypoint")
    list(APPEND calendar_fetch_sources "../${source}")
  endif()
endforeach()
set(calendar_fetch_forms)
foreach(form IN LISTS UI_FILES)
  list(APPEND calendar_fetch_forms "../${form}")
endforeach()
qt_wrap_ui(calendar_fetch_sources ${calendar_fetch_forms})
add_calendar_test(test_gcalendarfetch ${calendar_fetch_sources})
target_include_directories(test_gcalendarfetch PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  "${CMAKE_BINARY_DIR}/src/librssguard"
)
set_tests_properties(test_gcalendarfetch PROPERTIES TIMEOUT 60)
