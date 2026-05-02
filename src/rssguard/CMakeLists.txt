set(SOURCES
  main.cpp)

if(WIN32)
  list(APPEND SOURCES
    ${CMAKE_BINARY_DIR}/rssguard.rc
    ${CMAKE_SOURCE_DIR}/resources/desktop/rssguard.manifest
  )
endif()

add_executable(app ${SOURCES})

if(WIN32)
  set_property(TARGET app PROPERTY WIN32_EXECUTABLE true)
endif()

target_compile_definitions(app PRIVATE RSSGUARD_DLLSPEC=Q_DECL_IMPORT)

set_target_properties(app PROPERTIES
  OUTPUT_NAME ${CMAKE_PROJECT_NAME}
  C_STANDARD 99
)

target_include_directories(app PUBLIC
  ${CMAKE_BINARY_DIR}/src/librssguard
)

target_link_libraries(app PUBLIC
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Widgets
  rssguard
)

if(APPLE)
  set_target_properties(app PROPERTIES
    MACOSX_BUNDLE FALSE
    INSTALL_RPATH "@executable_path/../Frameworks"
  )
elseif(WIN32)
  target_link_libraries(app PUBLIC
    shell32
    odbc32
  )
endif()

if(WIN32 AND NOT BUILD_MSYS2)
  install(TARGETS app DESTINATION .)
  install(FILES ${CMAKE_SOURCE_DIR}/resources/graphics/${CMAKE_PROJECT_NAME}.ico
    DESTINATION .
  )
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/scripts/redist/
    DESTINATION .
    FILES_MATCHING PATTERN "*.dll"
  )
  install(FILES ${CMAKE_SOURCE_DIR}/resources/scripts/clang-format/clang-format.exe
    DESTINATION .
  )
elseif(OS2)
  install(TARGETS app DESTINATION .)
elseif((MINGW AND BUILD_MSYS2) OR (UNIX AND NOT APPLE))
  include (GNUInstallDirs)
  install(TARGETS app
    DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  if (NOT BUILD_MSYS2)
    # MSYS2 ecosystem does not need/want these files.
    install(FILES ${CMAKE_BINARY_DIR}/${APP_REVERSE_NAME}.desktop
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    )
    install(FILES ${CMAKE_BINARY_DIR}/${APP_REVERSE_NAME}.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
    )
    install(FILES ${CMAKE_SOURCE_DIR}/resources/graphics/${CMAKE_PROJECT_NAME}.png
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps
      RENAME ${APP_REVERSE_NAME}.png
    )
  endif()
elseif(APPLE)
  install(TARGETS app
    DESTINATION Contents/MacOS
  )
  install(FILES ${CMAKE_SOURCE_DIR}/resources/macosx/${CMAKE_PROJECT_NAME}.icns
    DESTINATION Contents/Resources
  )
  install(FILES
    ${CMAKE_BINARY_DIR}/Info.plist
    DESTINATION Contents
  )
endif()
