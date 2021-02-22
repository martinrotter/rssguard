TEMPLATE = app
TARGET = rssguard

MSG_PREFIX = "rssguard"
APP_TYPE = "executable"

include(../../pri/vars.pri)

isEmpty(PREFIX) {
  message($$MSG_PREFIX: PREFIX variable is not set. This might indicate error.)

  win32|os2|android {
    PREFIX = $$OUT_PWD/app
  }

  mac {
    PREFIX = $$quote($$OUT_PWD/$${APP_NAME}.app)
  }

  unix:!mac:!android {
    PREFIX = $$OUT_PWD/AppDir/usr
  }
}

include(../../pri/defs.pri)

message($$MSG_PREFIX: Current directory \"$$PWD\".)
message($$MSG_PREFIX: Shadow copy build directory \"$$OUT_PWD\".)
message($$MSG_PREFIX: $$APP_NAME version is: \"$$APP_VERSION\".)
message($$MSG_PREFIX: Detected Qt version: \"$$QT_VERSION\".)
message($$MSG_PREFIX: Build destination directory: \"$$DESTDIR\".)
message($$MSG_PREFIX: Prefix directory: \"$$PREFIX\".)
message($$MSG_PREFIX: Build revision: \"$$APP_REVISION\".)

include(../../pri/build_opts.pri)

DEFINES *= RSSGUARD_DLLSPEC=Q_DECL_IMPORT
SOURCES += main.cpp
INCLUDEPATH +=  $$PWD/../librssguard \
                $$PWD/../librssguard/gui \
                $$OUT_PWD/../librssguard \
                $$OUT_PWD/../librssguard/ui

DEPENDPATH += $$PWD/../librssguard

win32: LIBS += -L$$OUT_PWD/../librssguard/ -llibrssguard
unix: LIBS += -L$$OUT_PWD/../librssguard/ -lrssguard
os2: LIBS += -L$$OUT_PWD/../librssguard/ -lrssguard

win32 {
  # Prepare files for NSIS.
  SEDREPLACE = "| ForEach-Object { $_ -replace '@APP_VERSION@', '$$APP_VERSION' -replace '@APP_REVISION@', '$$APP_REVISION' -replace '@APP_NAME@', '$$APP_NAME' -replace '@APP_LOW_NAME@', '$$APP_LOW_NAME' -replace '@EXE_NAME@', '$${APP_LOW_NAME}.exe' -replace '@PWD@', '$$replace(PWD, /, \\\\)' -replace '@OUT_PWD@', '$$replace(OUT_PWD, /, \\\\)' }"
  message($$MSG_PREFIX: Sed replace string: \"$$SEDREPLACE\")

  FULLSEDCMD = "powershell -Command \"cat $$shell_path($$shell_quote($$PWD/../../resources/nsis/NSIS.definitions.nsh.in)) $$SEDREPLACE | Out-File $$shell_path($$shell_quote($$OUT_PWD/NSIS.definitions.nsh))\""
  message($$MSG_PREFIX: Full powershell command: $$FULLSEDCMD)

  system(xcopy /Y $$shell_path($$shell_quote($$PWD/../../resources/nsis/NSIS.template.in)) $$shell_path($$shell_quote($$OUT_PWD/)))
  system($$FULLSEDCMD)
}

include(../../pri/install.pri)
