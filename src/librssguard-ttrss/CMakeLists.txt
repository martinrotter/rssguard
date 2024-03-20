include(../cmake_plugins.cmake)
set(PLUGIN_TARGET "rssguard-ttrss")

set(SOURCES
  src/definitions.h
  src/gui/formeditttrssaccount.cpp
  src/gui/formeditttrssaccount.h
  src/gui/formttrssfeeddetails.cpp
  src/gui/formttrssfeeddetails.h
  src/gui/formttrssnote.cpp
  src/gui/formttrssnote.h
  src/gui/ttrssaccountdetails.cpp
  src/gui/ttrssaccountdetails.h
  src/gui/ttrssfeeddetails.cpp
  src/gui/ttrssfeeddetails.h
  src/ttrssfeed.cpp
  src/ttrssfeed.h
  src/ttrssnetworkfactory.cpp
  src/ttrssnetworkfactory.h
  src/ttrssnotetopublish.h
  src/ttrssserviceentrypoint.cpp
  src/ttrssserviceentrypoint.h
  src/ttrssserviceroot.cpp
  src/ttrssserviceroot.h
)

set(UI_FILES
  src/gui/formttrssnote.ui
  src/gui/ttrssaccountdetails.ui
  src/gui/ttrssfeeddetails.ui
)

# Deal with .ui files.
qt_wrap_ui(SOURCES ${UI_FILES})

# Bundle version info.
if(WIN32)
  enable_language("RC")
  list(APPEND SOURCES "${CMAKE_BINARY_DIR}/rssguard.rc")
endif()

add_library(${PLUGIN_TARGET} SHARED ${SOURCES} ${QM_FILES})

# Qt.
target_link_libraries(${PLUGIN_TARGET} PUBLIC
  rssguard
)

prepare_rssguard_plugin(${PLUGIN_TARGET})
