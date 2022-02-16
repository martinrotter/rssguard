if(UPDATE_TRANSLATIONS)
  # Regenerate "en" .ts file.
  file(GLOB_RECURSE ALL_SOURCES
    "${CMAKE_SOURCE_DIR}/src/librssguard/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/librssguard/*.ui")

  qt_add_lupdate(rssguard
    TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/rssguard_en.ts"
    SOURCES ${ALL_SOURCES}
    INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/src/librssguard"
    OPTIONS "-no-obsolete")
endif()

# Collect all .ts files and generate .qm files.
FILE(GLOB TS_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/*.ts)

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}")
qt_add_translation(QM_FILES
  ${TS_FILES}
  OPTIONS "-compress"
)

add_custom_target(update_qm DEPENDS ${QM_FILES})
