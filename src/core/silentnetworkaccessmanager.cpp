#include "core/silentnetworkaccessmanager.h"

#include "core/feedsmodelstandardfeed.h"

#include <QNetworkReply>
#include <QAuthenticator>


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
  QObject *originating_object = reply->request().originatingObject();

  if (originating_object->property("protected").toBool()) {
    // This feed contains authentication information, it is good.
    authenticator->setUser(originating_object->property("username").toString());
    authenticator->setPassword(originating_object->property("password").toString());

    qDebug("Feed '%s' requested authentication and got it.",
           qPrintable(reply->url().toString()));
  }
  else {
    // Authentication is required but this feed does not contain it.
    qDebug("Feed '%s' requested authentication but username/password is not available.",
           qPrintable(reply->url().toString()));
  }
}
