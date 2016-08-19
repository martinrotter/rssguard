#################################################################
#
# This file is part of RSS Guard.
#
# Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
#
# RSS Guard is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RSS Guard is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with RSS Guard. If not, see <http:# www.gnu.org/licenses/>.
#
#
#  This is RSS Guard compilation script for qmake.
#
# Usage:
#   a) DEBUG build for testing. (out of source build type)
#     cd ../build-dir
#     qmake ../rssguard-dir/rssguard.pro -r CONFIG+=debug PREFIX=./usr
#     make
#     make install
#
#   b) RELEASE build for production use. (out of source build type)
#     cd ../build-dir
#     qmake ../rssguard-dir/rssguard.pro -r CONFIG+=release PREFIX=./usr
#     make
#     make install
#
# Variables:
#   USE_WEBENGINE - if specified, then QtWebEngine module for internal web browser is used.
#                   Otherwise simple text component is used and some features will be disabled.
#   PREFIX - specifies base folder to which files are copied during "make install"
#            step, defaults to "$$OUT_PWD/usr" on Linux and to "$$OUT_PWD/app" on Windows.
#   LRELEASE_EXECUTABLE - specifies the name/path of "lrelease" executable, defaults to "lrelease".
#
#
#
# Other information:
#   - supports Windows, Linux, Mac OS X,
#   - Qt 5.7 and higher is required,
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

lessThan(QT_MAJOR_VERSION, 5)|lessThan(QT_MINOR_VERSION, 6) {
  warning(rssguard: At least Qt "5.6.0" is required. Expect errors!!!)
}

lessThan(QT_MAJOR_VERSION, 5)|lessThan(QT_MINOR_VERSION, 7) {
  warning(rssguard: You have Qt \"$$QT_VERSION\". Some features will be disabled.)
}

APP_NAME                      = "RSS Guard"
APP_LOW_NAME                  = "rssguard"
APP_LOW_H_NAME                = ".rssguard"
APP_AUTHOR                    = "Martin Rotter"
APP_COPYRIGHT                 = "(C) 2011-2016 $$APP_AUTHOR"
APP_VERSION                   = "3.3.4"
APP_LONG_NAME                 = "$$APP_NAME $$APP_VERSION"
APP_EMAIL                     = "rotter.martinos@gmail.com"
APP_URL                       = "https://github.com/martinrotter/rssguard"
APP_URL_ISSUES                = "https://github.com/martinrotter/rssguard/issues"
APP_URL_ISSUES_NEW            = "https://github.com/martinrotter/rssguard/issues/new"
APP_URL_WIKI                  = "https://github.com/martinrotter/rssguard/wiki"
APP_USERAGENT                 = "RSS Guard/$$APP_VERSION (github.com/martinrotter/rssguard)"
APP_DONATE_URL                = "https://goo.gl/YFVJ0j"

isEmpty(PREFIX) {
  message(rssguard: PREFIX variable is not set. This might indicate error.)

  win32 {
    PREFIX = $$OUT_PWD/app
  }

  unix:!mac {
    PREFIX = $$OUT_PWD/usr
  }
}

isEmpty(DESTDIR) {
  unix:!mac {
    DESTDIR = $$OUT_PWD/bin
  }
}

isEmpty(USE_WEBENGINE) {
  USE_WEBENGINE = true
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
  APP_REVISION = "-"
}

DEFINES += APP_REVISION='"\\\"$$APP_REVISION\\\""'

message(rssguard: RSS Guard version is: \"$$APP_VERSION\".)
message(rssguard: Detected Qt version: \"$$QT_VERSION\".)
message(rssguard: Build destination directory: \"$$DESTDIR\".)
message(rssguard: Prefix directory: \"$$PREFIX\".)
message(rssguard: Build revision: \"$$APP_REVISION\".)
message(rssguard: lrelease executable name: \"$$LRELEASE_EXECUTABLE\".)

QT += core gui widgets sql network xml printsupport
CONFIG *= c++11 debug_and_release warn_on
DEFINES *= QT_USE_QSTRINGBUILDER QT_USE_FAST_CONCATENATION QT_USE_FAST_OPERATOR_PLUS UNICODE _UNICODE
VERSION = $$APP_VERSION

MOC_DIR = $$OUT_PWD/moc
RCC_DIR = $$OUT_PWD/rcc
UI_DIR = $$OUT_PWD/ui

equals(USE_WEBENGINE, true) {
  message(rssguard: Application will be compiled WITH QtWebEngine module.)
  QT += webenginewidgets
  DEFINES += USE_WEBENGINE
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

HEADERS +=  src/core/feeddownloader.h \
            src/core/feedsmodel.h \
            src/core/feedsproxymodel.h \
            src/core/message.h \
            src/core/messagesmodel.h \
            src/core/messagesproxymodel.h \
            src/core/parsingfactory.h \
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
            src/gui/labelwithstatus.h \
            src/gui/lineeditwithstatus.h \
            src/gui/messagebox.h \
            src/gui/messagessearchlineedit.h \
            src/gui/messagestoolbar.h \
            src/gui/messagesview.h \
            src/gui/plaintoolbutton.h \
            src/gui/squeezelabel.h \
            src/gui/statusbar.h \
            src/gui/styleditemdelegatewithoutfocus.h \
            src/gui/systemtrayicon.h \
            src/gui/tabbar.h \
            src/gui/tabcontent.h \
            src/gui/tabwidget.h \
            src/gui/timespinbox.h \
            src/gui/toolbareditor.h \
            src/gui/widgetwithstatus.h \
            src/miscellaneous/application.h \
            src/miscellaneous/autosaver.h \
            src/miscellaneous/databasecleaner.h \
            src/miscellaneous/databasefactory.h \
            src/miscellaneous/databasequeries.h \
            src/miscellaneous/debugging.h \
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
            src/network-web/silentnetworkaccessmanager.h \
            src/network-web/webfactory.h \
            src/qtsingleapplication/qtlocalpeer.h \
            src/qtsingleapplication/qtlockedfile.h \
            src/qtsingleapplication/qtsingleapplication.h \
            src/qtsingleapplication/qtsinglecoreapplication.h \
            src/services/abstract/accountcheckmodel.h \
            src/services/abstract/category.h \
            src/services/abstract/feed.h \
            src/services/abstract/gui/formfeeddetails.h \
            src/services/abstract/recyclebin.h \
            src/services/abstract/rootitem.h \
            src/services/abstract/serviceentrypoint.h \
            src/services/abstract/serviceroot.h \
            src/services/owncloud/definitions.h \
            src/services/owncloud/gui/formeditowncloudaccount.h \
            src/services/owncloud/gui/formowncloudfeeddetails.h \
            src/services/owncloud/network/owncloudnetworkfactory.h \
            src/services/owncloud/owncloudcategory.h \
            src/services/owncloud/owncloudfeed.h \
            src/services/owncloud/owncloudrecyclebin.h \
            src/services/owncloud/owncloudserviceentrypoint.h \
            src/services/owncloud/owncloudserviceroot.h \
            src/services/standard/gui/formstandardcategorydetails.h \
            src/services/standard/gui/formstandardfeeddetails.h \
            src/services/standard/gui/formstandardimportexport.h \
            src/services/standard/standardcategory.h \
            src/services/standard/standardfeed.h \
            src/services/standard/standardfeedsimportexportmodel.h \
            src/services/standard/standardserviceentrypoint.h \
            src/services/standard/standardserviceroot.h \
            src/services/tt-rss/definitions.h \
            src/services/tt-rss/gui/formeditaccount.h \
            src/services/tt-rss/gui/formttrssfeeddetails.h \
            src/services/tt-rss/network/ttrssnetworkfactory.h \
            src/services/tt-rss/ttrsscategory.h \
            src/services/tt-rss/ttrssfeed.h \
            src/services/tt-rss/ttrssrecyclebin.h \
            src/services/tt-rss/ttrssserviceentrypoint.h \
            src/services/tt-rss/ttrssserviceroot.h \
            src/gui/settings/settingspanel.h \
            src/gui/settings/settingsgeneral.h \
            src/gui/settings/settingsdatabase.h \
            src/gui/settings/settingsshortcuts.h \
            src/gui/settings/settingsgui.h \
            src/gui/settings/settingslocalization.h \
            src/gui/settings/settingsbrowsermail.h \
            src/gui/settings/settingsfeedsmessages.h \
            src/gui/settings/settingsdownloads.h \
            src/miscellaneous/feedreader.h

SOURCES +=  src/core/feeddownloader.cpp \
            src/core/feedsmodel.cpp \
            src/core/feedsproxymodel.cpp \
            src/core/message.cpp \
            src/core/messagesmodel.cpp \
            src/core/messagesproxymodel.cpp \
            src/core/parsingfactory.cpp \
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
            src/gui/labelwithstatus.cpp \
            src/gui/lineeditwithstatus.cpp \
            src/gui/messagebox.cpp \
            src/gui/messagessearchlineedit.cpp \
            src/gui/messagestoolbar.cpp \
            src/gui/messagesview.cpp \
            src/gui/plaintoolbutton.cpp \
            src/gui/squeezelabel.cpp \
            src/gui/statusbar.cpp \
            src/gui/styleditemdelegatewithoutfocus.cpp \
            src/gui/systemtrayicon.cpp \
            src/gui/tabbar.cpp \
            src/gui/tabcontent.cpp \
            src/gui/tabwidget.cpp \
            src/gui/timespinbox.cpp \
            src/gui/toolbareditor.cpp \
            src/gui/widgetwithstatus.cpp \
            src/main.cpp \
            src/miscellaneous/application.cpp \
            src/miscellaneous/autosaver.cpp \
            src/miscellaneous/databasecleaner.cpp \
            src/miscellaneous/databasefactory.cpp \
            src/miscellaneous/databasequeries.cpp \
            src/miscellaneous/debugging.cpp \
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
            src/network-web/silentnetworkaccessmanager.cpp \
            src/network-web/webfactory.cpp \
            src/qtsingleapplication/qtlocalpeer.cpp \
            src/qtsingleapplication/qtlockedfile.cpp \
            src/qtsingleapplication/qtsingleapplication.cpp \
            src/qtsingleapplication/qtsinglecoreapplication.cpp \
            src/services/abstract/accountcheckmodel.cpp \
            src/services/abstract/category.cpp \
            src/services/abstract/feed.cpp \
            src/services/abstract/gui/formfeeddetails.cpp \
            src/services/abstract/recyclebin.cpp \
            src/services/abstract/rootitem.cpp \
            src/services/abstract/serviceentrypoint.cpp \
            src/services/abstract/serviceroot.cpp \
            src/services/owncloud/gui/formeditowncloudaccount.cpp \
            src/services/owncloud/gui/formowncloudfeeddetails.cpp \
            src/services/owncloud/network/owncloudnetworkfactory.cpp \
            src/services/owncloud/owncloudcategory.cpp \
            src/services/owncloud/owncloudfeed.cpp \
            src/services/owncloud/owncloudrecyclebin.cpp \
            src/services/owncloud/owncloudserviceentrypoint.cpp \
            src/services/owncloud/owncloudserviceroot.cpp \
            src/services/standard/gui/formstandardcategorydetails.cpp \
            src/services/standard/gui/formstandardfeeddetails.cpp \
            src/services/standard/gui/formstandardimportexport.cpp \
            src/services/standard/standardcategory.cpp \
            src/services/standard/standardfeed.cpp \
            src/services/standard/standardfeedsimportexportmodel.cpp \
            src/services/standard/standardserviceentrypoint.cpp \
            src/services/standard/standardserviceroot.cpp \
            src/services/tt-rss/gui/formeditaccount.cpp \
            src/services/tt-rss/gui/formttrssfeeddetails.cpp \
            src/services/tt-rss/network/ttrssnetworkfactory.cpp \
            src/services/tt-rss/ttrsscategory.cpp \
            src/services/tt-rss/ttrssfeed.cpp \
            src/services/tt-rss/ttrssrecyclebin.cpp \
            src/services/tt-rss/ttrssserviceentrypoint.cpp \
            src/services/tt-rss/ttrssserviceroot.cpp \
            src/gui/settings/settingspanel.cpp \
            src/gui/settings/settingsgeneral.cpp \
            src/gui/settings/settingsdatabase.cpp \
            src/gui/settings/settingsshortcuts.cpp \
            src/gui/settings/settingsgui.cpp \
            src/gui/settings/settingslocalization.cpp \
            src/gui/settings/settingsbrowsermail.cpp \
            src/gui/settings/settingsfeedsmessages.cpp \
            src/gui/settings/settingsdownloads.cpp \
            src/miscellaneous/feedreader.cpp

FORMS +=    src/gui/toolbareditor.ui \
            src/network-web/downloaditem.ui \
            src/network-web/downloadmanager.ui \
            src/gui/dialogs/formabout.ui \
            src/gui/dialogs/formaddaccount.ui \
            src/gui/dialogs/formbackupdatabasesettings.ui \
            src/gui/dialogs/formdatabasecleanup.ui \
            src/gui/dialogs/formmain.ui \
            src/gui/dialogs/formrestoredatabasesettings.ui \
            src/gui/dialogs/formsettings.ui \
            src/gui/dialogs/formupdate.ui \
            src/services/abstract/gui/formfeeddetails.ui \
            src/services/owncloud/gui/formeditowncloudaccount.ui \
            src/services/standard/gui/formstandardcategorydetails.ui \
            src/services/standard/gui/formstandardimportexport.ui \
            src/services/tt-rss/gui/formeditaccount.ui \
            src/gui/settings/settingsgeneral.ui \
            src/gui/settings/settingsdatabase.ui \
            src/gui/settings/settingsshortcuts.ui \
            src/gui/settings/settingsgui.ui \
            src/gui/settings/settingslocalization.ui \
            src/gui/settings/settingsbrowsermail.ui \
            src/gui/settings/settingsfeedsmessages.ui \
            src/gui/settings/settingsdownloads.ui

equals(USE_WEBENGINE, true) {
  HEADERS +=    src/gui/locationlineedit.h \
                src/gui/webviewer.h \
                src/gui/webbrowser.h \
                src/gui/discoverfeedsbutton.h \
                src/network-web/googlesuggest.h \
                src/network-web/webpage.h

  SOURCES +=    src/gui/locationlineedit.cpp \
                src/gui/webviewer.cpp \
                src/gui/webbrowser.cpp \
                src/gui/discoverfeedsbutton.cpp \
                src/network-web/googlesuggest.cpp \
                src/network-web/webpage.cpp
}
else {
  HEADERS +=    src/gui/messagepreviewer.h \
                src/gui/messagetextbrowser.h \
                src/gui/newspaperpreviewer.h

  SOURCES +=    src/gui/messagepreviewer.cpp \
                src/gui/messagetextbrowser.cpp \
                src/gui/newspaperpreviewer.cpp

  FORMS +=      src/gui/messagepreviewer.ui \
                src/gui/newspaperpreviewer.ui
}

TRANSLATIONS += localization/qtbase-cs.ts \
                localization/qtbase-da.ts \
                localization/qtbase-de.ts \
                localization/qtbase-fr.ts \
                localization/qtbase-he.ts \
                localization/qtbase-it.ts \
                localization/qtbase-ja.ts \
                localization/qtbase-sv.ts \
                localization/rssguard-cs.ts \
                localization/rssguard-da.ts \
                localization/rssguard-de.ts \
                localization/rssguard-en_GB.ts \
                localization/rssguard-en.ts \
                localization/rssguard-fr.ts \
                localization/rssguard-he.ts \
                localization/rssguard-id.ts \
                localization/rssguard-it.ts \
                localization/rssguard-ja.ts \
                localization/rssguard-lt.ts \
                localization/rssguard-nl.ts \
                localization/rssguard-pt.ts \
                localization/rssguard-sv.ts

TRANSLATIONS_WO_QT += $$PWD/localization/rssguard-cs.ts \
                      $$PWD/localization/rssguard-da.ts \
                      $$PWD/localization/rssguard-de.ts \
                      $$PWD/localization/rssguard-en_GB.ts \
                      $$PWD/localization/rssguard-en.ts \
                      $$PWD/localization/rssguard-fr.ts \
                      $$PWD/localization/rssguard-he.ts \
                      $$PWD/localization/rssguard-id.ts \
                      $$PWD/localization/rssguard-it.ts \
                      $$PWD/localization/rssguard-ja.ts \
                      $$PWD/localization/rssguard-lt.ts \
                      $$PWD/localization/rssguard-nl.ts \
                      $$PWD/localization/rssguard-pt.ts \
                      $$PWD/localization/rssguard-sv.ts

INCLUDEPATH +=  $$PWD/. \
                $$PWD/src \
                $$PWD/src/gui \
                $$PWD/src/gui/dialogs \
                $$PWD/src/dynamic-shortcuts

TEXTS = resources/text/CHANGELOG \
        resources/text/COPYING_BSD \
        resources/text/COPYING_GNU_GPL \
        resources/text/COPYING_GNU_GPL_HTML

# Make sure QM translations are generated.
lrelease.input = TRANSLATIONS
lrelease.output = $$OUT_PWD/l10n/${QMAKE_FILE_BASE}.qm
lrelease.commands = $$LRELEASE_EXECUTABLE -compress ${QMAKE_FILE_IN} -qm $$OUT_PWD/l10n/${QMAKE_FILE_BASE}.qm
lrelease.CONFIG += no_link target_predeps

# Create new "make lupdate" target.
lupdate.target = lupdate
lupdate.commands = lupdate -no-obsolete $$shell_path($$PWD/rssguard.pro) -ts $$shell_path($$TRANSLATIONS_WO_QT)

QMAKE_EXTRA_TARGETS += lupdate
QMAKE_EXTRA_COMPILERS += lrelease

# Create new "make 7zip" target and "make zip" target.
win32 {
  seven_zip.target = 7zip
  seven_zip.depends = install
  seven_zip.commands = $$shell_path($$shell_quote($$PWD/resources/scripts/7za/7za.exe)) a -t7z $$TARGET-$$APP_VERSION-win32.7z $$shell_path($$PREFIX/*)

  zip.target = zip
  zip.depends = install
  zip.commands = $$shell_path($$shell_quote($$PWD/resources/scripts/7za/7za.exe)) a -tzip $$TARGET-$$APP_VERSION-win32.zip $$shell_path($$PREFIX/*)

  QMAKE_EXTRA_TARGETS += seven_zip zip
}

# Create NSIS installer target on Windows.
win32 {
  nsis.target = nsis
  nsis.depends = install
  nsis.commands = \
    $$shell_path($$shell_quote($$PWD/resources/scripts/sed/sed.exe)) -e \"s|@APP_VERSION@|$$APP_VERSION|g; s|@APP_NAME@|$$APP_NAME|g; s|@APP_LOW_NAME@|$$APP_LOW_NAME|g; s|@EXE_NAME@|$${APP_LOW_NAME}.exe|g; s|@PWD@|$$replace(PWD, /, \\\\)|g; s|@OUT_PWD@|$$replace(OUT_PWD, /, \\\\)|g\" $$shell_path($$shell_quote($$PWD/resources/nsis/NSIS.definitions.nsh.in)) > $$shell_path($$shell_quote($$OUT_PWD/NSIS.definitions.nsh)) && \
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

  misc_sql.files = resources/misc/*.sql
  misc_sql.path = $$PREFIX/misc

  qt_dlls_root.files = resources/binaries/windows/qt5-msvc2013/*.dll
  qt_dlls_root.path = $$quote($$PREFIX/)

  qt_exe_root.files = resources/binaries/windows/qt5-msvc2013/*.exe
  qt_exe_root.path = $$quote($$PREFIX/)

  qt_dlls_bearer.files = resources/binaries/windows/qt5-msvc2013/bearer
  qt_dlls_bearer.path = $$quote($$PREFIX/)

  qt_dlls_iconengines.files = resources/binaries/windows/qt5-msvc2013/iconengines
  qt_dlls_iconengines.path = $$quote($$PREFIX/)

  qt_dlls_imageformats.files = resources/binaries/windows/qt5-msvc2013/imageformats
  qt_dlls_imageformats.path = $$quote($$PREFIX/)

  qt_dlls_platforms.files = resources/binaries/windows/qt5-msvc2013/platforms/
  qt_dlls_platforms.path = $$quote($$PREFIX/)

  qt_dlls_sqldrivers.files = resources/binaries/windows/qt5-msvc2013/sqldrivers
  qt_dlls_sqldrivers.path = $$quote($$PREFIX/)

  qt_dlls_translations.files = resources/binaries/windows/qt5-msvc2013/translations
  qt_dlls_translations.path = $$quote($$PREFIX/)

  qt_dlls_resources.files = resources/binaries/windows/qt5-msvc2013/resources
  qt_dlls_resources.path = $$quote($$PREFIX/)

  misc_icons.files = resources/graphics/misc
  misc_icons.path = $$quote($$PREFIX/icons/)

  faenza.files = resources/graphics/Faenza
  faenza.path = $$quote($$PREFIX/icons/)

  skins.files = resources/skins/base
  skins.path = $$quote($$PREFIX/skins/)

  sql.files = resources/misc
  sql.path = $$quote($$PREFIX/)

  feeds.files = resources/initial_feeds
  feeds.path = $$quote($$PREFIX/)

  texts.files = $$TEXTS
  texts.path = $$quote($$PREFIX/)

  ico.files = resources/graphics/$${TARGET}.ico
  ico.path = $$quote($$PREFIX/)

  app_icon.files = resources/graphics/$${TARGET}.png
  app_icon.path = $$quote($$PREFIX/)

  app_plain_icon.files = resources/graphics/$${TARGET}_plain.png
  app_plain_icon.path = $$quote($$PREFIX/)

  translations.files = $$OUT_PWD/l10n
  translations.path = $$quote($$PREFIX/)

  INSTALLS += target misc_sql qt_dlls_root qt_exe_root qt_dlls_bearer qt_dlls_iconengines \
              qt_dlls_imageformats qt_dlls_platforms qt_dlls_sqldrivers \
              qt_dlls_translations qt_dlls_resources misc_icons faenza skins \
              sql feeds texts ico app_icon app_plain_icon translations
}

# Install all files on Linux.
unix:!mac {
  target.path = $$PREFIX/bin

  # Install SQL initializers.
  misc_sql.files = resources/misc/*.sql
  misc_sql.path = $$quote($$PREFIX/share/$$TARGET/misc/)

  # Misc icons.
  misc_icons.files = resources/graphics/misc
  misc_icons.path = $$quote($$PREFIX/share/$$TARGET/icons/)

  # Initial feeds.
  misc_feeds.files = resources/initial_feeds
  misc_feeds.path = $$quote($$PREFIX/share/$$TARGET/)

  misc_icon.files = resources/graphics/$${TARGET}.png
  misc_icon.path = $$quote($$PREFIX/share/pixmaps/)

  skins.files = resources/skins
  skins.path = $$quote($$PREFIX/share/$$TARGET/)

  misc_plain_icon.files = resources/graphics/$${TARGET}_plain.png
  misc_plain_icon.path = $$quote($$PREFIX/share/$$TARGET/icons/)

  misc_texts.files = $$TEXTS
  misc_texts.path = $$quote($$PREFIX/share/$$TARGET/information/)

  desktop_file.files = resources/desktop/$${TARGET}.desktop
  desktop_file.path = $$quote($$PREFIX/share/applications/)

  desktop_file_autostart.files = resources/desktop/$${TARGET}.desktop.autostart
  desktop_file_autostart.path = $$quote($$PREFIX/share/$${TARGET}/autostart/)

  translations.files = $$OUT_PWD/l10n
  translations.path = $$quote($$PREFIX/share/$$TARGET/)

  INSTALLS += target misc_sql misc_icons misc_feeds \
              misc_icon misc_plain_icon skins misc_texts \
              desktop_file desktop_file_autostart translations
}

mac {
  CONFIG += app_bundle
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8

  QMAKE_INFO_PLIST = resources/macosx/Info.plist.in
  ICON = resources/macosx/$${TARGET}.icns
  IDENTIFIER = org.$${TARGET}.RSSGuard

  # Install SQL initializers.
  misc_sql.files = resources/misc
  misc_sql.path = Contents/Resources

  # Misc icons.
  misc_icons.files = resources/graphics/misc
  misc_icons.path = Contents/Resources/icons

  faenza.files = resources/graphics/Faenza
  faenza.path = Contents/Resources/icons

  # Initial feeds.
  misc_feeds.files = resources/initial_feeds
  misc_feeds.path = Contents/Resources

  skins.files = resources/skins
  skins.path = Contents/Resources

  misc_icon.files = resources/graphics/$${TARGET}.png
  misc_icon.path = Contents/Resources/icons

  misc_plain_icon.files = resources/graphics/$${TARGET}_plain.png
  misc_plain_icon.path = Contents/Resources/icons

  misc_texts.files = $$TEXTS
  misc_texts.path = Contents/Resources/information

  translations.files = $$OUT_PWD/l10n
  translations.path =  Contents/Resources

  QMAKE_BUNDLE_DATA += misc_sql misc_icons faenza misc_feeds skins \
                       misc_icon misc_plain_icon misc_texts translations

  INSTALLS += misc_sql misc_icons faenza misc_feeds skins \
              misc_icon misc_plain_icon misc_texts translations
}
