include(../cmake_plugins.cmake)
set(PLUGIN_TARGET "rssguard-gmail")

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

set(GMAIL_CLIENT_ID "" CACHE STRING "GMail client ID")
set(GMAIL_CLIENT_SECRET "" CACHE STRING "GMail client secret")

prepare_rssguard_plugin(${PLUGIN_TARGET})

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
  src/3rd-party/richtexteditor
)
