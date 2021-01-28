// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/greadernetwork.h"

GreaderNetwork::GreaderNetwork(QObject* parent)
  : QObject(parent), m_service(GreaderServiceRoot::Service::FreshRss) {}

QList<Message> GreaderNetwork::messages(ServiceRoot* root, const QString& stream_id, Feed::Status& error) {
  return {};
}

NetworkResult GreaderNetwork::status(const QNetworkProxy& custom_proxy) const {
  return NetworkResult(QNetworkReply::NetworkError::NoError, {});
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
