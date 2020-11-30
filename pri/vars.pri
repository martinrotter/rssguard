APP_NAME                      = "RSS Guard"
APP_LOW_NAME                  = "rssguard"
APP_REVERSE_NAME              = "com.github.rssguard"
APP_LOW_H_NAME                = ".rssguard"
APP_AUTHOR                    = "Martin Rotter"
APP_COPYRIGHT                 = "(C) 2011-2020 $$APP_AUTHOR"
APP_VERSION                   = "3.8.3"
APP_LONG_NAME                 = "$$APP_NAME $$APP_VERSION"
APP_EMAIL                     = "rotter.martinos@gmail.com"
APP_URL                       = "https://github.com/martinrotter/rssguard"
APP_URL_ISSUES                = "https://github.com/martinrotter/rssguard/issues"
APP_URL_ISSUES_NEW            = "https://github.com/martinrotter/rssguard/issues/new"
APP_URL_DOCUMENTATION         = "https://github.com/martinrotter/rssguard/blob/master/resources/docs/Documentation.md"
APP_USERAGENT                 = "RSS Guard/$$APP_VERSION (github.com/martinrotter/rssguard)"
APP_DONATE_URL                = "https://martinrotter.github.io/donate"
APP_WIN_ARCH                  = "win64"

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
