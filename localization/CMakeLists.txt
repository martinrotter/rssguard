# Collect all .ts files.
FILE(GLOB TS_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/*.ts)

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}")

# Update .ts file and/or just generate .qm files.
if(UPDATE_TRANSLATIONS)
  qt_create_translation(QM_FILES
    "${CMAKE_SOURCE_DIR}/src" ${TS_FILES}
    OPTIONS "-no-obsolete"
  )
else()
  qt_add_translation(QM_FILES
    ${TS_FILES}
    OPTIONS "-compress"
  )
endif()

add_custom_target(update_qm DEPENDS ${QM_FILES})
