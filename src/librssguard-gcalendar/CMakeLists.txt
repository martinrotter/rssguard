include(../cmake_plugins.cmake)
set(PLUGIN_TARGET "rssguard-gcalendar")

set(SOURCES
  src/calendarevent.cpp
  src/calendarevent.h
  src/calendareventdraft.cpp
  src/calendareventdraft.h
  src/calendarreminders.cpp
  src/calendarreminders.h
  src/gcalendarentrypoint.cpp
  src/gcalendarentrypoint.h
  src/gcalendarfeed.cpp
  src/gcalendarfeed.h
  src/gcalendarnetworkfactory.cpp
  src/gcalendarnetworkfactory.h
  src/gcalendarserviceroot.cpp
  src/gcalendarserviceroot.h
  src/gui/calendareventpreviewer.cpp
  src/gui/calendareventpreviewer.h
  src/gui/formcalendarevent.cpp
  src/gui/formcalendarevent.h
  src/gui/formeditgcalendaraccount.cpp
  src/gui/formeditgcalendaraccount.h
  src/gui/formgcalendarfeeddetails.cpp
  src/gui/formgcalendarfeeddetails.h
  src/gui/gcalendaraccountdetails.cpp
  src/gui/gcalendaraccountdetails.h
  src/gui/gcalendarreminderdetails.cpp
  src/gui/gcalendarreminderdetails.h
)

set(UI_FILES
  src/gui/calendareventpreviewer.ui
  src/gui/formcalendarevent.ui
  src/gui/formcalendareventdetails.ui
  src/gui/gcalendaraccountdetails.ui
  src/gui/gcalendarremindersettings.ui
)

set(GCALENDAR_CLIENT_ID "" CACHE STRING "Google Calendar client ID")
set(GCALENDAR_CLIENT_SECRET "" CACHE STRING "Google Calendar client secret")

prepare_rssguard_plugin(${PLUGIN_TARGET})

if(NOT GCALENDAR_CLIENT_ID STREQUAL "" AND NOT GCALENDAR_CLIENT_SECRET STREQUAL "")
  target_compile_definitions(${PLUGIN_TARGET}
    PRIVATE

    GCALENDAR_OFFICIAL_SUPPORT
    GCALENDAR_CLIENT_ID="${GCALENDAR_CLIENT_ID}"
    GCALENDAR_CLIENT_SECRET="${GCALENDAR_CLIENT_SECRET}"
  )

  message(STATUS "Enabling official Google Calendar support.")
else()
  message(STATUS "Google Calendar client ID/secret variables are not set. Disabling official support.")
endif()
