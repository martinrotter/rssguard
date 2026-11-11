include(../cmake_plugins.cmake)
set(PLUGIN_TARGET "rssguard-xmpp")

set(SOURCES
  src/xmppentrypoint.cpp
  src/xmppentrypoint.h
  src/xmppcategory.cpp
  src/xmppcategory.h
  src/xmppfeed.cpp
  src/xmppfeed.h
  src/xmppnetwork.cpp
  src/xmppnetwork.h
  src/xmppserviceroot.cpp
  src/xmppserviceroot.h
  src/xmppubsubpmanager.cpp
  src/xmppubsubpmanager.h
  src/parsers/atomparser.cpp
  src/parsers/atomparser.h
  src/parsers/feedparser.cpp
  src/parsers/feedparser.h

  src/gui/formeditxmppaccount.cpp
  src/gui/formeditxmppaccount.h
  src/gui/xmppaccountdetails.cpp
  src/gui/xmppaccountdetails.h
)

set(UI_FILES
  src/gui/xmppaccountdetails.ui
)

if(NOT USE_SYSTEM_QXMPP AND NOT qxmpp_ROOT)
  # This will look for prepared library qxmpp in '<root>/resources/scripts/qxmpp'
  # and for prepared header files in '<root>/resources/scripts/qxmpp/include'.
  set(qxmpp_ROOT "${CMAKE_SOURCE_DIR}/resources/scripts/qxmpp")
  message(STATUS "Using compiled qxmpp.")
endif()

# Use the exported target so headers, import library and Qt dependencies belong
# to the same QXmpp installation. A raw DLL/library search can silently pair
# incompatible Qt 5/6 builds or headers from a different QXmpp release.
find_package(QXmppQt6 1.15 CONFIG REQUIRED HINTS "${qxmpp_ROOT}")
set_property(TARGET QXmpp::QXmpp PROPERTY IMPORTED_GLOBAL TRUE)

prepare_rssguard_plugin(${PLUGIN_TARGET})

target_link_libraries(${PLUGIN_TARGET} PRIVATE QXmpp::QXmpp)
target_compile_features(${PLUGIN_TARGET} PRIVATE cxx_std_20)
