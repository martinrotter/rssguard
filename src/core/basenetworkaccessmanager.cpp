#include <QNetworkProxy>

#include "core/settings.h"
#include "core/defs.h"
#include "core/basenetworkaccessmanager.h"


BaseNetworkAccessManager::BaseNetworkAccessManager(QObject *parent)
  : QNetworkAccessManager(parent) {
  loadSettings();
}

void BaseNetworkAccessManager::loadSettings() {
  QNetworkProxy new_proxy;
  // TODO: Continue here.
  setProxy(new_proxy);
}

QNetworkReply *BaseNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                       const QNetworkRequest &request,
                                                       QIODevice *outgoingData) {
  return QNetworkAccessManager::createRequest(op,
                                              request,
                                              outgoingData);
}
