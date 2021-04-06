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

  if (selected_proxy_type == QNetworkProxy::ProxyType::NoProxy) {
    // No extra setting is needed, set new proxy and exit this method.
    setProxy(QNetworkProxy::ProxyType::NoProxy);
  }
  else {
    setProxy(QNetworkProxy::applicationProxy());
  }

  qDebugNN << LOGSEC_NETWORK << "Settings of BaseNetworkAccessManager loaded.";
}

void BaseNetworkAccessManager::acceptRedirection(const QUrl& url) {
  auto* reply = qobject_cast<QNetworkReply*>(sender());
  emit reply->redirectAllowed();

  qDebugNN << LOGSEC_NETWORK << "Accepting redirect to" << QUOTE_W_SPACE_DOT(url.toString());
}

void BaseNetworkAccessManager::onSslErrors(QNetworkReply* reply, const QList<QSslError>& error) {
  qWarningNN << LOGSEC_NETWORK
             << "Ignoring SSL errors for"
             << QUOTE_W_SPACE(reply->url().toString())
             << ":"
             << QUOTE_W_SPACE(reply->errorString())
             << "- code"
             << QUOTE_W_SPACE_DOT(reply->error());
  reply->ignoreSslErrors(error);
}

QNetworkReply* BaseNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                       const QNetworkRequest& request,
                                                       QIODevice* outgoingData) {
  QNetworkRequest new_request = request;

#if defined(Q_OS_WIN)
  new_request.setAttribute(QNetworkRequest::Attribute::HttpPipeliningAllowedAttribute, true);
  new_request.setAttribute(QNetworkRequest::Attribute::Http2AllowedAttribute, true);
#endif

#if QT_VERSION >= 0x050900
  new_request.setAttribute(QNetworkRequest::Attribute::RedirectPolicyAttribute,
                           QNetworkRequest::RedirectPolicy::UserVerifiedRedirectPolicy);
#elif QT_VERSION >= 0x050600
  new_request.setAttribute(QNetworkRequest::Attribute::FollowRedirectsAttribute, true);
#endif

  new_request.setRawHeader(HTTP_HEADERS_COOKIE, QSL("JSESSIONID= ").toLocal8Bit());
  new_request.setRawHeader(HTTP_HEADERS_USER_AGENT, QString(APP_USERAGENT).toLocal8Bit());

  auto reply = QNetworkAccessManager::createRequest(op, new_request, outgoingData);
  connect(reply, &QNetworkReply::redirected, this, &BaseNetworkAccessManager::acceptRedirection);
  return reply;
}
