#include "core/silentnetworkaccessmanager.h"

#include <QNetworkReply>


SilentNetworkAccessManager::SilentNetworkAccessManager(QObject *parent)
  : BaseNetworkAccessManager(parent) {
  connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
          this, SLOT(onSslErrors(QNetworkReply*,QList<QSslError>)));
  connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          this, SLOT(onAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
}

SilentNetworkAccessManager::~SilentNetworkAccessManager() {
  qDebug("Destroying SilentNetworkAccessManages instance.");
}

void SilentNetworkAccessManager::onSslErrors(QNetworkReply *reply,
                                           const QList<QSslError> &error) {
  qDebug("SSL errors for '%s': '%s' (code %d).",
         qPrintable(reply->url().toString()),
         qPrintable(reply->errorString()),
         (int) reply->error());

  reply->ignoreSslErrors(error);
}

void SilentNetworkAccessManager::onAuthenticationRequired(QNetworkReply *reply,
                                                          QAuthenticator *authenticator) {
  Q_UNUSED(authenticator)

  qDebug("Autorization problems for '%s'.",
         qPrintable(reply->url().toString()));
}
