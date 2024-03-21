include(../cmake_plugins.cmake)
set(PLUGIN_TARGET "rssguard-nextcloud")

set(SOURCES
  src/definitions.h
  src/gui/formeditnextcloudaccount.cpp
  src/gui/formeditnextcloudaccount.h
  src/gui/nextcloudaccountdetails.cpp
  src/gui/nextcloudaccountdetails.h
  src/nextcloudfeed.cpp
  src/nextcloudfeed.h
  src/nextcloudnetworkfactory.cpp
  src/nextcloudnetworkfactory.h
  src/nextcloudserviceentrypoint.cpp
  src/nextcloudserviceentrypoint.h
  src/nextcloudserviceroot.cpp
  src/nextcloudserviceroot.h
)

set(UI_FILES
  src/gui/nextcloudaccountdetails.ui
)

prepare_rssguard_plugin(${PLUGIN_TARGET})
