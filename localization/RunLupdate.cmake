# Qt 6.5 has no -warnings-are-errors option. Enforce it for all supported versions.
get_filename_component(destination "${OUTPUT}" ABSOLUTE)
get_filename_component(directory "${destination}" DIRECTORY)
get_filename_component(filename "${destination}" NAME)
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef nonce)
# Keep the staging file beside the catalog so source locations stay unchanged.
set(staged "${directory}/.lupdate-${nonce}-${filename}")
if(EXISTS "${destination}")
  configure_file("${destination}" "${staged}" COPYONLY)
endif()

execute_process(
  COMMAND "${LUPDATE}" "@${INPUT}" -ts "${staged}" -no-obsolete
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE diagnostics)

if(NOT result STREQUAL "0" OR NOT diagnostics STREQUAL "")
  file(REMOVE "${staged}")
  message(FATAL_ERROR "lupdate failed or reported warnings:\n${output}${diagnostics}")
endif()
file(RENAME "${staged}" "${destination}" RESULT result)
if(NOT result STREQUAL "0")
  file(REMOVE "${staged}")
  message(FATAL_ERROR "Could not replace translation catalog: ${result}")
endif()
message(STATUS "${output}")
