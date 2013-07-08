#include <QNetworkProxy>

#include "core/settings.h"
#include "core/defs.h"
#include "core/basenetworkaccessmanager.h"


BaseNetworkAccessManager::BaseNetworkAccessManager(QObject *parent)
  : QNetworkAccessManager(parent) {
}

void BaseNetworkAccessManager::loadSettings() {
  QNetworkProxy new_proxy;
  // TODO: Continue here.
}
