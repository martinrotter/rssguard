include(../cmake_plugins.cmake)
set(PLUGIN_TARGET "rssguard-nextcloud")

set(SOURCES
  src/definitions.h
  src/gui/formeditnextcloudaccount.cpp
  src/gui/formeditnextcloudaccount.h
  src/gui/nextcloudaccountdetails.cpp
  src/gui/nextcloudaccountdetails.h
  src/nextcloudfeed.cpp
  src/nextcloudfeed.h
  src/nextcloudnetworkfactory.cpp
  src/nextcloudnetworkfactory.h
  src/nextcloudserviceentrypoint.cpp
  src/nextcloudserviceentrypoint.h
  src/nextcloudserviceroot.cpp
  src/nextcloudserviceroot.h
)

set(UI_FILES
  src/gui/nextcloudaccountdetails.ui
)

# Deal with .ui files.
qt_wrap_ui(SOURCES ${UI_FILES})

# Bundle version info.
if(WIN32)
  enable_language("RC")
  list(APPEND SOURCES "${CMAKE_BINARY_DIR}/rssguard.rc")
endif()

add_library(${PLUGIN_TARGET} SHARED ${SOURCES} ${QM_FILES})

target_link_libraries(${PLUGIN_TARGET} PUBLIC
  rssguard
)

prepare_rssguard_plugin(${PLUGIN_TARGET})
