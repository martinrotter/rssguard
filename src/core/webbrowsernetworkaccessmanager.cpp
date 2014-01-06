#include "core/webbrowsernetworkaccessmanager.h"

#include <QNetworkReply>


WebBrowserNetworkAccessManager::WebBrowserNetworkAccessManager(QObject *parent)
  : BaseNetworkAccessManager(parent) {
  connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
          this, SLOT(onSslErrors(QNetworkReply*,QList<QSslError>)));
}

WebBrowserNetworkAccessManager::~WebBrowserNetworkAccessManager() {
  qDebug("Destroying WebBrowserNetworkAccessManager instance.");
}

void WebBrowserNetworkAccessManager::onSslErrors(QNetworkReply *reply,
                                                 const QList<QSslError> &error) {
  qDebug("SSL errors for '%s': '%s' (code %d).",
         qPrintable(reply->url().toString()),
         qPrintable(reply->errorString()),
         (int) reply->error());

  reply->ignoreSslErrors(error);
}
