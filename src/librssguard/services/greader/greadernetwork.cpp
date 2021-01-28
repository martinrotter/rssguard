// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/greadernetwork.h"

#include "miscellaneous/application.h"
#include "network-web/networkfactory.h"

GreaderNetwork::GreaderNetwork(QObject* parent)
  : QObject(parent), m_service(GreaderServiceRoot::Service::FreshRss) {}

QList<Message> GreaderNetwork::messages(ServiceRoot* root, const QString& stream_id, Feed::Status& error) {
  return {};
}

QNetworkReply::NetworkError GreaderNetwork::clientLogin(const QNetworkProxy& proxy) {
  QString full_url = generateFullUrl(Operations::ClientLogin);
  auto timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;
  auto network_result = NetworkFactory::performNetworkOperation(full_url,
                                                                timeout,
                                                                {},
                                                                output,
                                                                QNetworkAccessManager::Operation::GetOperation,
                                                                {},
                                                                false,
                                                                {},
                                                                {},
                                                                proxy);

  if (network_result.first == QNetworkReply::NetworkError::NoError) {
    // Save credentials.
    auto lines = QString::fromUtf8(output).replace(QSL("\r"), QString()).split('\n');
    int a = 5;
  }

  return network_result.first;
}

GreaderServiceRoot::Service GreaderNetwork::service() const {
  return m_service;
}

void GreaderNetwork::setService(const GreaderServiceRoot::Service& service) {
  m_service = service;
}

QString GreaderNetwork::username() const {
  return m_username;
}

void GreaderNetwork::setUsername(const QString& username) {
  m_username = username;
}

QString GreaderNetwork::password() const {
  return m_password;
}

void GreaderNetwork::setPassword(const QString& password) {
  m_password = password;
}

QString GreaderNetwork::baseUrl() const {
  return m_baseUrl;
}

void GreaderNetwork::setBaseUrl(const QString& base_url) {
  m_baseUrl = base_url;
}

QString GreaderNetwork::serviceToString(GreaderServiceRoot::Service service) {
  switch (service) {
    case GreaderServiceRoot::Service::FreshRss:
      return QSL("FreshRSS");

    case GreaderServiceRoot::Service::Bazqux:
      return QSL("Bazqux");

    case GreaderServiceRoot::Service::TheOldReader:
      return QSL("TheOldReader");

    default:
      return tr("Unknown service");
  }
}

int GreaderNetwork::batchSize() const {
  return m_batchSize;
}

void GreaderNetwork::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

QString GreaderNetwork::sanitizedBaseUrl() const {
  if (m_baseUrl.endsWith('/')) {
    return m_baseUrl;
  }
  else {
    return m_baseUrl + QL1C('/');
  }
}

QString GreaderNetwork::generateFullUrl(GreaderNetwork::Operations operation) const {
  switch (operation) {
    case Operations::ClientLogin:
      return sanitizedBaseUrl() + QSL("accounts/ClientLogin?Email=%1&Passwd=%2").arg(username(), password());
  }
}
