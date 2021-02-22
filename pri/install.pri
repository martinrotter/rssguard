# Setup all public headers, this needs to be kept in
# sync with truly used headers.
INSTALL_HEADERS = \
../librssguard/services/abstract/accountcheckmodel.h \
../librssguard/services/abstract/cacheforserviceroot.h \
../librssguard/services/abstract/category.h \
../librssguard/services/abstract/feed.h \
../librssguard/services/abstract/gui/authenticationdetails.h \
../librssguard/services/abstract/gui/formfeeddetails.h \
../librssguard/services/abstract/importantnode.h \
../librssguard/services/abstract/label.h \
../librssguard/services/abstract/labelsnode.h \
../librssguard/services/abstract/recyclebin.h \
../librssguard/services/abstract/rootitem.h \
../librssguard/services/abstract/serviceentrypoint.h \
../librssguard/services/abstract/serviceroot.h

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

# Install all files on OS/2.
os2 {
  target.path = $$PREFIX

  lib.files = $$OUT_PWD/../librssguard/rssguard.dll $$OUT_PWD/../librssguard/rssguard.lib
  lib.path = $$PREFIX
  lib.CONFIG = no_check_exist

  INSTALLS += target lib

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
