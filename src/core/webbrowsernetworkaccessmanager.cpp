#include "core/webbrowsernetworkaccessmanager.h"


WebBrowserNetworkAccessManager::WebBrowserNetworkAccessManager(QObject *parent)
  : BaseNetworkAccessManager(parent) {
}

WebBrowserNetworkAccessManager::~WebBrowserNetworkAccessManager() {
  qDebug("Destroying WebBrowserNetworkAccessManager instance.");
}
