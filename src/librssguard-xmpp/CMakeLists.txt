include(../cmake_plugins.cmake)
set(PLUGIN_TARGET "rssguard-xmpp")

set(SOURCES
  src/xmppentrypoint.cpp
  src/xmppentrypoint.h
  src/xmppfeed.cpp
  src/xmppfeed.h
  src/xmppnetwork.cpp
  src/xmppnetwork.h
  src/xmppserviceroot.cpp
  src/xmppserviceroot.h
  src/xmppubsubpmanager.cpp
  src/xmppubsubpmanager.h
)

set(UI_FILES
)

prepare_rssguard_plugin(${PLUGIN_TARGET})