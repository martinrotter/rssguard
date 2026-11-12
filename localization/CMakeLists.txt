if(BUILD_WITH_QT6 AND (UPDATE_TRANSLATIONS OR ENABLE_TESTING))
  include(TranslationSources.cmake)
  rssguard_translation_sources(translation_sources translation_includes)
  set(translation_input "")
  foreach(directory IN LISTS translation_includes)
    string(APPEND translation_input "-I${directory}\n")
  endforeach()
  foreach(source IN LISTS translation_sources)
    string(APPEND translation_input "${source}\n")
  endforeach()
  set(RSSGUARD_LUPDATE_INPUT "${CMAKE_CURRENT_BINARY_DIR}/lupdate-input.txt")
  set(RSSGUARD_LUPDATE_INPUT "${RSSGUARD_LUPDATE_INPUT}" PARENT_SCOPE)
  file(GENERATE OUTPUT "${RSSGUARD_LUPDATE_INPUT}" CONTENT "${translation_input}")

  if(UPDATE_TRANSLATIONS)
    # The English catalog is the source for translators, not a selectable localization.
    # Invoke the tool directly to avoid a duplicate target-source scan and support Qt 6.5.
    add_custom_target(rssguard_lupdate
      COMMAND ${CMAKE_COMMAND} "-DLUPDATE=$<TARGET_FILE:Qt6::lupdate>" "-DINPUT=${RSSGUARD_LUPDATE_INPUT}"
        "-DOUTPUT=${CMAKE_CURRENT_SOURCE_DIR}/rssguard_en.ts" -P "${CMAKE_CURRENT_SOURCE_DIR}/RunLupdate.cmake"
      VERBATIM)
    add_custom_target(update_translations DEPENDS rssguard_lupdate)
  endif()
endif()

# Collect application and Qt catalogs without translation-update staging files.
FILE(GLOB TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/rssguard_*.ts ${CMAKE_CURRENT_SOURCE_DIR}/qtbase_*.ts)

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
