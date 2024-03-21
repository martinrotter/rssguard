include(../cmake_plugins.cmake)
set(PLUGIN_TARGET "rssguard-feedly")

set(SOURCES
  src/definitions.h
  src/feedlyentrypoint.cpp
  src/feedlyentrypoint.h
  src/feedlynetwork.cpp
  src/feedlynetwork.h
  src/feedlyserviceroot.cpp
  src/feedlyserviceroot.h
  src/gui/feedlyaccountdetails.cpp
  src/gui/feedlyaccountdetails.h
  src/gui/formeditfeedlyaccount.cpp
  src/gui/formeditfeedlyaccount.h
)

set(UI_FILES
  src/gui/feedlyaccountdetails.ui
)

set(FEEDLY_CLIENT_ID "" CACHE STRING "Feedly client ID")
set(FEEDLY_CLIENT_SECRET "" CACHE STRING "Feedly client secret")

prepare_rssguard_plugin(${PLUGIN_TARGET})

if(NOT FEEDLY_CLIENT_ID STREQUAL "" AND NOT FEEDLY_CLIENT_SECRET STREQUAL "")
  target_compile_definitions(${PLUGIN_TARGET}
    PRIVATE

    FEEDLY_OFFICIAL_SUPPORT
    FEEDLY_CLIENT_ID="${FEEDLY_CLIENT_ID}"
    FEEDLY_CLIENT_SECRET="${FEEDLY_CLIENT_SECRET}"
  )

  message(STATUS "Enabling official Feedly support.")
else()
  message(STATUS "Feedly client ID/secret variables are not set. Disabling official support.")
endif()
