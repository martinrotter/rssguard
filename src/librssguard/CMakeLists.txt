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
  gui/richtexteditor/mrichtextedit.cpp
  gui/richtexteditor/mrichtextedit.h
  gui/richtexteditor/mtextedit.cpp
  gui/richtexteditor/mtextedit.h
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
  gui/settings/settingsmediaplayer.cpp
  gui/settings/settingsmediaplayer.h
  gui/settings/settingsgui.cpp
  gui/settings/settingsgui.h
  gui/settings/settingslocalization.cpp
  gui/settings/settingslocalization.h
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
  network-web/adblock/adblockdialog.cpp
  network-web/adblock/adblockdialog.h
  network-web/adblock/adblockicon.cpp
  network-web/adblock/adblockicon.h
  network-web/adblock/adblockmanager.cpp
  network-web/adblock/adblockmanager.h
  network-web/adblock/adblockrequestinfo.cpp
  network-web/adblock/adblockrequestinfo.h
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
  network-web/apiserver.cpp
  network-web/apiserver.h
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
  services/feedly/definitions.h
  services/feedly/feedlyentrypoint.cpp
  services/feedly/feedlyentrypoint.h
  services/feedly/feedlynetwork.cpp
  services/feedly/feedlynetwork.h
  services/feedly/feedlyserviceroot.cpp
  services/feedly/feedlyserviceroot.h
  services/feedly/gui/feedlyaccountdetails.cpp
  services/feedly/gui/feedlyaccountdetails.h
  services/feedly/gui/formeditfeedlyaccount.cpp
  services/feedly/gui/formeditfeedlyaccount.h
  services/gmail/definitions.h
  services/gmail/gmailentrypoint.cpp
  services/gmail/gmailentrypoint.h
  services/gmail/gmailnetworkfactory.cpp
  services/gmail/gmailnetworkfactory.h
  services/gmail/gmailserviceroot.cpp
  services/gmail/gmailserviceroot.h
  services/gmail/gui/emailpreviewer.cpp
  services/gmail/gui/emailpreviewer.h
  services/gmail/gui/emailrecipientcontrol.cpp
  services/gmail/gui/emailrecipientcontrol.h
  services/gmail/gui/formaddeditemail.cpp
  services/gmail/gui/formaddeditemail.h
  services/gmail/gui/formeditgmailaccount.cpp
  services/gmail/gui/formeditgmailaccount.h
  services/gmail/gui/gmailaccountdetails.cpp
  services/gmail/gui/gmailaccountdetails.h
  services/greader/definitions.h
  services/greader/greaderentrypoint.cpp
  services/greader/greaderentrypoint.h
  services/greader/greadernetwork.cpp
  services/greader/greadernetwork.h
  services/greader/greaderserviceroot.cpp
  services/greader/greaderserviceroot.h
  services/greader/greaderfeed.cpp
  services/greader/greaderfeed.h
  services/greader/gui/formeditgreaderaccount.cpp
  services/greader/gui/formeditgreaderaccount.h
  services/greader/gui/greaderaccountdetails.cpp
  services/greader/gui/greaderaccountdetails.h
  services/greader/gui/formgreaderfeeddetails.cpp
  services/greader/gui/formgreaderfeeddetails.h
  services/greader/gui/greaderfeeddetails.cpp
  services/greader/gui/greaderfeeddetails.h
  services/owncloud/definitions.h
  services/owncloud/gui/formeditowncloudaccount.cpp
  services/owncloud/gui/formeditowncloudaccount.h
  services/owncloud/gui/owncloudaccountdetails.cpp
  services/owncloud/gui/owncloudaccountdetails.h
  services/owncloud/owncloudfeed.cpp
  services/owncloud/owncloudfeed.h
  services/owncloud/owncloudnetworkfactory.cpp
  services/owncloud/owncloudnetworkfactory.h
  services/owncloud/owncloudserviceentrypoint.cpp
  services/owncloud/owncloudserviceentrypoint.h
  services/owncloud/owncloudserviceroot.cpp
  services/owncloud/owncloudserviceroot.h
  services/reddit/definitions.h
  services/reddit/gui/formeditredditaccount.cpp
  services/reddit/gui/formeditredditaccount.h
  services/reddit/gui/redditaccountdetails.cpp
  services/reddit/gui/redditaccountdetails.h
  services/reddit/redditcategory.cpp
  services/reddit/redditcategory.h
  services/reddit/redditentrypoint.cpp
  services/reddit/redditentrypoint.h
  services/reddit/redditnetworkfactory.cpp
  services/reddit/redditnetworkfactory.h
  services/reddit/redditserviceroot.cpp
  services/reddit/redditserviceroot.h
  services/reddit/redditsubscription.cpp
  services/reddit/redditsubscription.h
  services/standard/definitions.h
  services/standard/gui/formdiscoverfeeds.cpp
  services/standard/gui/formdiscoverfeeds.h
  services/standard/gui/formeditstandardaccount.cpp
  services/standard/gui/formeditstandardaccount.h
  services/standard/gui/formstandardfeeddetails.cpp
  services/standard/gui/formstandardfeeddetails.h
  services/standard/gui/formstandardimportexport.cpp
  services/standard/gui/formstandardimportexport.h
  services/standard/gui/standardfeeddetails.cpp
  services/standard/gui/standardfeeddetails.h
  services/standard/gui/standardaccountdetails.cpp
  services/standard/gui/standardaccountdetails.h
  services/standard/parsers/atomparser.cpp
  services/standard/parsers/atomparser.h
  services/standard/parsers/feedparser.cpp
  services/standard/parsers/feedparser.h
  services/standard/parsers/jsonparser.cpp
  services/standard/parsers/jsonparser.h
  services/standard/parsers/rdfparser.cpp
  services/standard/parsers/rdfparser.h
  services/standard/parsers/rssparser.cpp
  services/standard/parsers/rssparser.h
  services/standard/parsers/sitemapparser.cpp
  services/standard/parsers/sitemapparser.h
  services/standard/standardcategory.cpp
  services/standard/standardcategory.h
  services/standard/standardfeed.cpp
  services/standard/standardfeed.h
  services/standard/standardfeedsimportexportmodel.cpp
  services/standard/standardfeedsimportexportmodel.h
  services/standard/standardserviceentrypoint.cpp
  services/standard/standardserviceentrypoint.h
  services/standard/standardserviceroot.cpp
  services/standard/standardserviceroot.h
  services/tt-rss/definitions.h
  services/tt-rss/gui/formeditttrssaccount.cpp
  services/tt-rss/gui/formeditttrssaccount.h
  services/tt-rss/gui/formttrssfeeddetails.cpp
  services/tt-rss/gui/formttrssfeeddetails.h
  services/tt-rss/gui/formttrssnote.cpp
  services/tt-rss/gui/formttrssnote.h
  services/tt-rss/gui/ttrssaccountdetails.cpp
  services/tt-rss/gui/ttrssaccountdetails.h
  services/tt-rss/gui/ttrssfeeddetails.cpp
  services/tt-rss/gui/ttrssfeeddetails.h
  services/tt-rss/ttrssfeed.cpp
  services/tt-rss/ttrssfeed.h
  services/tt-rss/ttrssnetworkfactory.cpp
  services/tt-rss/ttrssnetworkfactory.h
  services/tt-rss/ttrssnotetopublish.h
  services/tt-rss/ttrssserviceentrypoint.cpp
  services/tt-rss/ttrssserviceentrypoint.h
  services/tt-rss/ttrssserviceroot.cpp
  services/tt-rss/ttrssserviceroot.h
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
  gui/reusable/networkproxydetails.ui
  gui/reusable/searchtextwidget.ui
  gui/richtexteditor/mrichtextedit.ui
  gui/settings/settingsbrowsermail.ui
  gui/settings/settingsdatabase.ui
  gui/settings/settingsdownloads.ui
  gui/settings/settingsfeedsmessages.ui
  gui/settings/settingsgeneral.ui
  gui/settings/settingsgui.ui
  gui/settings/settingslocalization.ui
  gui/settings/settingsnodejs.ui
  gui/settings/settingsnotifications.ui
  gui/settings/settingsshortcuts.ui
  gui/settings/settingsmediaplayer.ui
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
  services/feedly/gui/feedlyaccountdetails.ui
  services/gmail/gui/emailpreviewer.ui
  services/gmail/gui/formaddeditemail.ui
  services/gmail/gui/gmailaccountdetails.ui
  services/greader/gui/greaderaccountdetails.ui
  services/greader/gui/greaderfeeddetails.ui
  services/owncloud/gui/owncloudaccountdetails.ui
  services/reddit/gui/redditaccountdetails.ui
  services/standard/gui/formdiscoverfeeds.ui
  services/standard/gui/formstandardimportexport.ui
  services/standard/gui/standardfeeddetails.ui
  services/standard/gui/standardaccountdetails.ui
  services/tt-rss/gui/formttrssnote.ui
  services/tt-rss/gui/ttrssaccountdetails.ui
  services/tt-rss/gui/ttrssfeeddetails.ui
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

  # Search libmvp.
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

# Add mimesis.
list(APPEND SOURCES
  3rd-party/mimesis/mimesis.cpp
  3rd-party/mimesis/mimesis.hpp
  3rd-party/mimesis/quoted-printable.cpp
  3rd-party/mimesis/quoted-printable.hpp
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

# Add ZLIB.
if(ENABLE_COMPRESSED_SITEMAP)
  find_package(ZLIB REQUIRED)

  # Add qcompressor.
  list(APPEND SOURCES
    3rd-party/qcompressor/qcompressor.cpp
    3rd-party/qcompressor/qcompressor.h
  )
endif(ENABLE_COMPRESSED_SITEMAP)

if(ZLIB_FOUND)
  message(STATUS "Using system zlib ${ZLIB_VERSION_STRING}.")
endif(ZLIB_FOUND)

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

# Process OAuth service secrets.
set(FEEDLY_CLIENT_ID "" CACHE STRING "Feedly client ID")
set(FEEDLY_CLIENT_SECRET "" CACHE STRING "Feedly client secret")

set(GMAIL_CLIENT_ID "" CACHE STRING "GMail client ID")
set(GMAIL_CLIENT_SECRET "" CACHE STRING "GMail client secret")

set(INOREADER_CLIENT_ID "" CACHE STRING "Inoreader client ID")
set(INOREADER_CLIENT_SECRET "" CACHE STRING "Inoreader client secret")

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

# Add OAuth services definitions.
if(NOT FEEDLY_CLIENT_ID STREQUAL "" AND NOT FEEDLY_CLIENT_SECRET STREQUAL "")
  target_compile_definitions(rssguard
    PRIVATE

    FEEDLY_OFFICIAL_SUPPORT
    FEEDLY_CLIENT_ID="${FEEDLY_CLIENT_ID}"
    FEEDLY_CLIENT_SECRET="${FEEDLY_CLIENT_SECRET}"
  )

  message(STATUS "Enabling official Feedly support.")
else()
  message(STATUS "Feedly client ID/secret variables are not set. Disabling official support.")
endif()

if(NOT GMAIL_CLIENT_ID STREQUAL "" AND NOT GMAIL_CLIENT_SECRET STREQUAL "")
  target_compile_definitions(rssguard
    PRIVATE

    GMAIL_OFFICIAL_SUPPORT
    GMAIL_CLIENT_ID="${GMAIL_CLIENT_ID}"
    GMAIL_CLIENT_SECRET="${GMAIL_CLIENT_SECRET}"
  )

  message(STATUS "Enabling official Gmail support.")
else()
  message(STATUS "Gmail client ID/secret variables are not set. Disabling official support.")
endif()

if(NOT INOREADER_CLIENT_ID STREQUAL "" AND NOT INOREADER_CLIENT_SECRET STREQUAL "")
  target_compile_definitions(rssguard
    PRIVATE

    INOREADER_OFFICIAL_SUPPORT
    INOREADER_CLIENT_ID="${INOREADER_CLIENT_ID}"
    INOREADER_CLIENT_SECRET="${INOREADER_CLIENT_SECRET}"
  )

  message(STATUS "Enabling official Inoreader support.")
else()
  message(STATUS "Inoreader client ID/secret variables are not set. Disabling official support.")
endif()

# Add specific definitions.
target_compile_definitions(rssguard
  PRIVATE
  RSSGUARD_DLLSPEC=Q_DECL_EXPORT

  APP_AUTHOR="${APP_AUTHOR}"
  APP_DONATE_URL="${APP_DONATE_URL}"
  APP_EMAIL="${APP_EMAIL}"
  APP_LOW_H_NAME=".${CMAKE_PROJECT_NAME}"
  APP_REVISION="${APP_REVISION}"
  APP_SYSTEM_NAME="${CMAKE_SYSTEM_NAME}"
  APP_SYSTEM_VERSION="${CMAKE_SYSTEM_PROCESSOR}"

  APP_URL_DOCUMENTATION="${APP_URL_DOCUMENTATION}"
  APP_URL_ISSUES_NEW="${APP_URL_ISSUES_NEW}"
  APP_USERAGENT="${APP_NAME}/${CMAKE_PROJECT_VERSION}")

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

if(ZLIB_FOUND)
  target_include_directories(rssguard AFTER
    PRIVATE
    ${ZLIB_INCLUDE_DIRS}
  )

  target_compile_definitions(rssguard
    PRIVATE

    ENABLE_COMPRESSED_SITEMAP
  )

  target_link_libraries(rssguard PRIVATE
    ${ZLIB_LIBRARIES}
  )
endif(ZLIB_FOUND)

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
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
  )
elseif(ENABLE_MEDIAPLAYER_LIBMPV)
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
    Shell32.lib
    odbc32
  )
endif()

if(WIN32 OR OS2)
  install(TARGETS rssguard DESTINATION .)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION include
    FILES_MATCHING PATTERN "services/abstract/*.h"
  )
  install(DIRECTORY services/abstract
    DESTINATION include/librssguard/services
    FILES_MATCHING PATTERN "*.h"
  )
elseif(UNIX AND NOT APPLE AND NOT ANDROID)
  include (GNUInstallDirs)
  install(TARGETS rssguard
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(DIRECTORY services/abstract
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/librssguard/services
    FILES_MATCHING PATTERN "*.h"
  )
elseif(APPLE)
  install(TARGETS rssguard
    DESTINATION Contents/MacOS
  )
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION Contents/Resources/Include
    FILES_MATCHING PATTERN "services/abstract/*.h"
  )
  install(DIRECTORY services/abstract
    DESTINATION Contents/Resources/Include/librssguard/services
    FILES_MATCHING PATTERN "*.h"
  )
endif()
