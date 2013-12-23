#include "core/webpage.h"

#include "core/webbrowsernetworkaccessmanager.h"
#include "gui/webbrowser.h"

#include <QNetworkReply>
#include <QWebFrame>


WebPage::WebPage(QObject *parent)
  : QWebPage(parent) {
  // Setup global network access manager.
  // NOTE: This makes network settings easy for all web browsers.
  setNetworkAccessManager(WebBrowser::globalNetworkManager());
}

WebPage::~WebPage() {
}

QWebPage *WebPage::createWindow(WebWindowType type) {
  return QWebPage::createWindow(type);
}
