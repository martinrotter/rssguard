#include <QNetworkReply>
#include <QWebFrame>

#include "core/webbrowsernetworkaccessmanager.h"
#include "core/basewebpage.h"
#include "gui/webbrowser.h"


BaseWebPage::BaseWebPage(QObject *parent)
  : QWebPage(parent) {
  // Setup global network access manager.
  // NOTE: This makes network settings easy for all web browsers.
  setNetworkAccessManager(WebBrowser::globalNetworkManager());
}

BaseWebPage::~BaseWebPage() {
}

QWebPage *BaseWebPage::createWindow(WebWindowType type) {
  return QWebPage::createWindow(type);
}
