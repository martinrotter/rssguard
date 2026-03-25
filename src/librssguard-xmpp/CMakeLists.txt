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

if(NOT USE_SYSTEM_QXMPP)
  # This will look for prepared library qxmpp in <root>/resources/scripts/qxmpp
  # and for prepared header files in <root>/resources/scripts/qxmpp/include
  set(qxmpp_ROOT "${CMAKE_SOURCE_DIR}/resources/scripts/qxmpp")
  message(STATUS "Using compiled qxmpp.")
endif()

find_library(qxmpp_LIBRARIES
  NAMES "QXmpp" "QXmppQt${QT_VERSION_MAJOR}"
  HINTS "${qxmpp_ROOT}" "${qxmpp_ROOT}/lib"
  NO_CACHE
  REQUIRED
)

find_path(qxmpp_INCLUDE_DIRS
  NAMES "QXmppClient.h"
  PATH_SUFFIXES "include" "include/QXmppQt${QT_VERSION_MAJOR}"
  HINTS "${qxmpp_ROOT}"
  NO_CACHE
  REQUIRED
)

message(STATUS "Found qxmpp lib: '${qxmpp_LIBRARIES}'.")
message(STATUS "Found qxmpp headers: '${qxmpp_INCLUDE_DIRS}'.")

#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

prepare_rssguard_plugin(${PLUGIN_TARGET})

target_link_libraries(rssguard-xmpp PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_features(${PLUGIN_TARGET} PRIVATE cxx_std_20)

target_include_directories(${PLUGIN_TARGET} AFTER
  PRIVATE
  ${qxmpp_INCLUDE_DIRS}
)

target_link_libraries(${PLUGIN_TARGET} PRIVATE
  ${qxmpp_LIBRARIES}
)
