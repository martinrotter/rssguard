if(WIN32)
  enable_language("RC")
  add_executable(app main.cpp ${CMAKE_BINARY_DIR}/rssguard.rc)
else()
  add_executable(app main.cpp)
endif()

target_compile_definitions(app PRIVATE RSSGUARD_DLLSPEC=Q_DECL_IMPORT)

set_target_properties (app PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})

target_include_directories(app PUBLIC
  ${CMAKE_BINARY_DIR}/src/librssguard/rssguard_autogen/include
)

target_link_libraries(app PUBLIC
  Qt::Core
  Qt::Gui
  Qt::Widgets
  rssguard
)

if(QT_VERSION_MAJOR EQUAL 6)
  target_link_libraries(app PUBLIC
    Qt::Core5Compat
  )
endif()

if(APPLE)
  target_compile_options(app PUBLIC -mmacosx-version-min=10.10)
  target_link_options(app PUBLIC -mmacosx-version-min=10.10)
  set_target_properties(app PROPERTIES
    MACOSX_BUNDLE FALSE
  )
elseif(WIN32)
  target_link_libraries(app PUBLIC
    Shell32.lib
    odbc32
  )
endif()

if(WIN32)
  install(TARGETS app DESTINATION .)
  install(FILES ${CMAKE_SOURCE_DIR}/resources/graphics/${CMAKE_PROJECT_NAME}.ico
    DESTINATION .
  )
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/scripts/redist/
    DESTINATION .
    FILES_MATCHING PATTERN "*.dll"
  )
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/scripts/clang-format
    DESTINATION .
    FILES_MATCHING PATTERN "*.exe"
  )
elseif(OS2)
  install(TARGETS app DESTINATION .)
elseif(UNIX AND NOT APPLE AND NOT ANDROID)
  include (GNUInstallDirs)
  install(TARGETS app
    DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
  install(FILES ${CMAKE_SOURCE_DIR}/resources/desktop/${APP_REVERSE_NAME}.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
  )
  install(FILES ${CMAKE_SOURCE_DIR}/resources/desktop/${APP_REVERSE_NAME}.appdata.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
  )
  install(FILES ${CMAKE_SOURCE_DIR}/resources/graphics/${CMAKE_PROJECT_NAME}.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps
  )
elseif(APPLE)
  install(TARGETS app
    DESTINATION Contents/MacOS
  )
  install(FILES ${CMAKE_SOURCE_DIR}/resources/macosx/${CMAKE_PROJECT_NAME}.icns
    DESTINATION Contents/Resources
  )
  install(FILES
    ${CMAKE_SOURCE_DIR}/resources/macosx/Info.plist.in
    ${CMAKE_BINARY_DIR}/Info.plist
    DESTINATION Contents
  )
  file(WRITE PkgInfo "APPL????")
  install(FILES PkgInfo
    DESTINATION Contents
  )
endif()
