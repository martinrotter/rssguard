function(add_rssguard_test TEST_TARGET)
  add_executable(${TEST_TARGET} ${ARGN})

  target_compile_definitions(${TEST_TARGET}
    PRIVATE
    RSSGUARD_DLLSPEC=Q_DECL_IMPORT
  )

  target_link_libraries(${TEST_TARGET}
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    rssguard
  )

  if(WIN32)
    set(TEST_EXTRA_PATH "")
    set(TEST_PLATFORM "offscreen")

    if(TEST_TARGET STREQUAL "test_zoom" AND QT_VERSION_MAJOR EQUAL 5)
      # Qt 5's Windows WebEngine profile cannot initialize with the offscreen plugin.
      set(TEST_PLATFORM "windows")
    endif()

    if(TEST_TARGET STREQUAL "test_mediawikiparser")
      set(TEST_EXTRA_PATH "$<TARGET_FILE_DIR:rssguard-standard>$<SEMICOLON>${CMAKE_SOURCE_DIR}/resources/scripts/libmpv$<SEMICOLON>")
    elseif(TEST_TARGET STREQUAL "test_styleditemdelegate" OR TEST_TARGET STREQUAL "test_zoom")
      set(TEST_EXTRA_PATH "${CMAKE_SOURCE_DIR}/resources/scripts/libmpv$<SEMICOLON>")
    endif()

    add_test(NAME ${TEST_TARGET}
      COMMAND ${CMAKE_COMMAND} -E env
        "QT_QPA_PLATFORM=${TEST_PLATFORM}"
        "PATH=${TEST_EXTRA_PATH}$<TARGET_FILE_DIR:rssguard>$<SEMICOLON>$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>$<SEMICOLON>$ENV{PATH}"
        $<TARGET_FILE:${TEST_TARGET}>
    )
  else()
    add_test(NAME ${TEST_TARGET}
      COMMAND ${CMAKE_COMMAND} -E env
        "QT_QPA_PLATFORM=offscreen"
        $<TARGET_FILE:${TEST_TARGET}>
    )
  endif()
endfunction()

if(ENABLE_TESTING)
  message(STATUS "Enabling unit tests.")

  add_subdirectory(dynamic-shortcuts)

  add_rssguard_test(test_globals
    definitions/test_globals.cpp
  )

  add_rssguard_test(test_textfactory
    miscellaneous/test_textfactory.cpp
  )

  add_rssguard_test(test_xmlencodingdetector
    miscellaneous/test_xmlencodingdetector.cpp
  )

  add_rssguard_test(test_domdocument
    miscellaneous/test_domdocument.cpp
  )

  add_rssguard_test(test_iconfactory
    miscellaneous/test_iconfactory.cpp
  )

  add_rssguard_test(test_iofactory
    miscellaneous/test_iofactory.cpp
  )

  add_rssguard_test(test_settings
    miscellaneous/test_settings.cpp
  )

  add_rssguard_test(test_applicationdiagnostics
    miscellaneous/test_applicationdiagnostics.cpp
  )

  add_rssguard_test(test_httpresponse
    network-web/test_httpresponse.cpp
  )

  add_rssguard_test(test_webfactory
    network-web/test_webfactory.cpp
  )

  add_rssguard_test(test_message
    core/test_message.cpp
  )

  add_rssguard_test(test_zoom
    gui/test_zoom.cpp
  )
  set_tests_properties(test_zoom PROPERTIES
    ENVIRONMENT "QT_OPENGL=software;QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu"
    TIMEOUT 120)
  if(WEB_ARTICLE_VIEWER_WEBENGINE)
    set(ZOOM_TEST_PLATFORM "offscreen")
    if(QT_VERSION_MAJOR EQUAL 5)
      if(WIN32)
        set(ZOOM_TEST_PLATFORM "windows")
      elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Qt 5's WebEngine view needs an X11 OpenGL context; CI provides it through Xvfb.
        set(ZOOM_TEST_PLATFORM "xcb")
      endif()
    endif()
    if(WIN32)
      add_test(NAME test_zoom_webengine
        COMMAND ${CMAKE_COMMAND} -E env
          "QT_QPA_PLATFORM=${ZOOM_TEST_PLATFORM}"
          "PATH=${CMAKE_SOURCE_DIR}/resources/scripts/libmpv$<SEMICOLON>$<TARGET_FILE_DIR:rssguard>$<SEMICOLON>$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>$<SEMICOLON>$ENV{PATH}"
          $<TARGET_FILE:test_zoom>)
    else()
      add_test(NAME test_zoom_webengine
        COMMAND ${CMAKE_COMMAND} -E env
          "QT_QPA_PLATFORM=${ZOOM_TEST_PLATFORM}"
          $<TARGET_FILE:test_zoom>)
    endif()
    set_tests_properties(test_zoom_webengine PROPERTIES
      ENVIRONMENT "RSSGUARD_TEST_WEBENGINE=1;QT_OPENGL=software;QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu"
      TIMEOUT 120)
  endif()

  add_rssguard_test(test_styleditemdelegate
    gui/test_styleditemdelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/librssguard/gui/reusable/styleditemdelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/librssguard/gui/reusable/styleditemdelegate.h
  )
  if(WIN32)
    set_tests_properties(test_styleditemdelegate PROPERTIES
      ENVIRONMENT "QT_QPA_FONTDIR=$ENV{WINDIR}/Fonts")
  endif()

  add_rssguard_test(test_databasequeries
    database/test_databasequeries.cpp
  )

  add_rssguard_test(test_mediawikiparser
    standard/test_mediawikiparser.cpp
  )
  target_link_libraries(test_mediawikiparser PRIVATE rssguard-standard)
  target_include_directories(test_mediawikiparser PRIVATE ${CMAKE_SOURCE_DIR}/src/librssguard-standard)
  set_target_properties(test_mediawikiparser PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src/librssguard-standard)
endif()

if(ENABLE_BENCHMARKS)
  message(STATUS "Enabling benchmarks.")

  add_rssguard_test(benchmark_textfactory
    benchmarks/benchmark_textfactory.cpp
  )

  set_tests_properties(benchmark_textfactory PROPERTIES LABELS "benchmark")
endif()
