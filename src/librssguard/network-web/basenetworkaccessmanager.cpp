// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/basenetworkaccessmanager.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "network-web/webfactory.h"

#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>

#if defined(NO_LITE)
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#endif

BaseNetworkAccessManager::BaseNetworkAccessManager(QObject* parent)
  : QNetworkAccessManager(parent), m_enableHttp2(false) {
  connect(this, &BaseNetworkAccessManager::sslErrors, this, &BaseNetworkAccessManager::onSslErrors);
  loadSettings();
}

void BaseNetworkAccessManager::loadSettings() {
  const QNetworkProxy::ProxyType selected_proxy_type =
    static_cast<QNetworkProxy::ProxyType>(qApp->settings()->value(GROUP(Proxy), SETTING(Proxy::Type)).toInt());

  if (selected_proxy_type == QNetworkProxy::ProxyType::NoProxy) {
    // No extra setting is needed, set new proxy and exit this method.
    setProxy(QNetworkProxy::ProxyType::NoProxy);
  }
  else {
    qWarningNN << LOGSEC_NETWORK << "Using application-wide proxy.";

    if (QNetworkProxy::applicationProxy().type() != QNetworkProxy::ProxyType::DefaultProxy &&
        QNetworkProxy::applicationProxy().type() != QNetworkProxy::ProxyType::NoProxy) {
      qWarningNN << LOGSEC_NETWORK
                 << "Used proxy address:" << QUOTE_W_SPACE_COMMA(QNetworkProxy::applicationProxy().hostName())
                 << " type:" << QUOTE_W_SPACE_DOT(QNetworkProxy::applicationProxy().type());
    }

    setProxy(QNetworkProxy::applicationProxy());
  }

  m_enableHttp2 = qApp->settings()->value(GROUP(Network), SETTING(Network::EnableHttp2)).toBool();

  qDebugNN << LOGSEC_NETWORK << "Settings of BaseNetworkAccessManager loaded.";
}

void BaseNetworkAccessManager::onSslErrors(QNetworkReply* reply, const QList<QSslError>& error) {
  qWarningNN << LOGSEC_NETWORK << "Ignoring SSL errors for" << QUOTE_W_SPACE_DOT(reply->url().toString());
  reply->ignoreSslErrors(error);
}

QNetworkReply* BaseNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                       const QNetworkRequest& request,
                                                       QIODevice* outgoingData) {
  QNetworkRequest new_request = request;

  new_request.setAttribute(QNetworkRequest::Attribute::RedirectPolicyAttribute,
                           QNetworkRequest::RedirectPolicy::ManualRedirectPolicy);

#if defined(Q_OS_WIN)
  new_request.setAttribute(QNetworkRequest::Attribute::HttpPipeliningAllowedAttribute, true);
#endif

  new_request.setAttribute(QNetworkRequest::Attribute::Http2AllowedAttribute, m_enableHttp2);

  // new_request.setMaximumRedirectsAllowed(0);

  new_request.setRawHeader(HTTP_HEADERS_COOKIE, QSL("JSESSIONID= ").toLocal8Bit());

  auto custom_ua = qApp->web()->customUserAgent();

  if (custom_ua.isEmpty()) {
    new_request.setRawHeader(HTTP_HEADERS_USER_AGENT, HTTP_COMPLETE_USERAGENT);
  }
  else {
    new_request.setRawHeader(HTTP_HEADERS_USER_AGENT, custom_ua.toLocal8Bit());
  }

  auto reply = QNetworkAccessManager::createRequest(op, new_request, outgoingData);

  auto ssl_conf = reply->sslConfiguration();

  auto aa = ssl_conf.backendConfiguration();

  ssl_conf.setPeerVerifyMode(QSslSocket::PeerVerifyMode::VerifyNone);
  ssl_conf.setSslOption(QSsl::SslOption::SslOptionDisableLegacyRenegotiation, false);

  reply->setSslConfiguration(ssl_conf);

  return reply;
}
