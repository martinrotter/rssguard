# Setup all public headers, this needs to be kept in
# sync with truly used headers.
INSTALL_HEADERS = \
../librssguard/3rd-party/boolinq/boolinq.h \
../librssguard/3rd-party/mimesis/mimesis.hpp \
../librssguard/3rd-party/mimesis/quoted-printable.hpp \
../librssguard/core/feeddownloader.h \
../librssguard/core/feedsmodel.h \
../librssguard/core/feedsproxymodel.h \
../librssguard/core/message.h \
../librssguard/core/messagefilter.h \
../librssguard/core/messagesmodel.h \
../librssguard/core/messagesmodelcache.cpp \
../librssguard/core/messagesmodelcache.h \
../librssguard/core/messagesmodelsqllayer.h \
../librssguard/core/messagesproxymodel.h \
../librssguard/definitions/definitions.h \
../librssguard/dynamic-shortcuts \
../librssguard/dynamic-shortcuts/dynamicshortcuts.cpp \
../librssguard/dynamic-shortcuts/dynamicshortcuts.h \
../librssguard/dynamic-shortcuts/dynamicshortcutswidget.cpp \
../librssguard/dynamic-shortcuts/dynamicshortcutswidget.h \
../librssguard/dynamic-shortcuts/shortcutcatcher.cpp \
../librssguard/dynamic-shortcuts/shortcutcatcher.h \
../librssguard/exceptions/applicationexception.h \
../librssguard/exceptions/filteringexception.h \
../librssguard/exceptions/ioexception.h \
../librssguard/gui/baselineedit.h \
../librssguard/gui/basetoolbar.h \
../librssguard/gui/colortoolbutton.h \
../librssguard/gui/comboboxwithstatus.cpp \
../librssguard/gui/comboboxwithstatus.h \
../librssguard/gui/dialogs/formabout.h \
../librssguard/gui/dialogs/formaddaccount.h \
../librssguard/gui/dialogs/formaddeditlabel.h \
../librssguard/gui/dialogs/formbackupdatabasesettings.h \
../librssguard/gui/dialogs/formdatabasecleanup.h \
../librssguard/gui/dialogs/formmain.h \
../librssguard/gui/dialogs/formmessagefiltersmanager.h \
../librssguard/gui/dialogs/formrestoredatabasesettings.h \
../librssguard/gui/dialogs/formsettings.h \
../librssguard/gui/dialogs/formupdate.h \
../librssguard/gui/discoverfeedsbutton.h \
../librssguard/gui/edittableview.h \
../librssguard/gui/feedmessageviewer.h \
../librssguard/gui/feedstoolbar.h \
../librssguard/gui/feedsview.h \
../librssguard/gui/guiutilities.h \
../librssguard/gui/labelwithstatus.cpp \
../librssguard/gui/labelwithstatus.h \
../librssguard/gui/lineeditwithstatus.cpp \
../librssguard/gui/lineeditwithstatus.h \
../librssguard/gui/locationlineedit.h \
../librssguard/gui/messagebox.h \
../librssguard/gui/messagebrowser.h \
../librssguard/gui/messagepreviewer.h \
../librssguard/gui/messagessearchlineedit.cpp \
../librssguard/gui/messagessearchlineedit.h \
../librssguard/gui/messagestoolbar.h \
../librssguard/gui/messagesview.h \
../librssguard/gui/messagetextbrowser.h \
../librssguard/gui/newspaperpreviewer.h \
../librssguard/gui/plaintoolbutton.h \
../librssguard/gui/searchtextwidget.cpp \
../librssguard/gui/searchtextwidget.h \
../librssguard/gui/searchtextwidget.ui \
../librssguard/gui/settings/settingsbrowsermail.h \
../librssguard/gui/settings/settingsdatabase.h \
../librssguard/gui/settings/settingsdownloads.h \
../librssguard/gui/settings/settingsfeedsmessages.h \
../librssguard/gui/settings/settingsgeneral.h \
../librssguard/gui/settings/settingsgui.h \
../librssguard/gui/settings/settingslocalization.h \
../librssguard/gui/settings/settingspanel.h \
../librssguard/gui/settings/settingsshortcuts.cpp \
../librssguard/gui/settings/settingsshortcuts.h \
../librssguard/gui/settings/settingsshortcuts.ui \
../librssguard/gui/squeezelabel.h \
../librssguard/gui/statusbar.h \
../librssguard/gui/styleditemdelegatewithoutfocus.cpp \
../librssguard/gui/styleditemdelegatewithoutfocus.h \
../librssguard/gui/systemtrayicon.h \
../librssguard/gui/tabbar.h \
../librssguard/gui/tabcontent.h \
../librssguard/gui/tabwidget.h \
../librssguard/gui/timespinbox.h \
../librssguard/gui/toolbareditor.h \
../librssguard/gui/treeviewcolumnsmenu.h \
../librssguard/gui/treewidget.h \
../librssguard/gui/webbrowser.h \
../librssguard/gui/webviewer.h \
../librssguard/gui/widgetwithstatus.cpp \
../librssguard/gui/widgetwithstatus.h \
../librssguard/miscellaneous/application.h \
../librssguard/miscellaneous/autosaver.h \
../librssguard/miscellaneous/databasecleaner.h \
../librssguard/miscellaneous/databasefactory.h \
../librssguard/miscellaneous/databasequeries.h \
../librssguard/miscellaneous/externaltool.h \
../librssguard/miscellaneous/feedreader.h \
../librssguard/miscellaneous/iconfactory.h \
../librssguard/miscellaneous/iofactory.h \
../librssguard/miscellaneous/localization.h \
../librssguard/miscellaneous/mutex.h \
../librssguard/miscellaneous/regexfactory.h \
../librssguard/miscellaneous/settings.h \
../librssguard/miscellaneous/settingsproperties.h \
../librssguard/miscellaneous/simplecrypt/simplecrypt.h \
../librssguard/miscellaneous/skinfactory.h \
../librssguard/miscellaneous/systemfactory.h \
../librssguard/miscellaneous/templates.h \
../librssguard/miscellaneous/textfactory.h \
../librssguard/network-web/adblock/adblockaddsubscriptiondialog.h \
../librssguard/network-web/adblock/adblockdialog.h \
../librssguard/network-web/adblock/adblockicon.h \
../librssguard/network-web/adblock/adblockmanager.h \
../librssguard/network-web/adblock/adblockmatcher.cpp \
../librssguard/network-web/adblock/adblockmatcher.h \
../librssguard/network-web/adblock/adblockrule.h \
../librssguard/network-web/adblock/adblocksearchtree.cpp \
../librssguard/network-web/adblock/adblocksearchtree.h \
../librssguard/network-web/adblock/adblocksubscription.h \
../librssguard/network-web/adblock/adblocktreewidget.h \
../librssguard/network-web/adblock/adblockurlinterceptor.h \
../librssguard/network-web/basenetworkaccessmanager.h \
../librssguard/network-web/downloader.h \
../librssguard/network-web/downloadmanager.h \
../librssguard/network-web/googlesuggest.h \
../librssguard/network-web/httpresponse.cpp \
../librssguard/network-web/httpresponse.h \
../librssguard/network-web/networkfactory.h \
../librssguard/network-web/networkurlinterceptor.h \
../librssguard/network-web/oauth2service.cpp \
../librssguard/network-web/oauth2service.h \
../librssguard/network-web/oauthhttphandler.cpp \
../librssguard/network-web/oauthhttphandler.h \
../librssguard/network-web/rssguardschemehandler.cpp \
../librssguard/network-web/rssguardschemehandler.h \
../librssguard/network-web/silentnetworkaccessmanager.h \
../librssguard/network-web/urlinterceptor.h \
../librssguard/network-web/webfactory.h \
../librssguard/network-web/webpage.h \
../librssguard/qtsingleapplication/qtlocalpeer.h \
../librssguard/qtsingleapplication/qtlockedfile.h \
../librssguard/qtsingleapplication/qtsingleapplication.h \
../librssguard/qtsingleapplication/qtsinglecoreapplication.h \
../librssguard/services/abstract/accountcheckmodel.cpp \
../librssguard/services/abstract/accountcheckmodel.h \
../librssguard/services/abstract/cacheforserviceroot.cpp \
../librssguard/services/abstract/cacheforserviceroot.h \
../librssguard/services/abstract/category.h \
../librssguard/services/abstract/feed.h \
../librssguard/services/abstract/gui/formfeeddetails.h \
../librssguard/services/abstract/importantnode.h \
../librssguard/services/abstract/label.h \
../librssguard/services/abstract/labelsnode.h \
../librssguard/services/abstract/recyclebin.h \
../librssguard/services/abstract/rootitem.h \
../librssguard/services/abstract/serviceentrypoint.h \
../librssguard/services/abstract/serviceroot.h \
../librssguard/services/gmail/definitions.h \
../librssguard/services/gmail/gmailentrypoint.h \
../librssguard/services/gmail/gmailfeed.h \
../librssguard/services/gmail/gmailserviceroot.h \
../librssguard/services/gmail/gui/emailrecipientcontrol.h \
../librssguard/services/gmail/gui/formaddeditemail.h \
../librssguard/services/gmail/gui/formdownloadattachment.cpp \
../librssguard/services/gmail/gui/formdownloadattachment.h \
../librssguard/services/gmail/gui/formdownloadattachment.ui \
../librssguard/services/gmail/gui/formeditgmailaccount.h \
../librssguard/services/gmail/network/gmailnetworkfactory.h \
../librssguard/services/inoreader/definitions.h \
../librssguard/services/inoreader/gui/formeditinoreaderaccount.h \
../librssguard/services/inoreader/inoreaderentrypoint.h \
../librssguard/services/inoreader/inoreaderfeed.h \
../librssguard/services/inoreader/inoreaderserviceroot.h \
../librssguard/services/inoreader/network/inoreadernetworkfactory.h \
../librssguard/services/owncloud/definitions.h \
../librssguard/services/owncloud/gui/formeditowncloudaccount.h \
../librssguard/services/owncloud/gui/formowncloudfeeddetails.h \
../librssguard/services/owncloud/network/owncloudnetworkfactory.h \
../librssguard/services/owncloud/owncloudfeed.h \
../librssguard/services/owncloud/owncloudserviceentrypoint.h \
../librssguard/services/owncloud/owncloudserviceroot.h \
../librssguard/services/standard/atomparser.h \
../librssguard/services/standard/feedparser.h \
../librssguard/services/standard/gui/formstandardcategorydetails.h \
../librssguard/services/standard/gui/formstandardfeeddetails.h \
../librssguard/services/standard/gui/formstandardimportexport.h \
../librssguard/services/standard/jsonparser.h \
../librssguard/services/standard/rdfparser.h \
../librssguard/services/standard/rssparser.h \
../librssguard/services/standard/standardcategory.h \
../librssguard/services/standard/standardfeed.h \
../librssguard/services/standard/standardfeedsimportexportmodel.h \
../librssguard/services/standard/standardserviceentrypoint.h \
../librssguard/services/standard/standardserviceroot.h \
../librssguard/services/tt-rss/definitions.h \
../librssguard/services/tt-rss/gui/formeditttrssaccount.h \
../librssguard/services/tt-rss/gui/formttrssfeeddetails.h \
../librssguard/services/tt-rss/network/ttrssnetworkfactory.h \
../librssguard/services/tt-rss/ttrssfeed.h \
../librssguard/services/tt-rss/ttrssserviceentrypoint.h \
../librssguard/services/tt-rss/ttrssserviceroot.h

# Install all files on Windows.
win32 {
  target.path = $$PREFIX
  
  lib.files = $$OUT_PWD/../librssguard/librssguard.dll $$OUT_PWD/../librssguard/librssguard.lib
  lib.path = $$PREFIX
  lib.CONFIG = no_check_exist

  clng.files = ../../resources/scripts/clang-format
  clng.path = $$PREFIX

  INSTALLS += target lib clng

  INSTALL_HEADERS_PREFIX = $$quote($$PREFIX/include/librssguard)
}

# Install all files on Linux.
unix:!mac:!android {
  target.path = $$PREFIX/bin

  desktop_file.files = ../../resources/desktop/$${APP_REVERSE_NAME}.desktop
  desktop_file.path = $$quote($$PREFIX/share/applications/)

  appdata.files = ../../resources/desktop/$${APP_REVERSE_NAME}.appdata.xml
  appdata.path = $$quote($$PREFIX/share/metainfo/)

  lib.files = $$OUT_PWD/../librssguard/librssguard.so
  lib.path = $$quote($$PREFIX/lib/)
  lib.CONFIG = no_check_exist

  desktop_icon.files = ../../resources/graphics/$${TARGET}.png
  desktop_icon.path = $$quote($$PREFIX/share/icons/hicolor/512x512/apps/)

  INSTALLS += target lib desktop_file desktop_icon appdata
  
  INSTALL_HEADERS_PREFIX = $$quote($$PREFIX/include/librssguard)
}

mac {
  IDENTIFIER = $$APP_REVERSE_NAME
  CONFIG -= app_bundle
  ICON = ../../resources/macosx/$${TARGET}.icns
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.10
  LIBS += -framework AppKit

  target.path = $$quote($$PREFIX/Contents/MacOS/)

  lib.files = $$OUT_PWD/../librssguard/librssguard.dylib
  lib.path = $$quote($$PREFIX/Contents/MacOS/)
  lib.CONFIG = no_check_exist

  # Install app icon.
  icns_icon.files = ../../resources/macosx/$${TARGET}.icns
  icns_icon.path = $$quote($$PREFIX/Contents/Resources/)

  # Install Info.plist.
  info_plist.files = ../../resources/macosx/Info.plist.in
  info_plist.path  = $$quote($$PREFIX/Contents/)

  # Process the just installed Info.plist.
  info_plist2.extra = @sed -e "s,@EXECUTABLE@,$$TARGET,g" -e "s,@SHORT_VERSION@,$$APP_VERSION,g" -e "s,@APP_NAME@,\"$$APP_NAME\",g" -e "s,@ICON@,$$basename(ICON),g"  -e "s,@TYPEINFO@,"????",g" $$shell_quote($$PREFIX/Contents/Info.plist.in) > $$shell_quote($$PREFIX/Contents/Info.plist) && \
                      rm -f $$shell_quote($$PREFIX/Contents/Info.plist.in)
  info_plist2.path = $$quote($$PREFIX/Contents/)

  # Install PkgInfo
  pkginfo.extra = @printf "APPL????" > $$shell_quote($$PREFIX/Contents/PkgInfo)
  pkginfo.path = $$quote($$PREFIX/Contents/)

  INSTALLS += target lib icns_icon info_plist info_plist2 pkginfo
  
  INSTALL_HEADERS_PREFIX = $$shell_quote($$PREFIX/Contents/Resources/Include/librssguard)
}

message($$MSG_PREFIX: Prefix for headers is \"$$INSTALL_HEADERS_PREFIX\".)

# Create install step for each folder of public headers.
for(header, INSTALL_HEADERS) {
  path = $${INSTALL_HEADERS_PREFIX}/$${dirname(header)}
  path = $$shell_quote($$path)

  message($$MSG_PREFIX: Adding header \"$$header\" to \"make install\" step with path \"$$path\".)

  eval(headers_$${dirname(header)}.files += $$header)
  eval(headers_$${dirname(header)}.path = $$path)
  eval(INSTALLS *= headers_$${dirname(header)})
}
