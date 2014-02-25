#include "core/webbrowsernetworkaccessmanager.h"

#include <QNetworkReply>
#include <QApplication>


QPointer<WebBrowserNetworkAccessManager> WebBrowserNetworkAccessManager::s_instance;

WebBrowserNetworkAccessManager::WebBrowserNetworkAccessManager(QObject *parent)
  : BaseNetworkAccessManager(parent) {
  connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          this, SLOT(onAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
}

WebBrowserNetworkAccessManager::~WebBrowserNetworkAccessManager() {
  qDebug("Destroying WebBrowserNetworkAccessManager instance.");
}

void WebBrowserNetworkAccessManager::onAuthenticationRequired(QNetworkReply *reply,
                                                              QAuthenticator *authenticator) {
  Q_UNUSED(authenticator);

  // TODO: Support authentication for web pages.
  qDebug("URL '%s' requested authentication but username/password is not available.",
         qPrintable(reply->url().toString()));
}

WebBrowserNetworkAccessManager *WebBrowserNetworkAccessManager::instance() {
  if (s_instance.isNull()) {
    s_instance = new WebBrowserNetworkAccessManager(qApp);
  }

  return s_instance;
}
