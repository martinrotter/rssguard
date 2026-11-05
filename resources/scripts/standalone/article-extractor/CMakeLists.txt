find_program(GO_EXECUTABLE go REQUIRED)

set(GO_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(GO_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/go-version.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/check_go.cmake")
message(STATUS "Article extractor Go compiler: ${GO_VERSION}")

set(OUTPUT_BINARY "${CMAKE_BINARY_DIR}/src/rssguard/rssguard-article-extractor${CMAKE_EXECUTABLE_SUFFIX}")
set(GO_FLAGS)
if(WIN32)
  list(APPEND GO_FLAGS "-ldflags=-s -w")
endif()

# Include local and vendored sources, but test edits do not rebuild the installed helper.
file(GLOB_RECURSE GO_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.go")
list(FILTER GO_SOURCES EXCLUDE REGEX "_test\\.go$")
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/go-sources.txt" CONTENT "${GO_SOURCES}\n")
set(GO_MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/go.mod" "${CMAKE_CURRENT_SOURCE_DIR}/go.sum")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/modules.txt")
  list(APPEND GO_MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/modules.txt")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${GO_MODULE_FILES} "${GO_EXECUTABLE}")

# SDK installers can preserve an old timestamp. Check the version without rebuilding unchanged binaries.
add_custom_target(rssguard_go_version_check
  COMMAND "${CMAKE_COMMAND}" "-DGO_EXECUTABLE=${GO_EXECUTABLE}" "-DGO_SOURCE_DIR=${GO_SOURCE_DIR}"
          "-DGO_VERSION_FILE=${GO_VERSION_FILE}" -P "${CMAKE_CURRENT_SOURCE_DIR}/check_go.cmake"
  BYPRODUCTS "${GO_VERSION_FILE}"
  VERBATIM
)
add_custom_command(
  OUTPUT "${OUTPUT_BINARY}"
  COMMAND "${GO_EXECUTABLE}" build -trimpath ${GO_FLAGS} -o "${OUTPUT_BINARY}" .
  DEPENDS ${GO_SOURCES} ${GO_MODULE_FILES} "${GO_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/go-sources.txt" "${GO_VERSION_FILE}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Building Go tool: rssguard-article-extractor"
  VERBATIM
)
add_custom_target(rssguard_article_extractor ALL DEPENDS "${OUTPUT_BINARY}")
add_dependencies(rssguard_article_extractor rssguard_go_version_check)
add_dependencies(rssguard rssguard_article_extractor)

if((WIN32 AND NOT BUILD_MSYS2) OR OS2)
  install(PROGRAMS "${OUTPUT_BINARY}" DESTINATION .)
elseif((MINGW AND BUILD_MSYS2) OR (UNIX AND NOT APPLE))
  include(GNUInstallDirs)
  install(PROGRAMS "${OUTPUT_BINARY}" DESTINATION ${CMAKE_INSTALL_BINDIR})
elseif(APPLE)
  install(PROGRAMS "${OUTPUT_BINARY}" DESTINATION Contents/MacOS)
endif()
