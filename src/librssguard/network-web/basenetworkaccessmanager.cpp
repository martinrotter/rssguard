// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/basenetworkaccessmanager.h"

#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"

#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>

BaseNetworkAccessManager::BaseNetworkAccessManager(QObject* parent)
  : QNetworkAccessManager(parent) {
  connect(this, &BaseNetworkAccessManager::sslErrors, this, &BaseNetworkAccessManager::onSslErrors);
  loadSettings();
}

void BaseNetworkAccessManager::loadSettings() {
  QNetworkProxy new_proxy;
  const QNetworkProxy::ProxyType selected_proxy_type = static_cast<QNetworkProxy::ProxyType>(qApp->settings()->value(GROUP(Proxy),
                                                                                                                     SETTING(Proxy::Type)).
                                                                                             toInt());

  if (selected_proxy_type == QNetworkProxy::NoProxy) {
    // No extra setting is needed, set new proxy and exit this method.
    setProxy(QNetworkProxy::NoProxy);
  }
  else {
    setProxy(QNetworkProxy::applicationProxy());
  }

  qDebugNN << LOGSEC_NETWORK << "Settings of BaseNetworkAccessManager loaded.";
}

void BaseNetworkAccessManager::onSslErrors(QNetworkReply* reply, const QList<QSslError>& error) {
  qWarningNN << LOGSEC_NETWORK
             << "Ignoring SSL errors for '"
             << reply->url().toString()
             << "':"
             << QUOTE_W_SPACE(reply->errorString())
             << "(code "
             << reply->error()
             << ").";
  reply->ignoreSslErrors(error);
}

QNetworkReply* BaseNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                       const QNetworkRequest& request,
                                                       QIODevice* outgoingData) {
  QNetworkRequest new_request = request;

  // This rapidly speeds up loading of web sites.
  // NOTE: https://en.wikipedia.org/wiki/HTTP_pipelining
  new_request.setAttribute(QNetworkRequest::Attribute::HttpPipeliningAllowedAttribute, true);

  // Setup custom user-agent.
  new_request.setRawHeader(HTTP_HEADERS_USER_AGENT, QString(APP_USERAGENT).toLocal8Bit());
  return QNetworkAccessManager::createRequest(op, new_request, outgoingData);
}
