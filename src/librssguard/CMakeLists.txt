set(SOURCES
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
    exceptions/filteringexception.cpp
    exceptions/filteringexception.h
    exceptions/ioexception.cpp
    exceptions/ioexception.h
    exceptions/networkexception.cpp
    exceptions/networkexception.h
    exceptions/scriptexception.cpp
    exceptions/scriptexception.h
    gui/dialogs/formabout.cpp
    gui/dialogs/formabout.h
    gui/dialogs/formaddaccount.cpp
    gui/dialogs/formaddaccount.h
    gui/dialogs/formaddeditlabel.cpp
    gui/dialogs/formaddeditlabel.h
    gui/dialogs/formbackupdatabasesettings.cpp
    gui/dialogs/formbackupdatabasesettings.h
    gui/dialogs/formdatabasecleanup.cpp
    gui/dialogs/formdatabasecleanup.h
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
    gui/messagebox.cpp
    gui/messagebox.h
    gui/messagepreviewer.cpp
    gui/messagepreviewer.h
    gui/messagesview.cpp
    gui/messagesview.h
    gui/newspaperpreviewer.cpp
    gui/newspaperpreviewer.h
    gui/notifications/notificationseditor.cpp
    gui/notifications/notificationseditor.h
    gui/notifications/singlenotificationeditor.cpp
    gui/notifications/singlenotificationeditor.h
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
    gui/reusable/labelsmenu.cpp
    gui/reusable/labelsmenu.h
    gui/reusable/labelwithstatus.cpp
    gui/reusable/labelwithstatus.h
    gui/reusable/lineeditwithstatus.cpp
    gui/reusable/lineeditwithstatus.h
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
    network-web/basenetworkaccessmanager.cpp
    network-web/basenetworkaccessmanager.h
    network-web/cookiejar.cpp
    network-web/cookiejar.h
    network-web/downloader.cpp
    network-web/downloader.h
    network-web/downloadmanager.cpp
    network-web/downloadmanager.h
    network-web/httpresponse.cpp
    network-web/httpresponse.h
    network-web/networkfactory.cpp
    network-web/networkfactory.h
    network-web/oauth2service.cpp
    network-web/oauth2service.h
    network-web/oauthhttphandler.cpp
    network-web/oauthhttphandler.h
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
    services/abstract/gui/authenticationdetails.cpp
    services/abstract/gui/authenticationdetails.h
    services/abstract/gui/formaccountdetails.cpp
    services/abstract/gui/formaccountdetails.h
    services/abstract/gui/formcategorydetails.cpp
    services/abstract/gui/formcategorydetails.h
    services/abstract/gui/formfeeddetails.cpp
    services/abstract/gui/formfeeddetails.h
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
    services/gmail/gui/emailrecipientcontrol.cpp
    services/gmail/gui/emailrecipientcontrol.h
    services/gmail/gui/formaddeditemail.cpp
    services/gmail/gui/formaddeditemail.h
    services/gmail/gui/formdownloadattachment.cpp
    services/gmail/gui/formdownloadattachment.h
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
    services/greader/gui/formeditgreaderaccount.cpp
    services/greader/gui/formeditgreaderaccount.h
    services/greader/gui/greaderaccountdetails.cpp
    services/greader/gui/greaderaccountdetails.h
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
    services/standard/atomparser.cpp
    services/standard/atomparser.h
    services/standard/definitions.h
    services/standard/feedparser.cpp
    services/standard/feedparser.h
    services/standard/gui/formeditstandardaccount.cpp
    services/standard/gui/formeditstandardaccount.h
    services/standard/gui/formstandardfeeddetails.cpp
    services/standard/gui/formstandardfeeddetails.h
    services/standard/gui/formstandardimportexport.cpp
    services/standard/gui/formstandardimportexport.h
    services/standard/gui/standardfeeddetails.cpp
    services/standard/gui/standardfeeddetails.h
    services/standard/jsonparser.cpp
    services/standard/jsonparser.h
    services/standard/rdfparser.cpp
    services/standard/rdfparser.h
    services/standard/rssparser.cpp
    services/standard/rssparser.h
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

# Add mimesis
list(APPEND SOURCES
    3rd-party/mimesis/mimesis.cpp
    3rd-party/mimesis/mimesis.hpp
    3rd-party/mimesis/quoted-printable.cpp
    3rd-party/mimesis/quoted-printable.hpp
)

# Add boolinq
list(APPEND SOURCES
    3rd-party/boolinq/boolinq.h
)

# Add SimpleCrypt.
list(APPEND SOURCES
    3rd-party/sc/simplecrypt.cpp
    3rd-party/sc/simplecrypt.h
)

if(APPLE)
    list(APPEND SOURCES
        miscellaneous/disablewindowtabbing.mm
    )
endif()

if(USE_WEBENGINE)
    list(APPEND SOURCES
        gui/reusable/discoverfeedsbutton.cpp
        gui/reusable/discoverfeedsbutton.h
        gui/reusable/locationlineedit.cpp
        gui/reusable/locationlineedit.h
        gui/webbrowser.cpp
        gui/webbrowser.h
        gui/webviewer.cpp
        gui/webviewer.h
        network-web/googlesuggest.cpp
        network-web/googlesuggest.h
        network-web/webpage.cpp
        network-web/webpage.h

        # Add AdBlock sources
        network-web/adblock/adblockdialog.cpp
        network-web/adblock/adblockdialog.h
        network-web/adblock/adblockicon.cpp
        network-web/adblock/adblockicon.h
        network-web/adblock/adblockmanager.cpp
        network-web/adblock/adblockmanager.h
        network-web/adblock/adblockrequestinfo.cpp
        network-web/adblock/adblockrequestinfo.h
        network-web/adblock/adblockurlinterceptor.cpp
        network-web/adblock/adblockurlinterceptor.h
        network-web/networkurlinterceptor.cpp
        network-web/networkurlinterceptor.h
        network-web/urlinterceptor.h
    )
else()
    list(APPEND SOURCES
        gui/messagetextbrowser.cpp
        gui/messagetextbrowser.h
        gui/messagebrowser.cpp
        gui/messagebrowser.h
    )
endif()

qt_add_resources(SOURCES ${CMAKE_SOURCE_DIR}/resources/sql.qrc)
qt_add_resources(SOURCES ${CMAKE_SOURCE_DIR}/resources/rssguard.qrc)

if(APPLE OR WIN32 OR OS2)
    qt_add_resources(SOURCES ${CMAKE_SOURCE_DIR}/resources/icons.qrc)
endif()

add_library(rssguard SHARED ${SOURCES} ${QM_FILES})

target_compile_definitions(rssguard PRIVATE RSSGUARD_DLLSPEC=Q_DECL_EXPORT)

target_include_directories(rssguard
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/reusable
    ${CMAKE_CURRENT_SOURCE_DIR}/dynamic-shortcuts
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/notifications
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/toolbars
)

target_link_libraries(rssguard PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Qml
    Qt::Sql
    Qt::Widgets
    Qt::Xml
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(rssguard PUBLIC
        Qt::Core5Compat
    )
endif()

if(USE_WEBENGINE)
    target_link_libraries(rssguard PUBLIC
        Qt::WebEngineWidgets
    )
endif()

if(NOT OS2)
    target_link_libraries(rssguard PUBLIC
        Qt::Multimedia
    )
endif()

if(UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(rssguard PUBLIC
        Qt::DBus
    )
endif()

if(APPLE)
    target_compile_options(rssguard PUBLIC -mmacosx-version-min=10.8)
    target_link_options(rssguard PUBLIC -mmacosx-version-min=10.8)
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
