TEMPLATE = lib

unix|mac {
  TARGET = rssguard
}
else {
  TARGET = librssguard
}

MSG_PREFIX  = "librssguard"
APP_TYPE    = "core library"

include(../../pri/vars.pri)
include(../../pri/defs.pri)

message($$MSG_PREFIX: Shadow copy build directory \"$$OUT_PWD\".)
message($$MSG_PREFIX: $$APP_NAME version is: \"$$APP_VERSION\".)
message($$MSG_PREFIX: Detected Qt version: \"$$QT_VERSION\".)
message($$MSG_PREFIX: Build destination directory: \"$$DESTDIR\".)
message($$MSG_PREFIX: Build revision: \"$$APP_REVISION\".)
message($$MSG_PREFIX: lrelease executable name: \"$$LRELEASE\".)

include(../../pri/build_opts.pri)

DEFINES *= RSSGUARD_DLLSPEC=Q_DECL_EXPORT
CONFIG += unversioned_libname unversioned_soname skip_target_version_ext

RESOURCES += ../../resources/sql.qrc \
             ../../resources/rssguard.qrc

mac|win32 {
  RESOURCES += ../../resources/icons.qrc
}

HEADERS += core/feeddownloader.h \
           core/feedsmodel.h \
           core/feedsproxymodel.h \
           core/message.h \
           core/messagefilter.h \
           core/messageobject.h \
           core/messagesforfiltersmodel.h \
           core/messagesmodel.h \
           core/messagesmodelcache.h \
           core/messagesmodelsqllayer.h \
           core/messagesproxymodel.h \
           definitions/definitions.h \
           dynamic-shortcuts/dynamicshortcuts.h \
           dynamic-shortcuts/dynamicshortcutswidget.h \
           dynamic-shortcuts/shortcutcatcher.h \
           exceptions/applicationexception.h \
           exceptions/filteringexception.h \
           exceptions/ioexception.h \
           gui/baselineedit.h \
           gui/basetoolbar.h \
           gui/colortoolbutton.h \
           gui/comboboxwithstatus.h \
           gui/dialogs/formabout.h \
           gui/dialogs/formaddaccount.h \
           gui/dialogs/formaddeditlabel.h \
           gui/dialogs/formbackupdatabasesettings.h \
           gui/dialogs/formdatabasecleanup.h \
           gui/dialogs/formmain.h \
           gui/dialogs/formmessagefiltersmanager.h \
           gui/dialogs/formrestoredatabasesettings.h \
           gui/dialogs/formsettings.h \
           gui/dialogs/formupdate.h \
           gui/edittableview.h \
           gui/feedmessageviewer.h \
           gui/feedstoolbar.h \
           gui/feedsview.h \
           gui/guiutilities.h \
           gui/labelsmenu.h \
           gui/labelwithstatus.h \
           gui/lineeditwithstatus.h \
           gui/messagebox.h \
           gui/messagessearchlineedit.h \
           gui/messagestoolbar.h \
           gui/messagesview.h \
           gui/nonclosablemenu.h \
           gui/plaintoolbutton.h \
           gui/settings/settingsbrowsermail.h \
           gui/settings/settingsdatabase.h \
           gui/settings/settingsdownloads.h \
           gui/settings/settingsfeedsmessages.h \
           gui/settings/settingsgeneral.h \
           gui/settings/settingsgui.h \
           gui/settings/settingslocalization.h \
           gui/settings/settingspanel.h \
           gui/settings/settingsshortcuts.h \
           gui/squeezelabel.h \
           gui/statusbar.h \
           gui/styleditemdelegatewithoutfocus.h \
           gui/systemtrayicon.h \
           gui/tabbar.h \
           gui/tabcontent.h \
           gui/tabwidget.h \
           gui/timespinbox.h \
           gui/toolbareditor.h \
           gui/treeviewcolumnsmenu.h \
           gui/widgetwithstatus.h \
           miscellaneous/application.h \
           miscellaneous/autosaver.h \
           miscellaneous/databasecleaner.h \
           miscellaneous/databasefactory.h \
           miscellaneous/databasequeries.h \
           miscellaneous/externaltool.h \
           miscellaneous/feedreader.h \
           miscellaneous/iconfactory.h \
           miscellaneous/iofactory.h \
           miscellaneous/localization.h \
           miscellaneous/mutex.h \
           miscellaneous/regexfactory.h \
           miscellaneous/settings.h \
           miscellaneous/settingsproperties.h \
           miscellaneous/simplecrypt/simplecrypt.h \
           miscellaneous/skinfactory.h \
           miscellaneous/systemfactory.h \
           miscellaneous/templates.h \
           miscellaneous/textfactory.h \
           network-web/basenetworkaccessmanager.h \
           network-web/downloader.h \
           network-web/downloadmanager.h \
           network-web/networkfactory.h \
           network-web/oauth2service.h \
           network-web/silentnetworkaccessmanager.h \
           network-web/webfactory.h \
           qtsingleapplication/qtlocalpeer.h \
           qtsingleapplication/qtlockedfile.h \
           qtsingleapplication/qtsingleapplication.h \
           qtsingleapplication/qtsinglecoreapplication.h \
           services/abstract/accountcheckmodel.h \
           services/abstract/cacheforserviceroot.h \
           services/abstract/category.h \
           services/abstract/feed.h \
           services/abstract/gui/formfeeddetails.h \
           services/abstract/importantnode.h \
           services/abstract/label.h \
           services/abstract/labelsnode.h \
           services/abstract/recyclebin.h \
           services/abstract/rootitem.h \
           services/abstract/serviceentrypoint.h \
           services/abstract/serviceroot.h \
           services/gmail/definitions.h \
           services/gmail/gmailentrypoint.h \
           services/gmail/gmailfeed.h \
           services/gmail/gmailserviceroot.h \
           services/gmail/gui/emailrecipientcontrol.h \
           services/gmail/gui/formeditgmailaccount.h \
           services/gmail/network/gmailnetworkfactory.h \
           services/inoreader/definitions.h \
           services/inoreader/gui/formeditinoreaderaccount.h \
           services/inoreader/inoreaderentrypoint.h \
           services/inoreader/inoreaderfeed.h \
           services/inoreader/inoreaderserviceroot.h \
           services/inoreader/network/inoreadernetworkfactory.h \
           services/owncloud/definitions.h \
           services/owncloud/gui/formeditowncloudaccount.h \
           services/owncloud/network/owncloudnetworkfactory.h \
           services/owncloud/owncloudfeed.h \
           services/owncloud/owncloudserviceentrypoint.h \
           services/owncloud/owncloudserviceroot.h \
           services/standard/atomparser.h \
           services/standard/feedparser.h \
           services/abstract/gui/authenticationdetails.h \
           services/standard/gui/formstandardcategorydetails.h \
           services/standard/gui/formstandardfeeddetails.h \
           services/standard/gui/formstandardimportexport.h \
           services/standard/gui/standardfeeddetails.h \
           services/standard/jsonparser.h \
           services/standard/rdfparser.h \
           services/standard/rssparser.h \
           services/standard/standardcategory.h \
           services/standard/standardfeed.h \
           services/standard/standardfeedsimportexportmodel.h \
           services/standard/standardserviceentrypoint.h \
           services/standard/standardserviceroot.h \
           services/tt-rss/definitions.h \
           services/tt-rss/gui/formeditttrssaccount.h \
           services/tt-rss/gui/formttrssfeeddetails.h \
           services/tt-rss/gui/ttrssfeeddetails.h \
           services/tt-rss/network/ttrssnetworkfactory.h \
           services/tt-rss/ttrssfeed.h \
           services/tt-rss/ttrssserviceentrypoint.h \
           services/tt-rss/ttrssserviceroot.h \
           network-web/httpresponse.h \
           services/gmail/gui/formdownloadattachment.h \
           services/gmail/gui/formaddeditemail.h \
           gui/searchtextwidget.h \
           network-web/oauthhttphandler.h \
           gui/messagepreviewer.h \
           gui/newspaperpreviewer.h

SOURCES += core/feeddownloader.cpp \
           core/feedsmodel.cpp \
           core/feedsproxymodel.cpp \
           core/message.cpp \
           core/messagefilter.cpp \
           core/messageobject.cpp \
           core/messagesforfiltersmodel.cpp \
           core/messagesmodel.cpp \
           core/messagesmodelcache.cpp \
           core/messagesmodelsqllayer.cpp \
           core/messagesproxymodel.cpp \
           dynamic-shortcuts/dynamicshortcuts.cpp \
           dynamic-shortcuts/dynamicshortcutswidget.cpp \
           dynamic-shortcuts/shortcutcatcher.cpp \
           exceptions/applicationexception.cpp \
           exceptions/filteringexception.cpp \
           exceptions/ioexception.cpp \
           gui/baselineedit.cpp \
           gui/basetoolbar.cpp \
           gui/colortoolbutton.cpp \
           gui/comboboxwithstatus.cpp \
           gui/dialogs/formabout.cpp \
           gui/dialogs/formaddaccount.cpp \
           gui/dialogs/formaddeditlabel.cpp \
           gui/dialogs/formbackupdatabasesettings.cpp \
           gui/dialogs/formdatabasecleanup.cpp \
           gui/dialogs/formmain.cpp \
           gui/dialogs/formmessagefiltersmanager.cpp \
           gui/dialogs/formrestoredatabasesettings.cpp \
           gui/dialogs/formsettings.cpp \
           gui/dialogs/formupdate.cpp \
           gui/edittableview.cpp \
           gui/feedmessageviewer.cpp \
           gui/feedstoolbar.cpp \
           gui/feedsview.cpp \
           gui/guiutilities.cpp \
           gui/labelsmenu.cpp \
           gui/labelwithstatus.cpp \
           gui/lineeditwithstatus.cpp \
           gui/messagebox.cpp \
           gui/messagessearchlineedit.cpp \
           gui/messagestoolbar.cpp \
           gui/messagesview.cpp \
           gui/nonclosablemenu.cpp \
           gui/plaintoolbutton.cpp \
           gui/settings/settingsbrowsermail.cpp \
           gui/settings/settingsdatabase.cpp \
           gui/settings/settingsdownloads.cpp \
           gui/settings/settingsfeedsmessages.cpp \
           gui/settings/settingsgeneral.cpp \
           gui/settings/settingsgui.cpp \
           gui/settings/settingslocalization.cpp \
           gui/settings/settingspanel.cpp \
           gui/settings/settingsshortcuts.cpp \
           gui/squeezelabel.cpp \
           gui/statusbar.cpp \
           gui/styleditemdelegatewithoutfocus.cpp \
           gui/systemtrayicon.cpp \
           gui/tabbar.cpp \
           gui/tabcontent.cpp \
           gui/tabwidget.cpp \
           gui/timespinbox.cpp \
           gui/toolbareditor.cpp \
           gui/treeviewcolumnsmenu.cpp \
           gui/widgetwithstatus.cpp \
           miscellaneous/application.cpp \
           miscellaneous/autosaver.cpp \
           miscellaneous/databasecleaner.cpp \
           miscellaneous/databasefactory.cpp \
           miscellaneous/databasequeries.cpp \
           miscellaneous/externaltool.cpp \
           miscellaneous/feedreader.cpp \
           miscellaneous/iconfactory.cpp \
           miscellaneous/iofactory.cpp \
           miscellaneous/localization.cpp \
           miscellaneous/mutex.cpp \
           miscellaneous/regexfactory.cpp \
           miscellaneous/settings.cpp \
           miscellaneous/simplecrypt/simplecrypt.cpp \
           miscellaneous/skinfactory.cpp \
           miscellaneous/systemfactory.cpp \
           miscellaneous/textfactory.cpp \
           network-web/basenetworkaccessmanager.cpp \
           network-web/downloader.cpp \
           network-web/downloadmanager.cpp \
           network-web/networkfactory.cpp \
           network-web/oauth2service.cpp \
           network-web/silentnetworkaccessmanager.cpp \
           network-web/webfactory.cpp \
           qtsingleapplication/qtlocalpeer.cpp \
           qtsingleapplication/qtlockedfile.cpp \
           qtsingleapplication/qtsingleapplication.cpp \
           qtsingleapplication/qtsinglecoreapplication.cpp \
           services/abstract/accountcheckmodel.cpp \
           services/abstract/cacheforserviceroot.cpp \
           services/abstract/category.cpp \
           services/abstract/feed.cpp \
           services/abstract/gui/formfeeddetails.cpp \
           services/abstract/importantnode.cpp \
           services/abstract/label.cpp \
           services/abstract/labelsnode.cpp \
           services/abstract/recyclebin.cpp \
           services/abstract/rootitem.cpp \
           services/abstract/serviceroot.cpp \
           services/gmail/gmailentrypoint.cpp \
           services/gmail/gmailfeed.cpp \
           services/gmail/gmailserviceroot.cpp \
           services/gmail/gui/emailrecipientcontrol.cpp \
           services/gmail/gui/formeditgmailaccount.cpp \
           services/gmail/network/gmailnetworkfactory.cpp \
           services/inoreader/gui/formeditinoreaderaccount.cpp \
           services/inoreader/inoreaderentrypoint.cpp \
           services/inoreader/inoreaderfeed.cpp \
           services/inoreader/inoreaderserviceroot.cpp \
           services/inoreader/network/inoreadernetworkfactory.cpp \
           services/owncloud/gui/formeditowncloudaccount.cpp \
           services/owncloud/network/owncloudnetworkfactory.cpp \
           services/owncloud/owncloudfeed.cpp \
           services/owncloud/owncloudserviceentrypoint.cpp \
           services/owncloud/owncloudserviceroot.cpp \
           services/standard/atomparser.cpp \
           services/standard/feedparser.cpp \
           services/abstract/gui/authenticationdetails.cpp \
           services/standard/gui/formstandardcategorydetails.cpp \
           services/standard/gui/formstandardfeeddetails.cpp \
           services/standard/gui/formstandardimportexport.cpp \
           services/standard/gui/standardfeeddetails.cpp \
           services/standard/jsonparser.cpp \
           services/standard/rdfparser.cpp \
           services/standard/rssparser.cpp \
           services/standard/standardcategory.cpp \
           services/standard/standardfeed.cpp \
           services/standard/standardfeedsimportexportmodel.cpp \
           services/standard/standardserviceentrypoint.cpp \
           services/standard/standardserviceroot.cpp \
           services/tt-rss/gui/formeditttrssaccount.cpp \
           services/tt-rss/gui/formttrssfeeddetails.cpp \
           services/tt-rss/gui/ttrssfeeddetails.cpp \
           services/tt-rss/network/ttrssnetworkfactory.cpp \
           services/tt-rss/ttrssfeed.cpp \
           services/tt-rss/ttrssserviceentrypoint.cpp \
           services/tt-rss/ttrssserviceroot.cpp \
           network-web/httpresponse.cpp \
           services/gmail/gui/formdownloadattachment.cpp \
           services/gmail/gui/formaddeditemail.cpp \
           gui/searchtextwidget.cpp \
           network-web/oauthhttphandler.cpp \
           gui/messagepreviewer.cpp \
           gui/newspaperpreviewer.cpp

mac {
  OBJECTIVE_SOURCES += miscellaneous/disablewindowtabbing.mm
}

FORMS += gui/dialogs/formabout.ui \
         gui/dialogs/formaddaccount.ui \
         gui/dialogs/formaddeditlabel.ui \
         gui/dialogs/formbackupdatabasesettings.ui \
         gui/dialogs/formdatabasecleanup.ui \
         gui/dialogs/formmain.ui \
         gui/dialogs/formmessagefiltersmanager.ui \
         gui/dialogs/formrestoredatabasesettings.ui \
         gui/dialogs/formsettings.ui \
         gui/dialogs/formupdate.ui \
         gui/settings/settingsbrowsermail.ui \
         gui/settings/settingsdatabase.ui \
         gui/settings/settingsdownloads.ui \
         gui/settings/settingsfeedsmessages.ui \
         gui/settings/settingsgeneral.ui \
         gui/settings/settingsgui.ui \
         gui/settings/settingslocalization.ui \
         gui/settings/settingsshortcuts.ui \
         gui/toolbareditor.ui \
         network-web/downloaditem.ui \
         network-web/downloadmanager.ui \
         services/abstract/gui/formfeeddetails.ui \
         services/gmail/gui/formeditgmailaccount.ui \
         services/inoreader/gui/formeditinoreaderaccount.ui \
         services/owncloud/gui/formeditowncloudaccount.ui \
         services/abstract/gui/authenticationdetails.ui \
         services/standard/gui/formstandardcategorydetails.ui \
         services/standard/gui/formstandardimportexport.ui \
         services/standard/gui/standardfeeddetails.ui \
         services/tt-rss/gui/formeditttrssaccount.ui \
         services/gmail/gui/formdownloadattachment.ui \
         services/gmail/gui/formaddeditemail.ui \
         gui/searchtextwidget.ui \
         gui/newspaperpreviewer.ui \
         services/tt-rss/gui/ttrssfeeddetails.ui

equals(USE_WEBENGINE, true) {
  HEADERS += gui/locationlineedit.h \
             gui/webviewer.h \
             gui/webbrowser.h \
             gui/discoverfeedsbutton.h \
             network-web/googlesuggest.h \
             network-web/webpage.h \
             network-web/rssguardschemehandler.h

  SOURCES += gui/locationlineedit.cpp \
             gui/webviewer.cpp \
             gui/webbrowser.cpp \
             gui/discoverfeedsbutton.cpp \
             network-web/googlesuggest.cpp \
             network-web/webpage.cpp \
             network-web/rssguardschemehandler.cpp

  # Add AdBlock sources.
  HEADERS += network-web/adblock/adblockaddsubscriptiondialog.h \
             network-web/adblock/adblockdialog.h \
             network-web/adblock/adblockicon.h \
             network-web/adblock/adblockmanager.h \
             network-web/adblock/adblockmatcher.h \
             network-web/adblock/adblockrule.h \
             network-web/adblock/adblocksearchtree.h \
             network-web/adblock/adblocksubscription.h \
             network-web/adblock/adblocktreewidget.h \
             network-web/adblock/adblockurlinterceptor.h \
             network-web/urlinterceptor.h \
             network-web/networkurlinterceptor.h \
             gui/treewidget.h

  SOURCES += network-web/adblock/adblockaddsubscriptiondialog.cpp \
             network-web/adblock/adblockdialog.cpp \
             network-web/adblock/adblockicon.cpp \
             network-web/adblock/adblockmanager.cpp \
             network-web/adblock/adblockmatcher.cpp \
             network-web/adblock/adblockrule.cpp \
             network-web/adblock/adblocksearchtree.cpp \
             network-web/adblock/adblocksubscription.cpp \
             network-web/adblock/adblocktreewidget.cpp \
             network-web/adblock/adblockurlinterceptor.cpp \
             network-web/networkurlinterceptor.cpp \
             gui/treewidget.cpp

  FORMS += network-web/adblock/adblockaddsubscriptiondialog.ui \
           network-web/adblock/adblockdialog.ui
}
else {
  HEADERS += gui/messagetextbrowser.h \
             gui/messagebrowser.h
  SOURCES += gui/messagetextbrowser.cpp \
             gui/messagebrowser.cpp
}

# Add mimesis.
SOURCES += $$files(3rd-party/mimesis/*.cpp, false)
HEADERS  += $$files(3rd-party/mimesis/*.hpp, false)

# Add boolinq.
HEADERS  += $$files(3rd-party/boolinq/*.h, false)

INCLUDEPATH +=  $$PWD/. \
                $$PWD/gui \
                $$PWD/gui/dialogs \
                $$PWD/dynamic-shortcuts

TRANSLATIONS += $$files($$PWD/../../localization/rssguard_*.ts, false) \
                $$files($$PWD/../../localization/qtbase_*.ts, false)

load(uic)
uic.commands -= -no-stringliteral

TR_EXCLUDE += $(QTDIR)

# Create new "make lupdate" target.
lupdate.target = lupdate
lupdate.commands = lupdate -no-obsolete -pro $$shell_quote($$shell_path($$PWD/librssguard.pro)) -ts $$shell_quote($$shell_path($$PWD/../../localization/rssguard_en.ts))

QMAKE_EXTRA_TARGETS += lupdate

# Make sure QM translations are nerated.
qtPrepareTool(LRELEASE, lrelease) {
  message($$MSG_PREFIX: Running: \"$$LRELEASE\" -compress librssguard.pro)
  system($$LRELEASE -compress librssguard.pro)
}

mac {
  IDENTIFIER = $$APP_REVERSE_NAME
  CONFIG -= app_bundle
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
  LIBS += -framework AppKit
}
