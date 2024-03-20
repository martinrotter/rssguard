function(prepare_rssguard_plugin plugin_target_name)
  if(NOT DEFINED LIBRSSGUARD_SOURCE_PATH)
    set(LIBRSSGUARD_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
  endif()

  target_compile_definitions(${plugin_target_name}
    PRIVATE
    RSSGUARD_DLLSPEC=Q_DECL_IMPORT
  )

  target_include_directories(${plugin_target_name}
    PUBLIC
    ${LIBRSSGUARD_SOURCE_PATH}
  )

  if(WIN32 OR OS2)
    install(TARGETS ${plugin_target_name} DESTINATION plugins)
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
endfunction()
