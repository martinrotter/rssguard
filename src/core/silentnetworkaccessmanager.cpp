#include "core/silentnetworkaccessmanager.h"

#include <QNetworkReply>


SilentNetworkAccessManager::SilentNetworkAccessManager(QObject *parent)
  : BaseNetworkAccessManager(parent) {

}

SilentNetworkAccessManager::~SilentNetworkAccessManager() {
  qDebug("Destroying SilentNetworkAccessManages instance.");
}

void SilentNetworkAccessManager::onSslErrors(QNetworkReply *reply,
                                           const QList<QSslError> &error) {
  qDebug("SSL errors for '%s'.",
         qPrintable(reply->url().toString()));

  reply->ignoreSslErrors(error);
}

void SilentNetworkAccessManager::onAuthenticationRequired(QNetworkReply *reply,
                                                          QAuthenticator *authenticator) {
  Q_UNUSED(authenticator)

  qDebug("Authentification problems for '%s'.",
         qPrintable(reply->url().toString()));
}
