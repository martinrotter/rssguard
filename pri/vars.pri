APP_NAME                      = "RSS Guard"
APP_LOW_NAME                  = "rssguard"
APP_REVERSE_NAME              = "com.github.rssguard"
APP_LOW_H_NAME                = ".rssguard"
APP_AUTHOR                    = "Martin Rotter"
APP_COPYRIGHT                 = "(C) 2011-2021 $$APP_AUTHOR"
APP_VERSION                   = "3.9.2"
APP_LONG_NAME                 = "$$APP_NAME $$APP_VERSION"
APP_EMAIL                     = "rotter.martinos@gmail.com"
APP_URL                       = "https://github.com/martinrotter/rssguard"
APP_URL_ISSUES                = "https://github.com/martinrotter/rssguard/issues"
APP_URL_ISSUES_NEW            = "https://github.com/martinrotter/rssguard/issues/new"
APP_URL_DOCUMENTATION         = "https://github.com/martinrotter/rssguard/blob/master/resources/docs/Documentation.md"
APP_USERAGENT                 = "RSS Guard/$$APP_VERSION (github.com/martinrotter/rssguard)"
APP_DONATE_URL                = "https://martinrotter.github.io/donate"

message(rssguard: Welcome RSS Guard qmake script.)

lessThan(QT_MAJOR_VERSION, 5)|lessThan(QT_MINOR_VERSION, 9) {
  warning(rssguard: At least Qt \"5.9.0\" is required!!!)
}

isEmpty(USE_WEBENGINE) {
  USE_WEBENGINE = false
  message($$MSG_PREFIX: USE_WEBENGINE variable is not set.)

  qtHaveModule(webenginewidgets) {
    USE_WEBENGINE = true
    ##message($$MSG_PREFIX: WebEngine component IS installed, enabling it.)
  }
  else {
    USE_WEBENGINE = false
    ##message($$MSG_PREFIX: WebEngine component is probably NOT installed, disabling it.)
  }
}

isEmpty(FEEDLY_CLIENT_ID)|isEmpty(FEEDLY_CLIENT_SECRET) {
  FEEDLY_OFFICIAL_SUPPORT = false

  message($$MSG_PREFIX: Feedly client ID/secret variables are not set. Disabling official support.)
}
else {
  FEEDLY_OFFICIAL_SUPPORT = true
  DEFINES *= FEEDLY_OFFICIAL_SUPPORT
  DEFINES *= FEEDLY_CLIENT_ID='"\\\"$$FEEDLY_CLIENT_ID\\\""'
  DEFINES *= FEEDLY_CLIENT_SECRET='"\\\"$$FEEDLY_CLIENT_SECRET\\\""'

  message($$MSG_PREFIX: Enabling official Feedly support.)
}
