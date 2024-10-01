include(../cmake_plugins.cmake)
set(PLUGIN_TARGET "rssguard-reddit")

set(SOURCES
  src/definitions.h
  src/gui/formeditredditaccount.cpp
  src/gui/formeditredditaccount.h
  src/gui/redditaccountdetails.cpp
  src/gui/redditaccountdetails.h
  src/redditcategory.cpp
  src/redditcategory.h
  src/redditentrypoint.cpp
  src/redditentrypoint.h
  src/redditnetworkfactory.cpp
  src/redditnetworkfactory.h
  src/redditserviceroot.cpp
  src/redditserviceroot.h
  src/redditsubscription.cpp
  src/redditsubscription.h

  src/3rd-party/mimesis/mimesis.cpp
  src/3rd-party/mimesis/mimesis.hpp
  src/3rd-party/mimesis/quoted-printable.cpp
  src/3rd-party/mimesis/quoted-printable.hpp
)

set(UI_FILES
  src/gui/redditaccountdetails.ui 
)

prepare_rssguard_plugin(${PLUGIN_TARGET})
