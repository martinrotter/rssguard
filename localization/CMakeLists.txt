if(UPDATE_TRANSLATIONS AND BUILD_WITH_QT6)
  # Regenerate "en" .ts file.
  #
  # "en" .ts file is only used as "source" language
  # and it is not offered in RSS Guard as localization.
  file(GLOB_RECURSE ALL_SOURCES
    "${CMAKE_SOURCE_DIR}/src/librssguard*/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/librssguard*/*.h"
    "${CMAKE_SOURCE_DIR}/src/librssguard*/*.ui")

  qt_add_lupdate(rssguard
    TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/rssguard_en.ts"
    SOURCES ${ALL_SOURCES}
    INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/src/librssguard"
    OPTIONS "-no-obsolete")
endif()

# Collect all .ts files and generate .qm files.
FILE(GLOB TS_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/*.ts)

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}")

if(BUILD_WITH_QT6)
  qt_add_lrelease(rssguard
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
    OPTIONS "-compress"
  )
else()
  qt_add_translation(QM_FILES
    ${TS_FILES}
    OPTIONS "-compress"
  )

  add_custom_target(rssguard_lrelease DEPENDS ${QM_FILES})
  add_dependencies(rssguard rssguard_lrelease)
endif()
