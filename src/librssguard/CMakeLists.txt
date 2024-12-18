set(SOURCES
  core/articlelistnotificationmodel.cpp
  core/articlelistnotificationmodel.h
  core/feeddownloader.cpp
  core/feeddownloader.h
  core/feedsmodel.cpp
  core/feedsmodel.h
  core/feedsproxymodel.cpp
  core/feedsproxymodel.h
  core/filterutils.cpp
  core/filterutils.h
  core/message.cpp
  core/message.h
  core/messagefilter.cpp
  core/messagefilter.h
  core/messageobject.cpp
  core/messageobject.h
  core/messagesforfiltersmodel.cpp
  core/messagesforfiltersmodel.h
  core/messagesmodel.cpp
  core/messagesmodel.h
  core/messagesmodelcache.cpp
  core/messagesmodelcache.h
  core/messagesmodelsqllayer.cpp
  core/messagesmodelsqllayer.h
  core/messagesproxymodel.cpp
  core/messagesproxymodel.h
  database/databasecleaner.cpp
  database/databasecleaner.h
  database/databasedriver.cpp
  database/databasedriver.h
  database/databasefactory.cpp
  database/databasefactory.h
  database/databasequeries.cpp
  database/databasequeries.h
  database/mariadbdriver.cpp
  database/mariadbdriver.h
  database/sqlitedriver.cpp
  database/sqlitedriver.h
  definitions/definitions.h
  definitions/globals.cpp
  definitions/globals.h
  definitions/typedefs.h
  dynamic-shortcuts/dynamicshortcuts.cpp
  dynamic-shortcuts/dynamicshortcuts.h
  dynamic-shortcuts/dynamicshortcutswidget.cpp
  dynamic-shortcuts/dynamicshortcutswidget.h
  dynamic-shortcuts/shortcutcatcher.cpp
  dynamic-shortcuts/shortcutcatcher.h
  exceptions/applicationexception.cpp
  exceptions/applicationexception.h
  exceptions/feedfetchexception.cpp
  exceptions/feedfetchexception.h
  exceptions/feedrecognizedbutfailedexception.cpp
  exceptions/feedrecognizedbutfailedexception.h
  exceptions/filteringexception.cpp
  exceptions/filteringexception.h
  exceptions/ioexception.cpp
  exceptions/ioexception.h
  exceptions/networkexception.cpp
  exceptions/networkexception.h
  exceptions/processexception.cpp
  exceptions/processexception.h
  exceptions/scriptexception.cpp
  exceptions/scriptexception.h
  gui/dialogs/formabout.cpp
  gui/dialogs/formabout.h
  gui/dialogs/formaddaccount.cpp
  gui/dialogs/formaddaccount.h
  gui/dialogs/formbackupdatabasesettings.cpp
  gui/dialogs/formbackupdatabasesettings.h
  gui/dialogs/formdatabasecleanup.cpp
  gui/dialogs/formdatabasecleanup.h
  gui/dialogs/formlog.cpp
  gui/dialogs/formlog.h
  gui/dialogs/formmain.cpp
  gui/dialogs/formmain.h
  gui/dialogs/formmessagefiltersmanager.cpp
  gui/dialogs/formmessagefiltersmanager.h
  gui/dialogs/formrestoredatabasesettings.cpp
  gui/dialogs/formrestoredatabasesettings.h
  gui/dialogs/formsettings.cpp
  gui/dialogs/formsettings.h
  gui/dialogs/formupdate.cpp
  gui/dialogs/formupdate.h
  gui/feedmessageviewer.cpp
  gui/feedmessageviewer.h
  gui/feedsview.cpp
  gui/feedsview.h
  gui/guiutilities.cpp
  gui/guiutilities.h
  gui/itemdetails.cpp
  gui/itemdetails.h
  gui/messagebox.cpp
  gui/messagebox.h
  gui/messagepreviewer.cpp
  gui/messagepreviewer.h
  gui/messagesview.cpp
  gui/messagesview.h
  gui/notifications/articlelistnotification.cpp
  gui/notifications/articlelistnotification.h
  gui/notifications/basetoastnotification.cpp
  gui/notifications/basetoastnotification.h
  gui/notifications/notificationseditor.cpp
  gui/notifications/notificationseditor.h
  gui/notifications/singlenotificationeditor.cpp
  gui/notifications/singlenotificationeditor.h
  gui/notifications/toastnotification.cpp
  gui/notifications/toastnotification.h
  gui/notifications/toastnotificationsmanager.cpp
  gui/notifications/toastnotificationsmanager.h
  gui/reusable/articleamountcontrol.cpp
  gui/reusable/articleamountcontrol.h
  gui/reusable/baselineedit.cpp
  gui/reusable/baselineedit.h
  gui/reusable/basetreeview.cpp
  gui/reusable/basetreeview.h
  gui/reusable/colortoolbutton.cpp
  gui/reusable/colortoolbutton.h
  gui/reusable/comboboxwithstatus.cpp
  gui/reusable/comboboxwithstatus.h
  gui/reusable/edittableview.cpp
  gui/reusable/edittableview.h
  gui/reusable/helpspoiler.cpp
  gui/reusable/helpspoiler.h
  gui/reusable/jssyntaxhighlighter.cpp
  gui/reusable/jssyntaxhighlighter.h
  gui/reusable/labelsmenu.cpp
  gui/reusable/labelsmenu.h
  gui/reusable/labelwithstatus.cpp
  gui/reusable/labelwithstatus.h
  gui/reusable/lineeditwithstatus.cpp
  gui/reusable/lineeditwithstatus.h
  gui/reusable/locationlineedit.cpp
  gui/reusable/locationlineedit.h
  gui/reusable/messagecountspinbox.cpp
  gui/reusable/messagecountspinbox.h
  gui/reusable/networkproxydetails.cpp
  gui/reusable/networkproxydetails.h
  gui/reusable/nonclosablemenu.cpp
  gui/reusable/nonclosablemenu.h
  gui/reusable/plaintoolbutton.cpp
  gui/reusable/plaintoolbutton.h
  gui/reusable/progressbarwithtext.cpp
  gui/reusable/progressbarwithtext.h
  gui/reusable/resizablestackedwidget.cpp
  gui/reusable/resizablestackedwidget.h
  gui/reusable/searchlineedit.cpp
  gui/reusable/searchlineedit.h
  gui/reusable/searchtextwidget.cpp
  gui/reusable/searchtextwidget.h
  gui/reusable/squeezelabel.cpp
  gui/reusable/squeezelabel.h
  gui/reusable/styleditemdelegatewithoutfocus.cpp
  gui/reusable/styleditemdelegatewithoutfocus.h
  gui/reusable/timespinbox.cpp
  gui/reusable/timespinbox.h
  gui/reusable/treeviewcolumnsmenu.cpp
  gui/reusable/treeviewcolumnsmenu.h
  gui/reusable/widgetwithstatus.cpp
  gui/reusable/widgetwithstatus.h
  gui/settings/settingsbrowsermail.cpp
  gui/settings/settingsbrowsermail.h
  gui/settings/settingsdatabase.cpp
  gui/settings/settingsdatabase.h
  gui/settings/settingsdownloads.cpp
  gui/settings/settingsdownloads.h
  gui/settings/settingsfeedsmessages.cpp
  gui/settings/settingsfeedsmessages.h
  gui/settings/settingsgeneral.cpp
  gui/settings/settingsgeneral.h
  gui/settings/settingsgui.cpp
  gui/settings/settingsgui.h
  gui/settings/settingslocalization.cpp
  gui/settings/settingslocalization.h
  gui/settings/settingsmediaplayer.cpp
  gui/settings/settingsmediaplayer.h
  gui/settings/settingsnodejs.cpp
  gui/settings/settingsnodejs.h
  gui/settings/settingsnotifications.cpp
  gui/settings/settingsnotifications.h
  gui/settings/settingspanel.cpp
  gui/settings/settingspanel.h
  gui/settings/settingsshortcuts.cpp
  gui/settings/settingsshortcuts.h
  gui/systemtrayicon.cpp
  gui/systemtrayicon.h
  gui/tabbar.cpp
  gui/tabbar.h
  gui/tabcontent.cpp
  gui/tabcontent.h
  gui/tabwidget.cpp
  gui/tabwidget.h
  gui/toolbars/basetoolbar.cpp
  gui/toolbars/basetoolbar.h
  gui/toolbars/feedstoolbar.cpp
  gui/toolbars/feedstoolbar.h
  gui/toolbars/messagestoolbar.cpp
  gui/toolbars/messagestoolbar.h
  gui/toolbars/statusbar.cpp
  gui/toolbars/statusbar.h
  gui/toolbars/toolbareditor.cpp
  gui/toolbars/toolbareditor.h
  gui/webbrowser.cpp
  gui/webbrowser.h
  gui/webviewers/webviewer.cpp
  gui/webviewers/webviewer.h
  miscellaneous/application.cpp
  miscellaneous/application.h
  miscellaneous/autosaver.cpp
  miscellaneous/autosaver.h
  miscellaneous/externaltool.cpp
  miscellaneous/externaltool.h
  miscellaneous/feedreader.cpp
  miscellaneous/feedreader.h
  miscellaneous/iconfactory.cpp
  miscellaneous/iconfactory.h
  miscellaneous/iofactory.cpp
  miscellaneous/iofactory.h
  miscellaneous/localization.cpp
  miscellaneous/localization.h
  miscellaneous/mutex.cpp
  miscellaneous/mutex.h
  miscellaneous/nodejs.cpp
  miscellaneous/nodejs.h
  miscellaneous/notification.cpp
  miscellaneous/notification.h
  miscellaneous/notificationfactory.cpp
  miscellaneous/notificationfactory.h
  miscellaneous/pluginfactory.cpp
  miscellaneous/pluginfactory.h
  miscellaneous/regexfactory.cpp
  miscellaneous/regexfactory.h
  miscellaneous/settings.cpp
  miscellaneous/settings.h
  miscellaneous/settingsproperties.h
  miscellaneous/singleapplication.cpp
  miscellaneous/singleapplication.h
  miscellaneous/skinfactory.cpp
  miscellaneous/skinfactory.h
  miscellaneous/systemfactory.cpp
  miscellaneous/systemfactory.h
  miscellaneous/templates.h
  miscellaneous/textfactory.cpp
  miscellaneous/textfactory.h
  miscellaneous/thread.cpp
  miscellaneous/thread.h
  network-web/adblock/adblockdialog.cpp
  network-web/adblock/adblockdialog.h
  network-web/adblock/adblockicon.cpp
  network-web/adblock/adblockicon.h
  network-web/adblock/adblockmanager.cpp
  network-web/adblock/adblockmanager.h
  network-web/adblock/adblockrequestinfo.cpp
  network-web/adblock/adblockrequestinfo.h
  network-web/gemini/geminiclient.cpp
  network-web/gemini/geminiclient.h
  network-web/gemini/geminiparser.cpp
  network-web/gemini/geminiparser.h
  network-web/apiserver.cpp
  network-web/apiserver.h
  network-web/articleparse.cpp
  network-web/articleparse.h
  network-web/basenetworkaccessmanager.cpp
  network-web/basenetworkaccessmanager.h
  network-web/cookiejar.cpp
  network-web/cookiejar.h
  network-web/downloader.cpp
  network-web/downloader.h
  network-web/downloadmanager.cpp
  network-web/downloadmanager.h
  network-web/googlesuggest.cpp
  network-web/googlesuggest.h
  network-web/httpresponse.cpp
  network-web/httpresponse.h
  network-web/httpserver.cpp
  network-web/httpserver.h
  network-web/networkfactory.cpp
  network-web/networkfactory.h
  network-web/oauth2service.cpp
  network-web/oauth2service.h
  network-web/oauthhttphandler.cpp
  network-web/oauthhttphandler.h
  network-web/readability.cpp
  network-web/readability.h
  network-web/silentnetworkaccessmanager.cpp
  network-web/silentnetworkaccessmanager.h
  network-web/webfactory.cpp
  network-web/webfactory.h
  services/abstract/accountcheckmodel.cpp
  services/abstract/accountcheckmodel.h
  services/abstract/cacheforserviceroot.cpp
  services/abstract/cacheforserviceroot.h
  services/abstract/category.cpp
  services/abstract/category.h
  services/abstract/feed.cpp
  services/abstract/feed.h
  services/abstract/gui/accountdetails.cpp
  services/abstract/gui/accountdetails.h
  services/abstract/gui/authenticationdetails.cpp
  services/abstract/gui/authenticationdetails.h
  services/abstract/gui/custommessagepreviewer.cpp
  services/abstract/gui/custommessagepreviewer.h
  services/abstract/gui/formaccountdetails.cpp
  services/abstract/gui/formaccountdetails.h
  services/abstract/gui/formaddeditlabel.cpp
  services/abstract/gui/formaddeditlabel.h
  services/abstract/gui/formaddeditprobe.cpp
  services/abstract/gui/formaddeditprobe.h
  services/abstract/gui/formcategorydetails.cpp
  services/abstract/gui/formcategorydetails.h
  services/abstract/gui/formfeeddetails.cpp
  services/abstract/gui/formfeeddetails.h
  services/abstract/gui/httpheadersdetails.cpp
  services/abstract/gui/httpheadersdetails.h
  services/abstract/gui/multifeededitcheckbox.cpp
  services/abstract/gui/multifeededitcheckbox.h
  services/abstract/importantnode.cpp
  services/abstract/importantnode.h
  services/abstract/label.cpp
  services/abstract/label.h
  services/abstract/labelsnode.cpp
  services/abstract/labelsnode.h
  services/abstract/recyclebin.cpp
  services/abstract/recyclebin.h
  services/abstract/rootitem.cpp
  services/abstract/rootitem.h
  services/abstract/search.cpp
  services/abstract/search.h
  services/abstract/searchsnode.cpp
  services/abstract/searchsnode.h
  services/abstract/serviceentrypoint.h
  services/abstract/serviceroot.cpp
  services/abstract/serviceroot.h
  services/abstract/unreadnode.cpp
  services/abstract/unreadnode.h
)

set(UI_FILES
  gui/dialogs/formabout.ui
  gui/dialogs/formaddaccount.ui
  gui/dialogs/formbackupdatabasesettings.ui
  gui/dialogs/formdatabasecleanup.ui
  gui/dialogs/formlog.ui
  gui/dialogs/formmain.ui
  gui/dialogs/formmessagefiltersmanager.ui
  gui/dialogs/formrestoredatabasesettings.ui
  gui/dialogs/formsettings.ui
  gui/dialogs/formupdate.ui
  gui/itemdetails.ui
  gui/notifications/articlelistnotification.ui
  gui/notifications/notificationseditor.ui
  gui/notifications/singlenotificationeditor.ui
  gui/notifications/toastnotification.ui
  gui/reusable/articleamountcontrol.ui
  gui/reusable/networkproxydetails.ui
  gui/reusable/searchtextwidget.ui
  gui/settings/settingsbrowsermail.ui
  gui/settings/settingsdatabase.ui
  gui/settings/settingsdownloads.ui
  gui/settings/settingsfeedsmessages.ui
  gui/settings/settingsgeneral.ui
  gui/settings/settingsgui.ui
  gui/settings/settingslocalization.ui
  gui/settings/settingsmediaplayer.ui
  gui/settings/settingsnodejs.ui
  gui/settings/settingsnotifications.ui
  gui/settings/settingsshortcuts.ui
  gui/toolbars/toolbareditor.ui
  network-web/adblock/adblockdialog.ui
  network-web/downloaditem.ui
  network-web/downloadmanager.ui
  services/abstract/gui/accountdetails.ui
  services/abstract/gui/authenticationdetails.ui
  services/abstract/gui/formaccountdetails.ui
  services/abstract/gui/formaddeditlabel.ui
  services/abstract/gui/formaddeditprobe.ui
  services/abstract/gui/formcategorydetails.ui
  services/abstract/gui/formfeeddetails.ui
  services/abstract/gui/httpheadersdetails.ui
)

if(ENABLE_MEDIAPLAYER)
  list(APPEND SOURCES
    gui/mediaplayer/playerbackend.cpp
    gui/mediaplayer/playerbackend.h
    gui/mediaplayer/mediaplayer.cpp
    gui/mediaplayer/mediaplayer.h
  )

  list(APPEND UI_FILES
    gui/mediaplayer/mediaplayer.ui
  )
endif()

if(ENABLE_MEDIAPLAYER_QTMULTIMEDIA)
  list(APPEND SOURCES
    gui/mediaplayer/qtmultimedia/qtmultimediabackend.cpp
    gui/mediaplayer/qtmultimedia/qtmultimediabackend.h
  )
elseif(ENABLE_MEDIAPLAYER_LIBMPV)
  list(APPEND SOURCES
    gui/mediaplayer/libmpv/libmpvbackend.cpp
    gui/mediaplayer/libmpv/libmpvbackend.h
    gui/mediaplayer/libmpv/libmpvwidget.cpp
    gui/mediaplayer/libmpv/libmpvwidget.h
    gui/mediaplayer/libmpv/qthelper.h
  )

  if(WIN32)
    # Tweak finding of libs to make it work with official libmpv libs.
    list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a")
  endif()

  # Search libmpv.
  find_library(LibMPV_LIBRARIES
    NAMES "mpv" "libmpv" "libmpv-v2"
    HINTS "${LibMPV_ROOT}"
    NO_CACHE
    REQUIRED
  )

  find_path(LibMPV_INCLUDE_DIRS
    NAMES "mpv/client.h"
    PATH_SUFFIXES "include"
    HINTS "${LibMPV_ROOT}"
    NO_CACHE
    REQUIRED
  )

  message(STATUS "Found libmpv: ${LibMPV_LIBRARIES}.")
endif()

if(NO_LITE)
  list(APPEND SOURCES
    # WebEngine-based web (and message) browser.
    gui/webviewers/webengine/webengineviewer.cpp
    gui/webviewers/webengine/webengineviewer.h
    network-web/adblock/adblockurlinterceptor.cpp
    network-web/adblock/adblockurlinterceptor.h
    network-web/webengine/networkurlinterceptor.cpp
    network-web/webengine/networkurlinterceptor.h
    network-web/webengine/urlinterceptor.h
    network-web/webengine/webenginepage.cpp
    network-web/webengine/webenginepage.h
  )
endif()

list(APPEND SOURCES
  # QTextBrowser.
  gui/webviewers/qtextbrowser/textbrowserviewer.h
  gui/webviewers/qtextbrowser/textbrowserviewer.cpp
)

# Add boolinq.
list(APPEND SOURCES
  3rd-party/boolinq/boolinq.h
)

# Add sqlite.
if(USE_SYSTEM_SQLITE)
  find_package(SQLite3)

  if(SQLite3_FOUND)
    # Include directory and library are linked to below.
    message(STATUS "Using system SQLite3 ${SQLite3_VERSION}.")
  else()
    message(FATAL_ERROR "System SQLite3 not detected. Set USE_SYSTEM_SQLITE=OFF if you want to build with bundled SQLite3.")
  endif()
else()
  message(STATUS "Using bundled SQLite3.")

  list(APPEND SOURCES
    3rd-party/sqlite/sqlite3.c
    3rd-party/sqlite/sqlite3.h
  )

  set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -DSQLITE_THREADSAFE=1 -DSQLITE_ENABLE_COLUMN_METADATA=1")
endif()

# Add SimpleCrypt.
list(APPEND SOURCES
  3rd-party/sc/simplecrypt.cpp
  3rd-party/sc/simplecrypt.h
)

# Deal with .ui files.
qt_wrap_ui(SOURCES ${UI_FILES})

# Bundle resources.
qt_add_resources(SOURCES ${CMAKE_SOURCE_DIR}/resources/sql.qrc)
qt_add_resources(SOURCES ${CMAKE_SOURCE_DIR}/resources/rssguard.qrc)

if(APPLE OR WIN32 OR OS2 OR FORCE_BUNDLE_ICONS)
  qt_add_resources(SOURCES ${CMAKE_SOURCE_DIR}/resources/icons.qrc)
endif()

# Bundle version info.
if(WIN32)
  enable_language("RC")
  list(APPEND SOURCES "${CMAKE_BINARY_DIR}/rssguard.rc")
endif()

add_library(rssguard SHARED ${SOURCES} ${QM_FILES})

# Bundle icons on some platforms which do not provide system-wide icon themes.
if(FORCE_BUNDLE_ICONS)
  target_compile_definitions(rssguard
    PRIVATE
    FORCE_BUNDLE_ICONS
  )

  message(STATUS "Forcibly bundling icon themes.")
endif()

target_include_directories(rssguard
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/gui
  ${CMAKE_CURRENT_SOURCE_DIR}/gui/dialogs
  ${CMAKE_CURRENT_SOURCE_DIR}/gui/reusable
  ${CMAKE_CURRENT_SOURCE_DIR}/services/abstract/gui
  ${CMAKE_CURRENT_SOURCE_DIR}/dynamic-shortcuts

  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/gui/notifications
  ${CMAKE_CURRENT_SOURCE_DIR}/gui/toolbars
  ${CMAKE_CURRENT_SOURCE_DIR}/gui/richtexteditor
)

if(SQLite3_FOUND)
  target_include_directories(rssguard AFTER
    PRIVATE
    ${SQLite3_INCLUDE_DIRS}
  )

  target_compile_definitions(rssguard
    PRIVATE

    SYSTEM_SQLITE3
  )

  target_link_libraries(rssguard PRIVATE
    ${SQLite3_LIBRARIES}
  )
endif()

# Qt.
target_link_libraries(rssguard PUBLIC
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
  target_link_libraries(rssguard PUBLIC
    Qt${QT_VERSION_MAJOR}::Core5Compat
  )
endif()

if(WIN32)
  target_link_libraries(rssguard PUBLIC
    Qt${QT_VERSION_MAJOR}::GuiPrivate
  )
endif()

if(NO_LITE)
  target_link_libraries(rssguard PUBLIC
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
  )
endif()

if(WIN32 AND NOT BUILD_WITH_QT6)
  target_link_libraries(rssguard PUBLIC
    Qt${QT_VERSION_MAJOR}::WinExtras
  )
endif()

if(NOT OS2)
  target_link_libraries(rssguard PUBLIC
    Qt${QT_VERSION_MAJOR}::Multimedia
  )
endif()

if(ENABLE_MEDIAPLAYER_QTMULTIMEDIA)
  target_link_libraries(rssguard PUBLIC
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
  )
elseif(ENABLE_MEDIAPLAYER_LIBMPV)
  if(MEDIAPLAYER_FORCE_OPENGL)
    target_link_libraries(rssguard PUBLIC
      Qt${QT_VERSION_MAJOR}::OpenGL
    )

    if(BUILD_WITH_QT6)
      target_link_libraries(rssguard PUBLIC
        Qt${QT_VERSION_MAJOR}::OpenGLWidgets
      )
    endif()
  endif()

  target_include_directories(rssguard AFTER
    PRIVATE
    ${LibMPV_INCLUDE_DIRS}
  )

  target_link_libraries(rssguard PUBLIC
    ${LibMPV_LIBRARIES}
  )
endif()

if(UNIX AND NOT APPLE AND NOT ANDROID)
  target_link_libraries(rssguard PUBLIC
    Qt${QT_VERSION_MAJOR}::DBus
  )
endif()

if(APPLE)
  target_link_libraries(rssguard PUBLIC
    "-framework AppKit"
  )
elseif(WIN32)
  target_link_libraries(rssguard PUBLIC
    shell32
    odbc32
  )
endif()

if((WIN32 AND NOT BUILD_MSYS2) OR OS2)
  install(TARGETS rssguard DESTINATION .)
  set(HEADERS_FOLDER "include/librssguard")
elseif((MINGW AND BUILD_MSYS2) OR (UNIX AND NOT APPLE AND NOT ANDROID))
  include (GNUInstallDirs)
  install(TARGETS rssguard
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  set(HEADERS_FOLDER "${CMAKE_INSTALL_INCLUDEDIR}/librssguard")
elseif(APPLE)
  install(TARGETS rssguard DESTINATION Contents/MacOS)
  set(HEADERS_FOLDER "Contents/Resources/Include")
endif()

# Add specific definitions.
target_compile_definitions(rssguard
  PRIVATE
  RSSGUARD_DLLSPEC=Q_DECL_EXPORT
  RSSGUARD_LIBDIR="${CMAKE_INSTALL_LIBDIR}"
)

# Install public headers.
set(HEADERS_ROOT "${PROJECT_SOURCE_DIR}/src/librssguard")
file(GLOB_RECURSE HEADER_FILES RELATIVE ${HEADERS_ROOT} "*.h")

foreach(HEADER_FILE ${HEADER_FILES})
  #message(${HEADER_FILE})
  get_filename_component(HEADER_PATH ${HEADER_FILE} DIRECTORY)
  #message(${HEADER_PATH})

  file(READ ${HEADER_FILE} HEADER_CONTENT)
  string(FIND "${HEADER_CONTENT}" "RSSGUARD_DLLSPEC" HEADER_EXPORT_IDX)

  if(${HEADER_EXPORT_IDX} GREATER -1)
    install(FILES ${HEADER_FILE} DESTINATION "${HEADERS_FOLDER}/${HEADER_PATH}")
  endif()
endforeach()
