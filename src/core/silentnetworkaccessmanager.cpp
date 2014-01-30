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
  FeedsModelStandardFeed *feed = static_cast<FeedsModelStandardFeed*>(reply->request().originatingObject()->property("feed").value<void*>());

  // TODO: tady do autenticatoru dosadit udaje z feedu
  // pokud je obsahuje
  // a taky promyslet zda to delat takhle vubec, ale funguje
  // to
  /*
   *authenticator->setUser("rotter.martinos");
   *authenticator->setPassword("gorottin0151");
  */

  qDebug("Authentication problems for '%s'.",
         qPrintable(reply->url().toString()));
}
