if(NOT DEFINED LIBRSSGUARD_BINARY_PATH)
  set(LIBRSSGUARD_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

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

# Add ZLIB.
if(ENABLE_COMPRESSED_SITEMAP)
  find_package(ZLIB REQUIRED)

  # Add qcompressor.
  list(APPEND SOURCES
    src/3rd-party/qcompressor/qcompressor.cpp
    src/3rd-party/qcompressor/qcompressor.h
  )
endif(ENABLE_COMPRESSED_SITEMAP)

if(ZLIB_FOUND)
  message(STATUS "Using system zlib ${ZLIB_VERSION_STRING}.")
endif(ZLIB_FOUND)

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
)

target_include_directories(rssguard-standard
  PUBLIC
  ${LIBRSSGUARD_SOURCE_PATH}
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

if(ZLIB_FOUND)
  target_include_directories(rssguard-standard AFTER
    PRIVATE
    ${ZLIB_INCLUDE_DIRS}
  )

  target_compile_definitions(rssguard-standard
    PRIVATE
    ENABLE_COMPRESSED_SITEMAP
  )

  target_link_libraries(rssguard-standard PRIVATE
    ${ZLIB_LIBRARIES}
  )
endif(ZLIB_FOUND)

if(WIN32 OR OS2)
  install(TARGETS rssguard-standard DESTINATION plugins)
elseif(UNIX AND NOT APPLE AND NOT ANDROID)
  include (GNUInstallDirs)
  install(TARGETS rssguard-standard
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/rssguard
  )
elseif(APPLE)
  install(TARGETS rssguard-standard
    DESTINATION Contents/MacOS
  )
endif()
