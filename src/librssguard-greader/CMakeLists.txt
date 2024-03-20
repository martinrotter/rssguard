include(../cmake_plugins.cmake)
set(PLUGIN_TARGET "rssguard-greader")

set(SOURCES
  src/definitions.h
  src/greaderentrypoint.cpp
  src/greaderentrypoint.h
  src/greaderfeed.cpp
  src/greaderfeed.h
  src/greadernetwork.cpp
  src/greadernetwork.h
  src/greaderserviceroot.cpp
  src/greaderserviceroot.h
  src/gui/formeditgreaderaccount.cpp
  src/gui/formeditgreaderaccount.h
  src/gui/formgreaderfeeddetails.cpp
  src/gui/formgreaderfeeddetails.h
  src/gui/greaderaccountdetails.cpp
  src/gui/greaderaccountdetails.h
  src/gui/greaderfeeddetails.cpp
  src/gui/greaderfeeddetails.h
)

set(UI_FILES
  src/gui/greaderaccountdetails.ui
  src/gui/greaderfeeddetails.ui
)

# Deal with .ui files.
qt_wrap_ui(SOURCES ${UI_FILES})

set(INOREADER_CLIENT_ID "" CACHE STRING "Inoreader client ID")
set(INOREADER_CLIENT_SECRET "" CACHE STRING "Inoreader client secret")

# Bundle version info.
if(WIN32)
  enable_language("RC")
  list(APPEND SOURCES "${CMAKE_BINARY_DIR}/rssguard.rc")
endif()

add_library(${PLUGIN_TARGET} SHARED ${SOURCES} ${QM_FILES})

if(NOT INOREADER_CLIENT_ID STREQUAL "" AND NOT INOREADER_CLIENT_SECRET STREQUAL "")
  target_compile_definitions(rssguard
    PRIVATE

    INOREADER_OFFICIAL_SUPPORT
    INOREADER_CLIENT_ID="${INOREADER_CLIENT_ID}"
    INOREADER_CLIENT_SECRET="${INOREADER_CLIENT_SECRET}"
  )

  message(STATUS "Enabling official Inoreader support.")
else()
  message(STATUS "Inoreader client ID/secret variables are not set. Disabling official support.")
endif()

target_link_libraries(rssguard-greader PUBLIC
  rssguard
)

prepare_rssguard_plugin(${PLUGIN_TARGET})
