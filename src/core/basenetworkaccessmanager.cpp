#include <QNetworkProxy>

#include "core/settings.h"
#include "core/defs.h"
#include "core/basenetworkaccessmanager.h"


BaseNetworkAccessManager::BaseNetworkAccessManager(QObject *parent)
  : QNetworkAccessManager(parent) {
  loadSettings();
}

BaseNetworkAccessManager::~BaseNetworkAccessManager() {
  qDebug("Destroying BaseNetworkAccessManager instance.");
}

void BaseNetworkAccessManager::loadSettings() {
  QNetworkProxy new_proxy;

  // Load proxy values from settings.
  QNetworkProxy::ProxyType selected_proxy_type = static_cast<QNetworkProxy::ProxyType>(Settings::getInstance()->value(APP_CFG_PROXY,
                                                                                                                      "proxy_type",
                                                                                                                      QNetworkProxy::NoProxy).toInt());
  if (selected_proxy_type == QNetworkProxy::NoProxy) {
    // No extra setting is needed, set new proxy and exit this method.
    setProxy(QNetworkProxy::NoProxy);
    return;
  }

  // Custom proxy is selected, set it up.
  new_proxy.setType(selected_proxy_type);
  new_proxy.setHostName(Settings::getInstance()->value(APP_CFG_PROXY,
                                                       "host").toString());
  new_proxy.setPort(Settings::getInstance()->value(APP_CFG_PROXY,
                                                   "port", 80).toInt());
  new_proxy.setUser(Settings::getInstance()->value(APP_CFG_PROXY,
                                                   "username").toString());
  new_proxy.setPassword(Settings::getInstance()->value(APP_CFG_PROXY,
                                                       "password").toString());
  setProxy(new_proxy);

  qDebug("Settings of BaseNetworkAccessManager changed.");
}

QNetworkReply *BaseNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                       const QNetworkRequest &request,
                                                       QIODevice *outgoingData) {
  return QNetworkAccessManager::createRequest(op,
                                              request,
                                              outgoingData);
}
