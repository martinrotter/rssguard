if(NOT DEFINED LIBRSSGUARD_BINARY_PATH)
  set(LIBRSSGUARD_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

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

add_library(rssguard-greader SHARED ${SOURCES} ${QM_FILES})

# Add specific definitions.
target_compile_definitions(rssguard-greader
  PRIVATE
  RSSGUARD_DLLSPEC=Q_DECL_IMPORT
)

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

target_include_directories(rssguard-greader
  PUBLIC
  ${LIBRSSGUARD_SOURCE_PATH}
  src/3rd-party/richtexteditor
)

# Qt.
target_link_libraries(rssguard-greader PUBLIC
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
  install(TARGETS rssguard-greader DESTINATION plugins)
elseif(UNIX AND NOT APPLE AND NOT ANDROID)
  include (GNUInstallDirs)
  install(TARGETS rssguard-greader
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/rssguard
  )
elseif(APPLE)
  install(TARGETS rssguard-greader
    DESTINATION Contents/MacOS
  )
endif()
