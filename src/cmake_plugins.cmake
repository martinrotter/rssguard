function(prepare_rssguard_plugin plugin_target_name)
  message(STATUS "Preparing plugin ${plugin_target_name}.")

  if(NOT DEFINED LIBRSSGUARD_SOURCE_PATH)
    set(LIBRSSGUARD_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
  endif()

  qt_wrap_ui(SOURCES ${UI_FILES})

  if(WIN32)
    enable_language("RC")
    list(APPEND SOURCES "${CMAKE_BINARY_DIR}/rssguard.rc")
  endif()

  add_library(${PLUGIN_TARGET} SHARED ${SOURCES} ${QM_FILES})

  target_link_libraries(${plugin_target_name} PUBLIC
    rssguard
  )

  target_compile_definitions(${plugin_target_name}
    PRIVATE
    RSSGUARD_DLLSPEC=Q_DECL_IMPORT
  )

  target_include_directories(${plugin_target_name}
    PUBLIC
    ${LIBRSSGUARD_SOURCE_PATH}
  )

  if(MSVC OR OS2)
    install(TARGETS ${plugin_target_name} DESTINATION plugins)
  elseif(MINGW)
    include (GNUInstallDirs)
    install(TARGETS ${plugin_target_name}
      DESTINATION ${CMAKE_INSTALL_DATADIR}/rssguard/plugins)
  elseif(UNIX AND NOT APPLE AND NOT ANDROID)
    include (GNUInstallDirs)
    install(TARGETS ${plugin_target_name}
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/rssguard
    )
  elseif(APPLE)
    install(TARGETS ${plugin_target_name}
      DESTINATION Contents/MacOS
    )
  endif()

  message(STATUS "Plugin ${plugin_target_name} is prepared.")
endfunction()
