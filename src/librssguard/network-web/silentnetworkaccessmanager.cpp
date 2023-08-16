// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/silentnetworkaccessmanager.h"

#include "definitions/definitions.h"

#include <QAuthenticator>
#include <QNetworkReply>

SilentNetworkAccessManager::SilentNetworkAccessManager(QObject* parent) : BaseNetworkAccessManager(parent) {
  connect(this,
          &SilentNetworkAccessManager::authenticationRequired,
          this,
          &SilentNetworkAccessManager::onAuthenticationRequired,
          Qt::DirectConnection);
}

SilentNetworkAccessManager::~SilentNetworkAccessManager() {
  qDebugNN << LOGSEC_NETWORK << "Destroying SilentNetworkAccessManager instance.";
}

void SilentNetworkAccessManager::onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator) {
  if (reply->property("protected").toBool()) {
    // This feed contains authentication information, it is good.
    authenticator->setUser(reply->property("username").toString());
    authenticator->setPassword(reply->property("password").toString());
    reply->setProperty("authentication-given", true);
    qDebugNN << LOGSEC_NETWORK << "URL" << QUOTE_W_SPACE(reply->url().toString())
             << "requested authentication and got it.";
  }
  else {
    reply->setProperty("authentication-given", false);

    // Authentication is required but this item does not contain it.
    qWarningNN << LOGSEC_NETWORK << "Item" << QUOTE_W_SPACE(reply->url().toString())
               << "requested authentication but username/password is not available.";
  }
}
