#include "core/webbrowsernetworkaccessmanager.h"
#include "gui/basewebpage.h"
#include "gui/webbrowser.h"


BaseWebPage::BaseWebPage(QObject *parent) : QWebPage(parent) {
  setNetworkAccessManager(WebBrowser::globalNetworkManager());
}
