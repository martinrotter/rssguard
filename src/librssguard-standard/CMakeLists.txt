set(LIBRSSGUARD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../librssguard")

set(SOURCES
  src/definitions.h
  src/gui/formdiscoverfeeds.cpp
  src/gui/formdiscoverfeeds.h
  src/gui/formeditstandardaccount.cpp
  src/gui/formeditstandardaccount.h
  src/gui/formstandardfeeddetails.cpp
  src/gui/formstandardfeeddetails.h
  src/gui/formstandardimportexport.cpp
  src/gui/formstandardimportexport.h
  src/gui/standardaccountdetails.cpp
  src/gui/standardaccountdetails.h
  src/gui/standardfeeddetails.cpp
  src/gui/standardfeeddetails.h
  src/parsers/atomparser.cpp
  src/parsers/atomparser.h
  src/parsers/feedparser.cpp
  src/parsers/feedparser.h
  src/parsers/icalparser.cpp
  src/parsers/icalparser.h
  src/parsers/jsonparser.cpp
  src/parsers/jsonparser.h
  src/parsers/rdfparser.cpp
  src/parsers/rdfparser.h
  src/parsers/rssparser.cpp
  src/parsers/rssparser.h
  src/parsers/sitemapparser.cpp
  src/parsers/sitemapparser.h
  src/standardcategory.cpp
  src/standardcategory.h
  src/standardfeed.cpp
  src/standardfeed.h
  src/standardfeedsimportexportmodel.cpp
  src/standardfeedsimportexportmodel.h
  src/standardserviceentrypoint.cpp
  src/standardserviceentrypoint.h
  src/standardserviceroot.cpp
  src/standardserviceroot.h
)

set(UI_FILES
  src/gui/formdiscoverfeeds.ui
  src/gui/formstandardimportexport.ui
  src/gui/standardaccountdetails.ui
  src/gui/standardfeeddetails.ui
)

# Add mimesis.
list(APPEND SOURCES
  ${LIBRSSGUARD_PATH}/3rd-party/mimesis/mimesis.hpp
  ${LIBRSSGUARD_PATH}/3rd-party/mimesis/quoted-printable.hpp
)

# Add boolinq.
list(APPEND SOURCES
  ${LIBRSSGUARD_PATH}/3rd-party/boolinq/boolinq.h
)

# Deal with .ui files.
qt_wrap_ui(SOURCES ${UI_FILES})

# Bundle version info.
if(WIN32)
  enable_language("RC")
  list(APPEND SOURCES "${CMAKE_BINARY_DIR}/rssguard.rc")
endif()

add_library(rssguard-standard SHARED ${SOURCES} ${QM_FILES})

# Add specific definitions.
target_compile_definitions(rssguard-standard
  PRIVATE
  RSSGUARD_DLLSPEC=Q_DECL_IMPORT
  RSSGUARD_DLLSPEC_EXPORT=Q_DECL_EXPORT

  APP_AUTHOR="${APP_AUTHOR}"
  APP_DONATE_URL="${APP_DONATE_URL}"
  APP_EMAIL="${APP_EMAIL}"
  APP_LOW_H_NAME=".${CMAKE_PROJECT_NAME}"
  APP_REVISION="${APP_REVISION}"
  APP_SYSTEM_NAME="${CMAKE_SYSTEM_NAME}"
  APP_SYSTEM_VERSION="${CMAKE_SYSTEM_PROCESSOR}"

  APP_URL_DOCUMENTATION="${APP_URL_DOCUMENTATION}"
  APP_URL_ISSUES_NEW="${APP_URL_ISSUES_NEW}"
  APP_USERAGENT="${APP_NAME}/${CMAKE_PROJECT_VERSION}")

target_include_directories(rssguard-standard
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/src/librssguard
  ${LIBRSSGUARD_PATH}
  ${LIBRSSGUARD_PATH}/gui/dialogs
  ${LIBRSSGUARD_PATH}/gui/reusable
  ${LIBRSSGUARD_PATH}/services/abstract/gui
  ${LIBRSSGUARD_PATH}/dynamic-shortcuts

  PRIVATE
  ${LIBRSSGUARD_PATH}/gui/notifications
  ${LIBRSSGUARD_PATH}/gui/toolbars
  ${LIBRSSGUARD_PATH}/gui/richtexteditor
)

# Qt.
target_link_libraries(rssguard-standard PUBLIC
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
  target_link_libraries(rssguard-standard PUBLIC
    Qt${QT_VERSION_MAJOR}::Core5Compat
  )
endif()

if(WIN32)
  target_link_libraries(rssguard-standard PUBLIC
    Qt${QT_VERSION_MAJOR}::GuiPrivate
  )
endif()

if(NO_LITE)
  target_link_libraries(rssguard-standard PUBLIC
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
  )
endif()

if(WIN32 OR OS2)
  install(TARGETS rssguard-standard DESTINATION .)
elseif(UNIX AND NOT APPLE AND NOT ANDROID)
  include (GNUInstallDirs)
  install(TARGETS rssguard-standard
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
elseif(APPLE)
  install(TARGETS rssguard-standard
    DESTINATION Contents/MacOS
  )
endif()
