# Custom definitions.
DEFINES *= APP_VERSION='"\\\"$$APP_VERSION\\\""'
DEFINES *= APP_NAME='"\\\"$$APP_NAME\\\""'
DEFINES *= APP_LOW_NAME='"\\\"$$APP_LOW_NAME\\\""'
DEFINES *= APP_LOW_H_NAME='"\\\"$$APP_LOW_H_NAME\\\""'
DEFINES *= APP_LONG_NAME='"\\\"$$APP_LONG_NAME\\\""'
DEFINES *= APP_AUTHOR='"\\\"$$APP_AUTHOR\\\""'
DEFINES *= APP_EMAIL='"\\\"$$APP_EMAIL\\\""'
DEFINES *= APP_URL='"\\\"$$APP_URL\\\""'
DEFINES *= APP_URL_ISSUES='"\\\"$$APP_URL_ISSUES\\\""'
DEFINES *= APP_URL_ISSUES_NEW='"\\\"$$APP_URL_ISSUES_NEW\\\""'
DEFINES *= APP_URL_DOCUMENTATION='"\\\"$$APP_URL_DOCUMENTATION\\\""'
DEFINES *= APP_USERAGENT='"\\\"$$APP_USERAGENT\\\""'
DEFINES *= APP_DONATE_URL='"\\\"$$APP_DONATE_URL\\\""'
DEFINES *= APP_SYSTEM_NAME='"\\\"$$QMAKE_HOST.os\\\""'
DEFINES *= APP_SYSTEM_VERSION='"\\\"$$QMAKE_HOST.arch\\\""'

DISTFILES += ../../resources/scripts/uncrustify/uncrustify.cfg

CODECFORTR  = UTF-8
CODECFORSRC = UTF-8

exists($PWD/../../.git) {
  APP_REVISION = $$system(git rev-parse --short HEAD)
}

isEmpty(APP_REVISION) {
  APP_REVISION = ""
}

equals(USE_WEBENGINE, false) {
  # Add extra revision naming when building without webengine.
  APP_REVISION = $$sprintf('%1-%2', $$APP_REVISION, nowebengine)
}

DEFINES *= APP_REVISION='"\\\"$$APP_REVISION\\\""'
