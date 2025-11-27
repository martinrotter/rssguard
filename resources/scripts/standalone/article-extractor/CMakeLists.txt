find_program(GO_EXECUTABLE go REQUIRED)

set(OUTPUT_BINARY "${CMAKE_BINARY_DIR}/src/rssguard/rssguard-article-extractor")

if(WIN32)
  set(GO_FLAGS "-ldflags=\"-s -w\"")
else()
  set(GO_FLAGS "")
endif()

add_custom_command(
  OUTPUT ${OUTPUT_BINARY}
  COMMAND ${GO_EXECUTABLE} mod tidy
  COMMAND ${GO_EXECUTABLE} build ${GO_FLAGS} -o "${OUTPUT_BINARY}" -v "main.go"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/main.go
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building Go tool: rssguard-article-extractor"
)

add_custom_target(rssguard_article_extractor ALL DEPENDS "${OUTPUT_BINARY}")
add_dependencies(rssguard rssguard_article_extractor)

if((WIN32 AND NOT BUILD_MSYS2) OR OS2)
  install(FILES ${OUTPUT_BINARY} DESTINATION .)
elseif((MINGW AND BUILD_MSYS2) OR (UNIX AND NOT APPLE))
  include (GNUInstallDirs)
  install(FILES ${OUTPUT_BINARY} DESTINATION ${CMAKE_INSTALL_BINDIR})
elseif(APPLE)
  install(FILES ${OUTPUT_BINARY} DESTINATION Contents/MacOS)
endif()
