if(NOT DEFINED LIBRSSGUARD_BINARY_PATH)
  set(LIBRSSGUARD_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

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

add_library(rssguard-ttrss SHARED ${SOURCES} ${QM_FILES})

# Add specific definitions.
target_compile_definitions(rssguard-ttrss
  PRIVATE
  RSSGUARD_DLLSPEC=Q_DECL_IMPORT
)

target_include_directories(rssguard-ttrss
  PUBLIC
  ${LIBRSSGUARD_SOURCE_PATH}
  src/3rd-party/richtexteditor
)

# Qt.
target_link_libraries(rssguard-ttrss PUBLIC
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
  install(TARGETS rssguard-ttrss DESTINATION plugins)
elseif(UNIX AND NOT APPLE AND NOT ANDROID)
  include (GNUInstallDirs)
  install(TARGETS rssguard-ttrss
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/rssguard
  )
elseif(APPLE)
  install(TARGETS rssguard-ttrss
    DESTINATION Contents/MacOS
  )
endif()
