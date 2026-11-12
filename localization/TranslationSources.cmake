# Collect all user-facing code, including plugins and backends disabled in this build.
function(rssguard_translation_sources out_sources out_includes)
  file(GLOB libraries LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/src/librssguard*")
  set(sources "")
  set(includes "${CMAKE_SOURCE_DIR}/src")

  foreach(library IN LISTS libraries)
    if(NOT IS_DIRECTORY "${library}")
      continue()
    endif()

    get_filename_component(name "${library}" NAME)
    list(APPEND includes "${library}" "${CMAKE_BINARY_DIR}/src/${name}")
    file(GLOB_RECURSE library_sources CONFIGURE_DEPENDS LIST_DIRECTORIES false
      "${library}/*.cpp" "${library}/*.h" "${library}/*.ui")

    foreach(source IN LISTS library_sources)
      file(RELATIVE_PATH relative_source "${library}" "${source}")
      if(relative_source MATCHES "(^|/)(build|CMakeFiles)/")
        continue()
      endif()
      if(relative_source MATCHES "(^|/)3rd-party/" AND
         NOT (name STREQUAL "librssguard-gmail" AND relative_source MATCHES "^src/3rd-party/richtexteditor/"))
        continue()
      endif()
      list(APPEND sources "${source}")
    endforeach()
  endforeach()

  list(REMOVE_DUPLICATES sources)
  list(SORT sources)
  set(${out_sources} "${sources}" PARENT_SCOPE)
  set(${out_includes} "${includes}" PARENT_SCOPE)
endfunction()
