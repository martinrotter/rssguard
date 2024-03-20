if(NOT DEFINED LIBRSSGUARD_BINARY_PATH)
  set(LIBRSSGUARD_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

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

add_library(rssguard-nextcloud SHARED ${SOURCES} ${QM_FILES})

# Add specific definitions.
target_compile_definitions(rssguard-nextcloud
  PRIVATE
  RSSGUARD_DLLSPEC=Q_DECL_IMPORT
)

target_include_directories(rssguard-nextcloud
  PUBLIC
  ${LIBRSSGUARD_SOURCE_PATH}
)

# Qt.
target_link_libraries(rssguard-nextcloud PUBLIC
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

#if(QT_VERSION_MAJOR EQUAL 6)
#  target_link_libraries(rssguard-feedly PUBLIC
#    Qt${QT_VERSION_MAJOR}::Core5Compat
#  )
#endif()

if(WIN32 OR OS2)
  install(TARGETS rssguard-nextcloud DESTINATION plugins)
elseif(UNIX AND NOT APPLE AND NOT ANDROID)
  include (GNUInstallDirs)
  install(TARGETS rssguard-nextcloud
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/rssguard
  )
elseif(APPLE)
  install(TARGETS rssguard-nextcloud
    DESTINATION Contents/MacOS
  )
endif()
