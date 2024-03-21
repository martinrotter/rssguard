include(../cmake_plugins.cmake)
set(PLUGIN_TARGET "rssguard-standard")

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

prepare_rssguard_plugin(${PLUGIN_TARGET})

if(QT_VERSION_MAJOR EQUAL 6)
  target_link_libraries(${PLUGIN_TARGET} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core5Compat
  )
endif()

if(ZLIB_FOUND)
  target_include_directories(${PLUGIN_TARGET} AFTER
    PRIVATE
    ${ZLIB_INCLUDE_DIRS}
  )

  target_compile_definitions(${PLUGIN_TARGET}
    PRIVATE
    ENABLE_COMPRESSED_SITEMAP
  )

  target_link_libraries(${PLUGIN_TARGET} PRIVATE
    ${ZLIB_LIBRARIES}
  )
endif(ZLIB_FOUND)
