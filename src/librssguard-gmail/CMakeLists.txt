if(NOT DEFINED LIBRSSGUARD_BINARY_PATH)
  set(LIBRSSGUARD_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

set(SOURCES
  src/definitions.h
  src/gmailentrypoint.cpp
  src/gmailentrypoint.h
  src/gmailnetworkfactory.cpp
  src/gmailnetworkfactory.h
  src/gmailserviceroot.cpp
  src/gmailserviceroot.h
  src/gui/emailpreviewer.cpp
  src/gui/emailpreviewer.h
  src/gui/emailrecipientcontrol.cpp
  src/gui/emailrecipientcontrol.h
  src/gui/formaddeditemail.cpp
  src/gui/formaddeditemail.h
  src/gui/formeditgmailaccount.cpp
  src/gui/formeditgmailaccount.h
  src/gui/gmailaccountdetails.cpp
  src/gui/gmailaccountdetails.h

  src/3rd-party/mimesis/mimesis.cpp
  src/3rd-party/mimesis/mimesis.hpp
  src/3rd-party/mimesis/quoted-printable.cpp
  src/3rd-party/mimesis/quoted-printable.hpp

  src/3rd-party/richtexteditor/mrichtextedit.cpp
  src/3rd-party/richtexteditor/mrichtextedit.h
  src/3rd-party/richtexteditor/mtextedit.cpp
  src/3rd-party/richtexteditor/mtextedit.h
)

set(UI_FILES
  src/gui/emailpreviewer.ui
  src/gui/formaddeditemail.ui
  src/gui/gmailaccountdetails.ui
  src/3rd-party/richtexteditor/mrichtextedit.ui
)

# Deal with .ui files.
qt_wrap_ui(SOURCES ${UI_FILES})

set(GMAIL_CLIENT_ID "" CACHE STRING "GMail client ID")
set(GMAIL_CLIENT_SECRET "" CACHE STRING "GMail client secret")

# Bundle version info.
if(WIN32)
  enable_language("RC")
  list(APPEND SOURCES "${CMAKE_BINARY_DIR}/rssguard.rc")
endif()

add_library(rssguard-gmail SHARED ${SOURCES} ${QM_FILES})

# Add specific definitions.
target_compile_definitions(rssguard-gmail
  PRIVATE
  RSSGUARD_DLLSPEC=Q_DECL_IMPORT
  RSSGUARD_DLLSPEC_EXPORT=Q_DECL_EXPORT
)

if(NOT GMAIL_CLIENT_ID STREQUAL "" AND NOT GMAIL_CLIENT_SECRET STREQUAL "")
  target_compile_definitions(rssguard-gmail
    PRIVATE

    GMAIL_OFFICIAL_SUPPORT
    GMAIL_CLIENT_ID="${GMAIL_CLIENT_ID}"
    GMAIL_CLIENT_SECRET="${GMAIL_CLIENT_SECRET}"
  )

  message(STATUS "Enabling official Gmail support.")
else()
  message(STATUS "Gmail client ID/secret variables are not set. Disabling official support.")
endif()

target_include_directories(rssguard-gmail
  PUBLIC
  ${LIBRSSGUARD_SOURCE_PATH}
  src/3rd-party/richtexteditor
)

# Qt.
target_link_libraries(rssguard-gmail PUBLIC
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
  install(TARGETS rssguard-gmail DESTINATION plugins)
elseif(UNIX AND NOT APPLE AND NOT ANDROID)
  include (GNUInstallDirs)
  install(TARGETS rssguard-gmail
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/rssguard
  )
elseif(APPLE)
  install(TARGETS rssguard-gmail
    DESTINATION Contents/MacOS
  )
endif()
