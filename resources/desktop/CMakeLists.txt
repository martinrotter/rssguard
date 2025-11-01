set(APPDATA_NAME "${APP_NAME}")
set(APPDATA_SUMMARY "Simple, yet powerful news feed reader")

configure_file(
  rssguard.desktop.in
  ${CMAKE_BINARY_DIR}/${APP_REVERSE_NAME}.desktop
)
configure_file(
  rssguard.metainfo.xml.in
  ${CMAKE_BINARY_DIR}/${APP_REVERSE_NAME}.metainfo.xml
)

set(APPDATA_NAME ${APPDATA_NAME} PARENT_SCOPE)
set(APPDATA_SUMMARY ${APPDATA_SUMMARY} PARENT_SCOPE)
