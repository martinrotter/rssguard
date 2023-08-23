set(APPDATA_NAME_REGULAR ${APP_NAME})
set(APPDATA_NAME_LITE "${APP_NAME} Lite")
set(APPDATA_SUMMARY_REGULAR "Simple, yet powerful news feed reader")
set(APPDATA_SUMMARY_LITE "${APPDATA_SUMMARY_REGULAR} (no built-in web browser)")

if(USE_WEBENGINE)
  set(APPDATA_NAME "${APPDATA_NAME_REGULAR}")
  set(APPDATA_SUMMARY "${APPDATA_SUMMARY_REGULAR}")
else()
  set(APPDATA_NAME "${APPDATA_NAME_LITE}")
  set(APPDATA_SUMMARY "${APPDATA_SUMMARY_LITE}")
endif()

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
