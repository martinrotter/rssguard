if(NOT DEFINED LIBRSSGUARD_BINARY_PATH)
  set(LIBRSSGUARD_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

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

# Deal with .ui files.
qt_wrap_ui(SOURCES ${UI_FILES})

set(FEEDLY_CLIENT_ID "" CACHE STRING "Feedly client ID")
set(FEEDLY_CLIENT_SECRET "" CACHE STRING "Feedly client secret")

# Bundle version info.
if(WIN32)
  enable_language("RC")
  list(APPEND SOURCES "${CMAKE_BINARY_DIR}/rssguard.rc")
endif()

add_library(rssguard-feedly SHARED ${SOURCES} ${QM_FILES})

# Add specific definitions.
target_compile_definitions(rssguard-feedly
  PRIVATE
  RSSGUARD_DLLSPEC=Q_DECL_IMPORT
  RSSGUARD_DLLSPEC_EXPORT=Q_DECL_EXPORT
)

if(NOT FEEDLY_CLIENT_ID STREQUAL "" AND NOT FEEDLY_CLIENT_SECRET STREQUAL "")
  target_compile_definitions(rssguard-feedly
    PRIVATE

    FEEDLY_OFFICIAL_SUPPORT
    FEEDLY_CLIENT_ID="${FEEDLY_CLIENT_ID}"
    FEEDLY_CLIENT_SECRET="${FEEDLY_CLIENT_SECRET}"
  )

  message(STATUS "Enabling official Feedly support.")
else()
  message(STATUS "Feedly client ID/secret variables are not set. Disabling official support.")
endif()

target_include_directories(rssguard-feedly
  PUBLIC
  ${LIBRSSGUARD_SOURCE_PATH}
)

# Qt.
target_link_libraries(rssguard-feedly PUBLIC
  rssguard
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Qml
  Qt${QT_VERSION_MAJOR}::Sql
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::Xml
  Qt${QT_VERSION_MAJOR}::Concurrent
)

if(QT_VERSION_MAJOR EQUAL 6)
  target_link_libraries(rssguard-feedly PUBLIC
    Qt${QT_VERSION_MAJOR}::Core5Compat
  )
endif()

if(WIN32 OR OS2)
  install(TARGETS rssguard-feedly DESTINATION plugins)
elseif(UNIX AND NOT APPLE AND NOT ANDROID)
  include (GNUInstallDirs)
  install(TARGETS rssguard-feedly
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/rssguard
  )
elseif(APPLE)
  install(TARGETS rssguard-feedly
    DESTINATION Contents/MacOS
  )
endif()
