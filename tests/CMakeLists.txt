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

    if(TEST_TARGET STREQUAL "test_mediawikiparser")
      set(TEST_EXTRA_PATH "$<TARGET_FILE_DIR:rssguard-standard>$<SEMICOLON>${CMAKE_SOURCE_DIR}/resources/scripts/libmpv$<SEMICOLON>")
    endif()

    add_test(NAME ${TEST_TARGET}
      COMMAND ${CMAKE_COMMAND} -E env
        "QT_QPA_PLATFORM=offscreen"
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

  add_rssguard_test(test_httpresponse
    network-web/test_httpresponse.cpp
  )

  add_rssguard_test(test_webfactory
    network-web/test_webfactory.cpp
  )

  add_rssguard_test(test_message
    core/test_message.cpp
  )

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
