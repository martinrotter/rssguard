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

BaseNetworkAccessManager::~BaseNetworkAccessManager() = default;

void BaseNetworkAccessManager::loadSettings() {
  QNetworkProxy new_proxy;
  const QNetworkProxy::ProxyType selected_proxy_type = static_cast<QNetworkProxy::ProxyType>(qApp->settings()->value(GROUP(Proxy),
                                                                                                                     SETTING(Proxy::Type)).
                                                                                             toInt());

  if (selected_proxy_type == QNetworkProxy::NoProxy) {
    // No extra setting is needed, set new proxy and exit this method.
    setProxy(QNetworkProxy::NoProxy);
  }
  else if (selected_proxy_type == QNetworkProxy::DefaultProxy) {
    setProxy(QNetworkProxy::applicationProxy());
  }
  else {
    const Settings* settings = qApp->settings();

    // Custom proxy is selected, set it up.
    new_proxy.setType(selected_proxy_type);
    new_proxy.setHostName(settings->value(GROUP(Proxy), SETTING(Proxy::Host)).toString());
    new_proxy.setPort(quint16(settings->value(GROUP(Proxy), SETTING(Proxy::Port)).toInt()));
    new_proxy.setUser(settings->value(GROUP(Proxy), SETTING(Proxy::Username)).toString());
    new_proxy.setPassword(settings->password(GROUP(Proxy), SETTING(Proxy::Password)).toString());
    setProxy(new_proxy);
  }

  qDebug("Settings of BaseNetworkAccessManager loaded.");
}

void BaseNetworkAccessManager::onSslErrors(QNetworkReply* reply, const QList<QSslError>& error) {
  qWarning("Ignoring SSL errors for '%s': '%s' (code %d).", qPrintable(reply->url().toString()), qPrintable(reply->errorString()),
           int(reply->error()));
  reply->ignoreSslErrors(error);
}

QNetworkReply* BaseNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                       const QNetworkRequest& request,
                                                       QIODevice* outgoingData) {
  QNetworkRequest new_request = request;

  // This rapidly speeds up loading of web sites.
  // NOTE: https://en.wikipedia.org/wiki/HTTP_pipelining
  new_request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

  // Setup custom user-agent.
  new_request.setRawHeader(HTTP_HEADERS_USER_AGENT, QString(APP_USERAGENT).toLocal8Bit());
  return QNetworkAccessManager::createRequest(op, new_request, outgoingData);
}
