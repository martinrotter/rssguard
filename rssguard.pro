#################################################################
#
# For license of this file, see <project-root-folder>/LICENSE.md.
#
#
# This is RSS Guard compilation script for qmake.
#
# Usage:
#   a) DEBUG build for testing. (out of source build type)
#     cd ../build-dir
#     lrelease ../rssguard-dir/rssguard.pro
#     qmake ../rssguard-dir/rssguard.pro -r CONFIG+=debug PREFIX=./usr
#     make
#     make install
#
#   b) RELEASE build for production use. (out of source build type)
#     cd ../build-dir
#     lrelease ../rssguard-dir/rssguard.pro
#     qmake ../rssguard-dir/rssguard.pro -r CONFIG+=release PREFIX=./usr
#     make
#     make install
#
# Variables:
#   USE_WEBENGINE - if specified, then QtWebEngine module for internal web browser is used.
#                   Otherwise simple text component is used and some features will be disabled.
#                   Default value is "false". If QtWebEngine is installed during compilation, then
#                   value of this variable is tweaked automatically.
#   PREFIX - specifies base folder to which files are copied during "make install"
#            step, defaults to "$$OUT_PWD/usr" on Linux and to "$$OUT_PWD/app" on Windows.
#   LRELEASE_EXECUTABLE - specifies the name/path of "lrelease" executable, defaults to "lrelease".
#
#
# Other information:
#   - supports Windows, Linux, Mac OS X, Android,
#   - Qt 5.8.0 or higher is required,
#   - C++ 11 is required.
#
# Authors and contributors:
#   - Martin Rotter (project leader),
#   - Elbert Pol (huge OS/2-related contributions).
#
#################################################################

TEMPLATE    = app
TARGET      = rssguard
DEFINES	    *= QT_USE_QSTRINGBUILDER

message(rssguard: Welcome RSS Guard qmake script.)

lessThan(QT_MAJOR_VERSION, 5)|lessThan(QT_MINOR_VERSION, 8) {
  error(rssguard: At least Qt \"5.8.0\" is required!!!)
}

APP_NAME                      = "RSS Guard"
APP_LOW_NAME                  = "rssguard"
APP_REVERSE_NAME              = "com.github.rssguard"
APP_LOW_H_NAME                = ".rssguard"
APP_AUTHOR                    = "Martin Rotter"
APP_COPYRIGHT                 = "(C) 2011-2017 $$APP_AUTHOR"
APP_VERSION                   = "3.5.5"
APP_LONG_NAME                 = "$$APP_NAME $$APP_VERSION"
APP_EMAIL                     = "rotter.martinos@gmail.com"
APP_URL                       = "https://github.com/martinrotter/rssguard"
APP_URL_ISSUES                = "https://github.com/martinrotter/rssguard/issues"
APP_URL_ISSUES_NEW            = "https://github.com/martinrotter/rssguard/issues/new"
APP_URL_WIKI                  = "https://github.com/martinrotter/rssguard/wiki"
APP_USERAGENT                 = "RSS Guard/$$APP_VERSION (github.com/martinrotter/rssguard)"
APP_DONATE_URL                = "https://goo.gl/YFVJ0j"
APP_WIN_ARCH                  = "win64"

isEmpty(PREFIX) {
  message(rssguard: PREFIX variable is not set. This might indicate error.)

  win32 {
    PREFIX = $$OUT_PWD/app
  }

  android {
    PREFIX = $$OUT_PWD/app
  }

  mac {
    PREFIX = $$quote($$OUT_PWD/$${APP_NAME}.app)
  }

  unix:!mac:!android {
    PREFIX = $$OUT_PWD/AppDir/usr
  }
}

isEmpty(USE_WEBENGINE) {
  USE_WEBENGINE = false
  message("rssguard: USE_WEBENGINE variable is not set.")

  qtHaveModule(webenginewidgets) {
    USE_WEBENGINE = true
    message("rssguard: WebEngine component IS installed, enabling it.")
  }
  else {
    USE_WEBENGINE = false
    message("rssguard: WebEngine component is probably NOT installed, disabling it.")
  }
}

message(rssguard: Shadow copy build directory \"$$OUT_PWD\".)

isEmpty(LRELEASE_EXECUTABLE) {
  LRELEASE_EXECUTABLE = lrelease
  message(rssguard: LRELEASE_EXECUTABLE variable is not set.)
}

# Custom definitions.
DEFINES += APP_VERSION='"\\\"$$APP_VERSION\\\""'
DEFINES += APP_NAME='"\\\"$$APP_NAME\\\""'
DEFINES += APP_LOW_NAME='"\\\"$$APP_LOW_NAME\\\""'
DEFINES += APP_LOW_H_NAME='"\\\"$$APP_LOW_H_NAME\\\""'
DEFINES += APP_LONG_NAME='"\\\"$$APP_LONG_NAME\\\""'
DEFINES += APP_AUTHOR='"\\\"$$APP_AUTHOR\\\""'
DEFINES += APP_EMAIL='"\\\"$$APP_EMAIL\\\""'
DEFINES += APP_URL='"\\\"$$APP_URL\\\""'
DEFINES += APP_URL_ISSUES='"\\\"$$APP_URL_ISSUES\\\""'
DEFINES += APP_URL_ISSUES_NEW='"\\\"$$APP_URL_ISSUES_NEW\\\""'
DEFINES += APP_URL_WIKI='"\\\"$$APP_URL_WIKI\\\""'
DEFINES += APP_USERAGENT='"\\\"$$APP_USERAGENT\\\""'
DEFINES += APP_DONATE_URL='"\\\"$$APP_DONATE_URL\\\""'
DEFINES += APP_SYSTEM_NAME='"\\\"$$QMAKE_HOST.os\\\""'
DEFINES += APP_SYSTEM_VERSION='"\\\"$$QMAKE_HOST.arch\\\""'

CODECFORTR  = UTF-8
CODECFORSRC = UTF-8

exists(.git) {
  APP_REVISION = $$system(git rev-parse --short HEAD)
}

isEmpty(APP_REVISION) {
  APP_REVISION = ""
}

equals(USE_WEBENGINE, false) {
  # Add extra revision naming when building without webengine.
  APP_REVISION = $$sprintf('%1-%2', $$APP_REVISION, nowebengine)
}

DEFINES += APP_REVISION='"\\\"$$APP_REVISION\\\""'

message(rssguard: RSS Guard version is: \"$$APP_VERSION\".)
message(rssguard: Detected Qt version: \"$$QT_VERSION\".)
message(rssguard: Build destination directory: \"$$DESTDIR\".)
message(rssguard: Prefix directory: \"$$PREFIX\".)
message(rssguard: Build revision: \"$$APP_REVISION\".)
message(rssguard: lrelease executable name: \"$$LRELEASE_EXECUTABLE\".)

QT *= core gui widgets sql network xml

CONFIG *= c++11 warn_on
DEFINES *= QT_USE_QSTRINGBUILDER QT_USE_FAST_CONCATENATION QT_USE_FAST_OPERATOR_PLUS UNICODE _UNICODE
VERSION = $$APP_VERSION

win32 {
  # Makes sure we use correct subsystem on Windows.
  !contains(QMAKE_TARGET.arch, x86_64) {
    message(rssguard: Compilling x86 variant.)
    QMAKE_LFLAGS_WINDOWS = /SUBSYSTEM:WINDOWS,5.01
  } else {
    message(rssguard: Compilling x86_64 variant.)
    QMAKE_LFLAGS_WINDOWS = /SUBSYSTEM:WINDOWS,5.02
  }
}

DISTFILES +=    resources/scripts/uncrustify/uncrustify.cfg

MOC_DIR = $$OUT_PWD/moc
RCC_DIR = $$OUT_PWD/rcc
UI_DIR = $$OUT_PWD/ui

mac {
  QT *= macextras
}

equals(USE_WEBENGINE, true) {
  message(rssguard: Application will be compiled WITH QtWebEngine module.)
  QT *= webenginewidgets
  DEFINES *= USE_WEBENGINE
}
else {
  message(rssguard: Application will be compiled without QtWebEngine module. Some features will be disabled.)
}

# Make needed tweaks for RC file getting generated on Windows.
win32 {
  RC_ICONS = resources/graphics/rssguard.ico
  QMAKE_TARGET_COMPANY = $$APP_AUTHOR
  QMAKE_TARGET_DESCRIPTION = $$APP_NAME
  QMAKE_TARGET_COPYRIGHT = $$APP_COPYRIGHT
  QMAKE_TARGET_PRODUCT = $$APP_NAME
}

CONFIG *= resources_big
RESOURCES += resources/sql.qrc \
             resources/rssguard.qrc

HEADERS +=  src/core/feeddownloader.h \
            src/core/feedsmodel.h \
            src/core/feedsproxymodel.h \
            src/core/message.h \
            src/core/messagesmodel.h \
            src/core/messagesmodelcache.h \
            src/core/messagesmodelsqllayer.h \
            src/core/messagesproxymodel.h \
            src/definitions/definitions.h \
            src/dynamic-shortcuts/dynamicshortcuts.h \
            src/dynamic-shortcuts/dynamicshortcutswidget.h \
            src/dynamic-shortcuts/shortcutbutton.h \
            src/dynamic-shortcuts/shortcutcatcher.h \
            src/exceptions/applicationexception.h \
            src/exceptions/ioexception.h \
            src/gui/baselineedit.h \
            src/gui/basetoolbar.h \
            src/gui/colorlabel.h \
            src/gui/comboboxwithstatus.h \
            src/gui/dialogs/formabout.h \
            src/gui/dialogs/formaddaccount.h \
            src/gui/dialogs/formbackupdatabasesettings.h \
            src/gui/dialogs/formdatabasecleanup.h \
            src/gui/dialogs/formmain.h \
            src/gui/dialogs/formrestoredatabasesettings.h \
            src/gui/dialogs/formsettings.h \
            src/gui/dialogs/formupdate.h \
            src/gui/edittableview.h \
            src/gui/feedmessageviewer.h \
            src/gui/feedstoolbar.h \
            src/gui/feedsview.h \
            src/gui/guiutilities.h \
            src/gui/labelwithstatus.h \
            src/gui/lineeditwithstatus.h \
            src/gui/messagebox.h \
            src/gui/messagessearchlineedit.h \
            src/gui/messagestoolbar.h \
            src/gui/messagesview.h \
            src/gui/plaintoolbutton.h \
            src/gui/settings/settingsbrowsermail.h \
            src/gui/settings/settingsdatabase.h \
            src/gui/settings/settingsdownloads.h \
            src/gui/settings/settingsfeedsmessages.h \
            src/gui/settings/settingsgeneral.h \
            src/gui/settings/settingsgui.h \
            src/gui/settings/settingslocalization.h \
            src/gui/settings/settingspanel.h \
            src/gui/settings/settingsshortcuts.h \
            src/gui/squeezelabel.h \
            src/gui/statusbar.h \
            src/gui/styleditemdelegatewithoutfocus.h \
            src/gui/systemtrayicon.h \
            src/gui/tabbar.h \
            src/gui/tabcontent.h \
            src/gui/tabwidget.h \
            src/gui/timespinbox.h \
            src/gui/toolbareditor.h \
            src/gui/treeviewcolumnsmenu.h \
            src/gui/widgetwithstatus.h \
            src/miscellaneous/application.h \
            src/miscellaneous/autosaver.h \
            src/miscellaneous/databasecleaner.h \
            src/miscellaneous/databasefactory.h \
            src/miscellaneous/databasequeries.h \
            src/miscellaneous/debugging.h \
            src/miscellaneous/externaltool.h \
            src/miscellaneous/feedreader.h \
            src/miscellaneous/iconfactory.h \
            src/miscellaneous/iofactory.h \
            src/miscellaneous/localization.h \
            src/miscellaneous/mutex.h \
            src/miscellaneous/settings.h \
            src/miscellaneous/settingsproperties.h \
            src/miscellaneous/simplecrypt/simplecrypt.h \
            src/miscellaneous/skinfactory.h \
            src/miscellaneous/systemfactory.h \
            src/miscellaneous/textfactory.h \
            src/network-web/basenetworkaccessmanager.h \
            src/network-web/downloader.h \
            src/network-web/downloadmanager.h \
            src/network-web/networkfactory.h \
            src/network-web/oauth2service.h \
            src/network-web/silentnetworkaccessmanager.h \
            src/network-web/webfactory.h \
            src/qtsingleapplication/qtlocalpeer.h \
            src/qtsingleapplication/qtlockedfile.h \
            src/qtsingleapplication/qtsingleapplication.h \
            src/qtsingleapplication/qtsinglecoreapplication.h \
            src/services/abstract/accountcheckmodel.h \
            src/services/abstract/cacheforserviceroot.h \
            src/services/abstract/category.h \
            src/services/abstract/feed.h \
            src/services/abstract/gui/formfeeddetails.h \
            src/services/abstract/recyclebin.h \
            src/services/abstract/rootitem.h \
            src/services/abstract/serviceentrypoint.h \
            src/services/abstract/serviceroot.h \
            src/services/gmail/definitions.h \
            src/services/gmail/gmailentrypoint.h \
            src/services/gmail/gmailfeed.h \
            src/services/gmail/gmailserviceroot.h \
            src/services/gmail/gui/formeditgmailaccount.h \
            src/services/gmail/network/gmailnetworkfactory.h \
            src/services/inoreader/definitions.h \
            src/services/inoreader/gui/formeditinoreaderaccount.h \
            src/services/inoreader/inoreaderentrypoint.h \
            src/services/inoreader/inoreaderfeed.h \
            src/services/inoreader/inoreaderserviceroot.h \
            src/services/inoreader/network/inoreadernetworkfactory.h \
            src/services/owncloud/definitions.h \
            src/services/owncloud/gui/formeditowncloudaccount.h \
            src/services/owncloud/gui/formowncloudfeeddetails.h \
            src/services/owncloud/network/owncloudnetworkfactory.h \
            src/services/owncloud/owncloudfeed.h \
            src/services/owncloud/owncloudserviceentrypoint.h \
            src/services/owncloud/owncloudserviceroot.h \
            src/services/standard/atomparser.h \
            src/services/standard/feedparser.h \
            src/services/standard/gui/formstandardcategorydetails.h \
            src/services/standard/gui/formstandardfeeddetails.h \
            src/services/standard/gui/formstandardimportexport.h \
            src/services/standard/rdfparser.h \
            src/services/standard/rssparser.h \
            src/services/standard/standardcategory.h \
            src/services/standard/standardfeed.h \
            src/services/standard/standardfeedsimportexportmodel.h \
            src/services/standard/standardserviceentrypoint.h \
            src/services/standard/standardserviceroot.h \
            src/services/tt-rss/definitions.h \
            src/services/tt-rss/gui/formeditttrssaccount.h \
            src/services/tt-rss/gui/formttrssfeeddetails.h \
            src/services/tt-rss/network/ttrssnetworkfactory.h \
            src/services/tt-rss/ttrssfeed.h \
            src/services/tt-rss/ttrssserviceentrypoint.h \
            src/services/tt-rss/ttrssserviceroot.h \
            src/network-web/httpresponse.h \
            src/services/gmail/gui/formdownloadattachment.h \
            src/services/gmail/gui/formaddeditemail.h \
            src/gui/searchtextwidget.h

SOURCES +=  src/core/feeddownloader.cpp \
            src/core/feedsmodel.cpp \
            src/core/feedsproxymodel.cpp \
            src/core/message.cpp \
            src/core/messagesmodel.cpp \
            src/core/messagesmodelcache.cpp \
            src/core/messagesmodelsqllayer.cpp \
            src/core/messagesproxymodel.cpp \
            src/dynamic-shortcuts/dynamicshortcuts.cpp \
            src/dynamic-shortcuts/dynamicshortcutswidget.cpp \
            src/dynamic-shortcuts/shortcutbutton.cpp \
            src/dynamic-shortcuts/shortcutcatcher.cpp \
            src/exceptions/applicationexception.cpp \
            src/exceptions/ioexception.cpp \
            src/gui/baselineedit.cpp \
            src/gui/basetoolbar.cpp \
            src/gui/colorlabel.cpp \
            src/gui/comboboxwithstatus.cpp \
            src/gui/dialogs/formabout.cpp \
            src/gui/dialogs/formaddaccount.cpp \
            src/gui/dialogs/formbackupdatabasesettings.cpp \
            src/gui/dialogs/formdatabasecleanup.cpp \
            src/gui/dialogs/formmain.cpp \
            src/gui/dialogs/formrestoredatabasesettings.cpp \
            src/gui/dialogs/formsettings.cpp \
            src/gui/dialogs/formupdate.cpp \
            src/gui/edittableview.cpp \
            src/gui/feedmessageviewer.cpp \
            src/gui/feedstoolbar.cpp \
            src/gui/feedsview.cpp \
            src/gui/guiutilities.cpp \
            src/gui/labelwithstatus.cpp \
            src/gui/lineeditwithstatus.cpp \
            src/gui/messagebox.cpp \
            src/gui/messagessearchlineedit.cpp \
            src/gui/messagestoolbar.cpp \
            src/gui/messagesview.cpp \
            src/gui/plaintoolbutton.cpp \
            src/gui/settings/settingsbrowsermail.cpp \
            src/gui/settings/settingsdatabase.cpp \
            src/gui/settings/settingsdownloads.cpp \
            src/gui/settings/settingsfeedsmessages.cpp \
            src/gui/settings/settingsgeneral.cpp \
            src/gui/settings/settingsgui.cpp \
            src/gui/settings/settingslocalization.cpp \
            src/gui/settings/settingspanel.cpp \
            src/gui/settings/settingsshortcuts.cpp \
            src/gui/squeezelabel.cpp \
            src/gui/statusbar.cpp \
            src/gui/styleditemdelegatewithoutfocus.cpp \
            src/gui/systemtrayicon.cpp \
            src/gui/tabbar.cpp \
            src/gui/tabcontent.cpp \
            src/gui/tabwidget.cpp \
            src/gui/timespinbox.cpp \
            src/gui/toolbareditor.cpp \
            src/gui/treeviewcolumnsmenu.cpp \
            src/gui/widgetwithstatus.cpp \
            src/main.cpp \
            src/miscellaneous/application.cpp \
            src/miscellaneous/autosaver.cpp \
            src/miscellaneous/databasecleaner.cpp \
            src/miscellaneous/databasefactory.cpp \
            src/miscellaneous/databasequeries.cpp \
            src/miscellaneous/debugging.cpp \
            src/miscellaneous/externaltool.cpp \
            src/miscellaneous/feedreader.cpp \
            src/miscellaneous/iconfactory.cpp \
            src/miscellaneous/iofactory.cpp \
            src/miscellaneous/localization.cpp \
            src/miscellaneous/mutex.cpp \
            src/miscellaneous/settings.cpp \
            src/miscellaneous/simplecrypt/simplecrypt.cpp \
            src/miscellaneous/skinfactory.cpp \
            src/miscellaneous/systemfactory.cpp \
            src/miscellaneous/textfactory.cpp \
            src/network-web/basenetworkaccessmanager.cpp \
            src/network-web/downloader.cpp \
            src/network-web/downloadmanager.cpp \
            src/network-web/networkfactory.cpp \
            src/network-web/oauth2service.cpp \
            src/network-web/silentnetworkaccessmanager.cpp \
            src/network-web/webfactory.cpp \
            src/qtsingleapplication/qtlocalpeer.cpp \
            src/qtsingleapplication/qtlockedfile.cpp \
            src/qtsingleapplication/qtsingleapplication.cpp \
            src/qtsingleapplication/qtsinglecoreapplication.cpp \
            src/services/abstract/accountcheckmodel.cpp \
            src/services/abstract/cacheforserviceroot.cpp \
            src/services/abstract/category.cpp \
            src/services/abstract/feed.cpp \
            src/services/abstract/gui/formfeeddetails.cpp \
            src/services/abstract/recyclebin.cpp \
            src/services/abstract/rootitem.cpp \
            src/services/abstract/serviceentrypoint.cpp \
            src/services/abstract/serviceroot.cpp \
            src/services/gmail/gmailentrypoint.cpp \
            src/services/gmail/gmailfeed.cpp \
            src/services/gmail/gmailserviceroot.cpp \
            src/services/gmail/gui/formeditgmailaccount.cpp \
            src/services/gmail/network/gmailnetworkfactory.cpp \
            src/services/inoreader/gui/formeditinoreaderaccount.cpp \
            src/services/inoreader/inoreaderentrypoint.cpp \
            src/services/inoreader/inoreaderfeed.cpp \
            src/services/inoreader/inoreaderserviceroot.cpp \
            src/services/inoreader/network/inoreadernetworkfactory.cpp \
            src/services/owncloud/gui/formeditowncloudaccount.cpp \
            src/services/owncloud/gui/formowncloudfeeddetails.cpp \
            src/services/owncloud/network/owncloudnetworkfactory.cpp \
            src/services/owncloud/owncloudfeed.cpp \
            src/services/owncloud/owncloudserviceentrypoint.cpp \
            src/services/owncloud/owncloudserviceroot.cpp \
            src/services/standard/atomparser.cpp \
            src/services/standard/feedparser.cpp \
            src/services/standard/gui/formstandardcategorydetails.cpp \
            src/services/standard/gui/formstandardfeeddetails.cpp \
            src/services/standard/gui/formstandardimportexport.cpp \
            src/services/standard/rdfparser.cpp \
            src/services/standard/rssparser.cpp \
            src/services/standard/standardcategory.cpp \
            src/services/standard/standardfeed.cpp \
            src/services/standard/standardfeedsimportexportmodel.cpp \
            src/services/standard/standardserviceentrypoint.cpp \
            src/services/standard/standardserviceroot.cpp \
            src/services/tt-rss/gui/formeditttrssaccount.cpp \
            src/services/tt-rss/gui/formttrssfeeddetails.cpp \
            src/services/tt-rss/network/ttrssnetworkfactory.cpp \
            src/services/tt-rss/ttrssfeed.cpp \
            src/services/tt-rss/ttrssserviceentrypoint.cpp \
            src/services/tt-rss/ttrssserviceroot.cpp \
            src/network-web/httpresponse.cpp \
            src/services/gmail/gui/formdownloadattachment.cpp \
            src/services/gmail/gui/formaddeditemail.cpp \
            src/gui/searchtextwidget.cpp

mac {
  OBJECTIVE_SOURCES += src/miscellaneous/disablewindowtabbing.mm
}

FORMS +=    src/gui/dialogs/formabout.ui \
            src/gui/dialogs/formaddaccount.ui \
            src/gui/dialogs/formbackupdatabasesettings.ui \
            src/gui/dialogs/formdatabasecleanup.ui \
            src/gui/dialogs/formmain.ui \
            src/gui/dialogs/formrestoredatabasesettings.ui \
            src/gui/dialogs/formsettings.ui \
            src/gui/dialogs/formupdate.ui \
            src/gui/settings/settingsbrowsermail.ui \
            src/gui/settings/settingsdatabase.ui \
            src/gui/settings/settingsdownloads.ui \
            src/gui/settings/settingsfeedsmessages.ui \
            src/gui/settings/settingsgeneral.ui \
            src/gui/settings/settingsgui.ui \
            src/gui/settings/settingslocalization.ui \
            src/gui/settings/settingsshortcuts.ui \
            src/gui/toolbareditor.ui \
            src/network-web/downloaditem.ui \
            src/network-web/downloadmanager.ui \
            src/services/abstract/gui/formfeeddetails.ui \
            src/services/gmail/gui/formeditgmailaccount.ui \
            src/services/inoreader/gui/formeditinoreaderaccount.ui \
            src/services/owncloud/gui/formeditowncloudaccount.ui \
            src/services/standard/gui/formstandardcategorydetails.ui \
            src/services/standard/gui/formstandardimportexport.ui \
            src/services/tt-rss/gui/formeditttrssaccount.ui \
            src/services/gmail/gui/formdownloadattachment.ui \
            src/services/gmail/gui/formaddeditemail.ui \
            src/gui/searchtextwidget.ui

equals(USE_WEBENGINE, true) {
  HEADERS +=    src/gui/locationlineedit.h \
                src/gui/webviewer.h \
                src/gui/webbrowser.h \
                src/gui/discoverfeedsbutton.h \
                src/network-web/googlesuggest.h \
                src/network-web/webpage.h \
                src/network-web/rssguardschemehandler.h \
                src/gui/dialogs/oauthlogin.h

  SOURCES +=    src/gui/locationlineedit.cpp \
                src/gui/webviewer.cpp \
                src/gui/webbrowser.cpp \
                src/gui/discoverfeedsbutton.cpp \
                src/network-web/googlesuggest.cpp \
                src/network-web/webpage.cpp \
                src/network-web/rssguardschemehandler.cpp \
                src/gui/dialogs/oauthlogin.cpp

  # Add AdBlock sources.
  HEADERS +=    src/network-web/adblock/adblockaddsubscriptiondialog.h \
                src/network-web/adblock/adblockdialog.h \
                src/network-web/adblock/adblockicon.h \
                src/network-web/adblock/adblockmanager.h \
                src/network-web/adblock/adblockmatcher.h \
                src/network-web/adblock/adblockrule.h \
                src/network-web/adblock/adblocksearchtree.h \
                src/network-web/adblock/adblocksubscription.h \
                src/network-web/adblock/adblocktreewidget.h \
                src/network-web/adblock/adblockurlinterceptor.h \
                src/network-web/urlinterceptor.h \
                src/network-web/networkurlinterceptor.h \
                src/miscellaneous/simpleregexp.h \
                src/gui/treewidget.h

  SOURCES +=    src/network-web/adblock/adblockaddsubscriptiondialog.cpp \
                src/network-web/adblock/adblockdialog.cpp \
                src/network-web/adblock/adblockicon.cpp \
                src/network-web/adblock/adblockmanager.cpp \
                src/network-web/adblock/adblockmatcher.cpp \
                src/network-web/adblock/adblockrule.cpp \
                src/network-web/adblock/adblocksearchtree.cpp \
                src/network-web/adblock/adblocksubscription.cpp \
                src/network-web/adblock/adblocktreewidget.cpp \
                src/network-web/adblock/adblockurlinterceptor.cpp \
                src/network-web/networkurlinterceptor.cpp \
                src/miscellaneous/simpleregexp.cpp \
                src/gui/treewidget.cpp

  FORMS +=      src/network-web/adblock/adblockaddsubscriptiondialog.ui \
                src/network-web/adblock/adblockdialog.ui \
                src/gui/dialogs/oauthlogin.ui
}
else {
  HEADERS +=    src/gui/messagepreviewer.h \
                src/gui/messagetextbrowser.h \
                src/gui/newspaperpreviewer.h \
                src/network-web/oauthhttphandler.h

  SOURCES +=    src/gui/messagepreviewer.cpp \
                src/gui/messagetextbrowser.cpp \
                src/gui/newspaperpreviewer.cpp \
                src/network-web/oauthhttphandler.cpp

  FORMS +=      src/gui/messagepreviewer.ui \
                src/gui/newspaperpreviewer.ui
}

TRANSLATIONS += $$PWD/localization/rssguard_cs.ts \
                $$PWD/localization/rssguard_da.ts \
                $$PWD/localization/rssguard_de.ts \
                $$PWD/localization/rssguard_en_GB.ts \
                $$PWD/localization/rssguard_en.ts \
                $$PWD/localization/rssguard_es.ts \
                $$PWD/localization/rssguard_fr.ts \
                $$PWD/localization/rssguard_he.ts \
                $$PWD/localization/rssguard_id.ts \
                $$PWD/localization/rssguard_it.ts \
                $$PWD/localization/rssguard_ja.ts \
                $$PWD/localization/rssguard_lt.ts \
                $$PWD/localization/rssguard_nl.ts \
                $$PWD/localization/rssguard_pl.ts \
                $$PWD/localization/rssguard_pt.ts \
                $$PWD/localization/rssguard_sv.ts \
                $$PWD/localization/rssguard_zh.ts

INCLUDEPATH +=  $$PWD/. \
                $$PWD/src \
                $$PWD/src/gui \
                $$PWD/src/gui/dialogs \
                $$PWD/src/dynamic-shortcuts

# Create new "make lupdate" target.
lupdate.target = lupdate
lupdate.commands = lupdate $$shell_path($$PWD/rssguard.pro)

QMAKE_EXTRA_TARGETS += lupdate

# Create new "make 7zip" target and "make zip" target.
win32 {
  seven_zip.target = 7zip
  seven_zip.depends = install
  seven_zip.commands = $$shell_path($$shell_quote($$PWD/resources/scripts/7za/7za.exe)) a -t7z $$TARGET-$$APP_VERSION-$$APP_REVISION-$${APP_WIN_ARCH}.7z $$shell_path($$PREFIX/*)

  zip.target = zip
  zip.depends = install
  zip.commands = $$shell_path($$shell_quote($$PWD/resources/scripts/7za/7za.exe)) a -tzip $$TARGET-$$APP_VERSION-$$APP_REVISION-$${APP_WIN_ARCH}.zip $$shell_path($$PREFIX/*)

  QMAKE_EXTRA_TARGETS += seven_zip zip
}

mac {
  dmg.target = dmg
  dmg.depends = install
  dmg.commands = macdeployqt $$shell_quote($$shell_path($$PREFIX)) -dmg

  QMAKE_EXTRA_TARGETS += dmg
}

# Create NSIS installer target on Windows.
win32 {
  nsis.target = nsis
  nsis.depends = install
  nsis.commands = \
    $$shell_path($$shell_quote($$PWD/resources/scripts/sed/sed.exe)) -e \"s|@APP_VERSION@|$$APP_VERSION|g; s|@APP_WIN_ARCH@|$$APP_WIN_ARCH|g; s|@APP_REVISION@|$$APP_REVISION|g; s|@APP_NAME@|$$APP_NAME|g; s|@APP_LOW_NAME@|$$APP_LOW_NAME|g; s|@EXE_NAME@|$${APP_LOW_NAME}.exe|g; s|@PWD@|$$replace(PWD, /, \\\\)|g; s|@OUT_PWD@|$$replace(OUT_PWD, /, \\\\)|g\" $$shell_path($$shell_quote($$PWD/resources/nsis/NSIS.definitions.nsh.in)) > $$shell_path($$shell_quote($$OUT_PWD/NSIS.definitions.nsh)) && \
    xcopy /Y $$shell_path($$shell_quote($$PWD/resources/nsis/NSIS.template.in)) $$shell_path($$shell_quote($$OUT_PWD/)) && \
    $$shell_path($$shell_quote($$PWD/resources/scripts/nsis/makensis.exe)) $$shell_path($$shell_quote($$OUT_PWD/NSIS.template.in))

  QMAKE_EXTRA_TARGETS += nsis
}

win32 {
  windows_all.target = windows_all
  windows_all.depends = seven_zip nsis
  windows_all.commands = echo "windows_all done..."

  QMAKE_EXTRA_TARGETS += windows_all
}

# Install all files on Windows.
win32 {
  target.path = $$PREFIX

  qt_dlls_root.files = resources/binaries/windows/qt5-msvc2015/*.*
  qt_dlls_root.path = $$quote($$PREFIX/)

  qt_dlls_plugins.files = resources/binaries/windows/qt5-msvc2015/*
  qt_dlls_plugins.path = $$quote($$PREFIX/)

  INSTALLS += target qt_dlls_root qt_dlls_plugins

  equals(USE_WEBENGINE, true) {
    # Copy extra resource files for QtWebEngine.
    qtwebengine_dlls.files = resources/binaries/windows/qt5-msvc2015-webengine/*
    qtwebengine_dlls.path = $$quote($$PREFIX/)

    qtwebengine.files = resources/binaries/windows/qt5-msvc2015-webengine/*.*
    qtwebengine.path = $$quote($$PREFIX/)

    INSTALLS += qtwebengine_dlls qtwebengine
  }
}

# Install all files on Linux.
unix:!mac:!android {
  target.path = $$PREFIX/bin

  desktop_file.files = resources/desktop/$${APP_REVERSE_NAME}.desktop
  desktop_file.path = $$quote($$PREFIX/share/applications/)

  appdata.files = resources/desktop/$${APP_REVERSE_NAME}.appdata.xml
  appdata.path = $$quote($$PREFIX/share/metainfo/)

  desktop_icon.files = resources/graphics/$${TARGET}.png
  desktop_icon.path = $$quote($$PREFIX/share/pixmaps/)

  INSTALLS += target desktop_file desktop_icon appdata
}

android {
  target.path = $$PREFIX

  INSTALLS += target
}

mac {
  IDENTIFIER = org.$${TARGET}.RSSGuard
  CONFIG -= app_bundle
  ICON = resources/macosx/$${TARGET}.icns
  QMAKE_MAC_SDK = macosx10.12
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
  LIBS += -framework AppKit

  target.path = $$quote($$PREFIX/Contents/MacOS/)

  # Install app icon.
  icns_icon.files = resources/macosx/$${TARGET}.icns
  icns_icon.path = $$quote($$PREFIX/Contents/Resources/)

  # Install Info.plist.
  info_plist.files = resources/macosx/Info.plist.in
  info_plist.path  = $$quote($$PREFIX/Contents/)

  # Process the just installed Info.plist.
  info_plist2.extra = @sed -e "s,@EXECUTABLE@,$$TARGET,g" -e "s,@SHORT_VERSION@,$$APP_VERSION,g" -e "s,@APP_NAME@,\"$$APP_NAME\",g" -e "s,@ICON@,$$basename(ICON),g"  -e "s,@TYPEINFO@,"????",g" $$shell_quote($$PREFIX/Contents/Info.plist.in) > $$shell_quote($$PREFIX/Contents/Info.plist) && \
                      rm -f $$shell_quote($$PREFIX/Contents/Info.plist.in)
  info_plist2.path = $$quote($$PREFIX/Contents/)

  # Install PkgInfo
  pkginfo.extra = @printf "APPL????" > $$shell_quote($$PREFIX/Contents/PkgInfo)
  pkginfo.path = $$quote($$PREFIX/Contents/)

  INSTALLS += target icns_icon info_plist info_plist2 pkginfo
}
