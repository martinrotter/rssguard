// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/ttrssnetworkfactory.h"

#include "src/definitions.h"
#include "src/ttrssfeed.h"

#include <librssguard/3rd-party/boolinq/boolinq.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/feedfetchexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/network-web/networkfactory.h>
#include <librssguard/services/abstract/category.h>
#include <librssguard/services/abstract/label.h>
#include <librssguard/services/abstract/labelsnode.h>
#include <librssguard/services/abstract/rootitem.h>
#include <librssguard/services/abstract/serviceroot.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QPair>
#include <QVariant>

TtRssNetworkFactory::TtRssNetworkFactory()
  : m_bareUrl(QString()), m_fullUrl(QString()), m_username(QString()), m_password(QString()),
    m_batchSize(TTRSS_DEFAULT_MESSAGES), m_forceServerSideUpdate(false), m_intelligentSynchronization(false),
    m_authIsUsed(false), m_authUsername(QString()), m_authPassword(QString()), m_sessionId(QString()),
    m_lastError(QNetworkReply::NetworkError::NoError) {}

QString TtRssNetworkFactory::url() const {
  return m_bareUrl;
}

void TtRssNetworkFactory::setUrl(const QString& url) {
  m_bareUrl = url;

  if (!m_bareUrl.endsWith(QSL("/"))) {
    m_bareUrl = m_bareUrl + QSL("/");
  }

  if (!m_bareUrl.endsWith(QSL("api/"))) {
    m_fullUrl = m_bareUrl + QSL("api/");
  }
  else {
    m_fullUrl = m_bareUrl;
  }
}

QString TtRssNetworkFactory::username() const {
  return m_username;
}

void TtRssNetworkFactory::setUsername(const QString& username) {
  m_username = username;
}

QString TtRssNetworkFactory::password() const {
  return m_password;
}

void TtRssNetworkFactory::setPassword(const QString& password) {
  m_password = password;
}

QDateTime TtRssNetworkFactory::lastLoginTime() const {
  return m_lastLoginTime;
}

QNetworkReply::NetworkError TtRssNetworkFactory::lastError() const {
  return m_lastError;
}

TtRssLoginResponse TtRssNetworkFactory::login(const QNetworkProxy& proxy) {
  if (!m_sessionId.isEmpty()) {
    qWarningNN << LOGSEC_TTRSS << "Session ID is not empty before login, logging out first.";
    logout(proxy);
  }

  QJsonObject json;

  json[QSL("op")] = QSL("login");
  json[QSL("user")] = m_username;
  json[QSL("password")] = m_password;

  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_fullUrl,
                                            qApp->settings()
                                              ->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout))
                                              .toInt(),
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            proxy);
  TtRssLoginResponse login_response(QString::fromUtf8(result_raw));

  if (network_reply.m_networkError == QNetworkReply::NoError) {
    m_sessionId = login_response.sessionId();
    m_lastLoginTime = QDateTime::currentDateTime();
  }
  else {
    qWarningNN << LOGSEC_TTRSS << "Login failed with error:" << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  m_lastError = network_reply.m_networkError;
  return login_response;
}

TtRssResponse TtRssNetworkFactory::logout(const QNetworkProxy& proxy) {
  if (!m_sessionId.isEmpty()) {
    QJsonObject json;

    json[QSL("op")] = QSL("logout");
    json[QSL("sid")] = m_sessionId;
    QByteArray result_raw;
    QList<QPair<QByteArray, QByteArray>> headers;

    headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
    headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                       m_authUsername,
                                                       m_authPassword);

    NetworkResult network_reply =
      NetworkFactory::performNetworkOperation(m_fullUrl,
                                              qApp->settings()
                                                ->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout))
                                                .toInt(),
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              result_raw,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              proxy);

    m_lastError = network_reply.m_networkError;

    if (m_lastError == QNetworkReply::NetworkError::NoError) {
      m_sessionId.clear();
    }
    else {
      qWarningNN << LOGSEC_TTRSS << "Logout failed with error:" << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
    }

    return TtRssResponse(QString::fromUtf8(result_raw));
  }
  else {
    qWarningNN << LOGSEC_TTRSS << "Cannot logout because session ID is empty.";
    m_lastError = QNetworkReply::NetworkError::NoError;
    return TtRssResponse();
  }
}

TtRssGetLabelsResponse TtRssNetworkFactory::getLabels(const QNetworkProxy& proxy) {
  QJsonObject json;

  json[QSL("op")] = QSL("getLabels");
  json[QSL("sid")] = m_sessionId;

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_fullUrl,
                                            timeout,
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            proxy);
  TtRssGetLabelsResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login(proxy);
    json[QSL("sid")] = m_sessionId;
    network_reply =
      NetworkFactory::performNetworkOperation(m_fullUrl,
                                              timeout,
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              result_raw,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              proxy);
    result = TtRssGetLabelsResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS << "getLabels failed with error:" << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  m_lastError = network_reply.m_networkError;
  return result;
}

TtRssResponse TtRssNetworkFactory::shareToPublished(const TtRssNoteToPublish& note, const QNetworkProxy& proxy) {
  QJsonObject json;

  json[QSL("op")] = QSL("shareToPublished");
  json[QSL("sid")] = m_sessionId;
  json[QSL("title")] = note.m_title;
  json[QSL("url")] = note.m_url;
  json[QSL("content")] = note.m_content;

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_fullUrl,
                                            timeout,
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            proxy);
  TtRssResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login(proxy);
    json[QSL("sid")] = m_sessionId;
    network_reply =
      NetworkFactory::performNetworkOperation(m_fullUrl,
                                              timeout,
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              result_raw,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              proxy);
    result = TtRssResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS
               << "shareToPublished failed with error:" << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  m_lastError = network_reply.m_networkError;
  return result;
}

TtRssGetFeedsCategoriesResponse TtRssNetworkFactory::getFeedsCategories(const QNetworkProxy& proxy) {
  QJsonObject json;

  json[QSL("op")] = QSL("getFeedTree");
  json[QSL("sid")] = m_sessionId;
  json[QSL("include_empty")] = true;
  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_fullUrl,
                                            timeout,
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            proxy);
  TtRssGetFeedsCategoriesResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login(proxy);
    json[QSL("sid")] = m_sessionId;
    network_reply =
      NetworkFactory::performNetworkOperation(m_fullUrl,
                                              timeout,
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              result_raw,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              proxy);
    result = TtRssGetFeedsCategoriesResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS << "getFeedTree failed with error:" << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  m_lastError = network_reply.m_networkError;
  return result;
}

TtRssGetCompactHeadlinesResponse TtRssNetworkFactory::getCompactHeadlines(int feed_id,
                                                                          int limit,
                                                                          int skip,
                                                                          const QString& view_mode,
                                                                          const QNetworkProxy& proxy) {
  QJsonObject json;

  json[QSL("op")] = QSL("getCompactHeadlines");
  json[QSL("sid")] = m_sessionId;
  json[QSL("feed_id")] = feed_id;
  json[QSL("limit")] = limit;
  // json[QSL("skip")] = skip;
  json[QSL("view_mode")] = view_mode;

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_fullUrl,
                                            timeout,
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            proxy);
  TtRssGetCompactHeadlinesResponse result(QString::fromUtf8(result_raw));

  if (result.isUnknownMethod()) {
    qCriticalNN << LOGSEC_TTRSS << "'getCompactHeadlines' method is not installed.";

    throw FeedFetchException(Feed::Status::OtherError,
                             QSL("'getCompactHeadlines' method is not installed on your TT-RSS instance. Install "
                                 "'api_newsplus' plugin."));
  }
  else if (result.isNotLoggedIn()) {
    // We are not logged in.
    login(proxy);
    json[QSL("sid")] = m_sessionId;
    network_reply =
      NetworkFactory::performNetworkOperation(m_fullUrl,
                                              timeout,
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              result_raw,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              proxy);
    result = TtRssGetCompactHeadlinesResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.m_networkError != QNetworkReply::NetworkError::NoError) {
    qWarningNN << LOGSEC_TTRSS
               << "getCompactHeadlines failed with error:" << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  m_lastError = network_reply.m_networkError;

  return result;
}

TtRssGetHeadlinesResponse TtRssNetworkFactory::getArticle(const QStringList& article_ids, const QNetworkProxy& proxy) {
  QJsonObject json;

  json[QSL("op")] = QSL("getArticle");
  json[QSL("sid")] = m_sessionId;
  json[QSL("article_id")] = article_ids.join(QL1C(','));

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_fullUrl,
                                            timeout,
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            proxy);
  TtRssGetHeadlinesResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login(proxy);
    json[QSL("sid")] = m_sessionId;
    network_reply =
      NetworkFactory::performNetworkOperation(m_fullUrl,
                                              timeout,
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              result_raw,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              proxy);
    result = TtRssGetHeadlinesResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.m_networkError != QNetworkReply::NetworkError::NoError) {
    qWarningNN << LOGSEC_TTRSS << "getArticle failed with error:" << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  m_lastError = network_reply.m_networkError;
  return result;
}

TtRssGetHeadlinesResponse TtRssNetworkFactory::getHeadlines(int feed_id,
                                                            int limit,
                                                            int skip,
                                                            bool show_content,
                                                            bool include_attachments,
                                                            bool sanitize,
                                                            bool unread_only,
                                                            const QNetworkProxy& proxy) {
  QJsonObject json;

  json[QSL("op")] = QSL("getHeadlines");
  json[QSL("sid")] = m_sessionId;
  json[QSL("feed_id")] = feed_id;
  json[QSL("force_update")] = m_forceServerSideUpdate;
  json[QSL("limit")] = limit;
  json[QSL("skip")] = skip;
  json[QSL("view_mode")] = unread_only ? QSL("unread") : QSL("all_articles");
  json[QSL("show_content")] = show_content;
  json[QSL("include_attachments")] = include_attachments;
  json[QSL("sanitize")] = sanitize;

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_fullUrl,
                                            timeout,
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            proxy);
  TtRssGetHeadlinesResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login(proxy);
    json[QSL("sid")] = m_sessionId;
    network_reply =
      NetworkFactory::performNetworkOperation(m_fullUrl,
                                              timeout,
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              result_raw,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              proxy);
    result = TtRssGetHeadlinesResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS << "getHeadlines failed with error:" << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  m_lastError = network_reply.m_networkError;
  return result;
}

TtRssResponse TtRssNetworkFactory::setArticleLabel(const QStringList& article_ids,
                                                   const QString& label_custom_id,
                                                   bool assign,
                                                   const QNetworkProxy& proxy) {
  QJsonObject json;

  json[QSL("op")] = QSL("setArticleLabel");
  json[QSL("sid")] = m_sessionId;
  json[QSL("article_ids")] = article_ids.join(QSL(","));
  json[QSL("label_id")] = label_custom_id.toInt();
  json[QSL("assign")] = assign;

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_fullUrl,
                                            timeout,
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            proxy);
  TtRssResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login(proxy);
    json[QSL("sid")] = m_sessionId;
    network_reply =
      NetworkFactory::performNetworkOperation(m_fullUrl,
                                              timeout,
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              result_raw,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              proxy);
    result = TtRssResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS << "setArticleLabel failed with error"
               << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  m_lastError = network_reply.m_networkError;
  return result;
}

TtRssUpdateArticleResponse TtRssNetworkFactory::updateArticles(const QStringList& ids,
                                                               UpdateArticle::OperatingField field,
                                                               UpdateArticle::Mode mode,
                                                               const QNetworkProxy& proxy) {
  QJsonObject json;

  json[QSL("op")] = QSL("updateArticle");
  json[QSL("sid")] = m_sessionId;
  json[QSL("article_ids")] = ids.join(QSL(","));
  json[QSL("mode")] = int(mode);
  json[QSL("field")] = int(field);

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_fullUrl,
                                            timeout,
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            proxy);
  TtRssUpdateArticleResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login(proxy);
    json[QSL("sid")] = m_sessionId;
    network_reply =
      NetworkFactory::performNetworkOperation(m_fullUrl,
                                              timeout,
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              result_raw,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              proxy);
    result = TtRssUpdateArticleResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS << "updateArticle failed with error" << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  m_lastError = network_reply.m_networkError;
  return result;
}

TtRssSubscribeToFeedResponse TtRssNetworkFactory::subscribeToFeed(const QString& url,
                                                                  int category_id,
                                                                  const QNetworkProxy& proxy,
                                                                  bool protectd,
                                                                  const QString& username,
                                                                  const QString& password) {
  QJsonObject json;

  json[QSL("op")] = QSL("subscribeToFeed");
  json[QSL("sid")] = m_sessionId;
  json[QSL("feed_url")] = url;
  json[QSL("category_id")] = category_id;

  if (protectd) {
    json[QSL("login")] = username;
    json[QSL("password")] = password;
  }

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_fullUrl,
                                            timeout,
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            proxy);
  TtRssSubscribeToFeedResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login(proxy);
    json[QSL("sid")] = m_sessionId;
    network_reply =
      NetworkFactory::performNetworkOperation(m_fullUrl,
                                              timeout,
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              result_raw,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              proxy);
    result = TtRssSubscribeToFeedResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS << "updateArticle failed with error" << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  m_lastError = network_reply.m_networkError;
  return result;
}

TtRssUnsubscribeFeedResponse TtRssNetworkFactory::unsubscribeFeed(int feed_id, const QNetworkProxy& proxy) {
  QJsonObject json;

  json[QSL("op")] = QSL("unsubscribeFeed");
  json[QSL("sid")] = m_sessionId;
  json[QSL("feed_id")] = feed_id;
  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_fullUrl,
                                            timeout,
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            proxy);
  TtRssUnsubscribeFeedResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login(proxy);
    json[QSL("sid")] = m_sessionId;
    network_reply =
      NetworkFactory::performNetworkOperation(m_fullUrl,
                                              timeout,
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              result_raw,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              proxy);
    result = TtRssUnsubscribeFeedResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS << "getFeeds failed with error" << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  m_lastError = network_reply.m_networkError;
  return result;
}

int TtRssNetworkFactory::batchSize() const {
  return m_batchSize;
}

void TtRssNetworkFactory::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

bool TtRssNetworkFactory::intelligentSynchronization() const {
  return m_intelligentSynchronization;
}

void TtRssNetworkFactory::setIntelligentSynchronization(bool intelligent_synchronization) {
  m_intelligentSynchronization = intelligent_synchronization;
}

bool TtRssNetworkFactory::downloadOnlyUnreadMessages() const {
  return m_downloadOnlyUnreadMessages;
}

void TtRssNetworkFactory::setDownloadOnlyUnreadMessages(bool download_only_unread_messages) {
  m_downloadOnlyUnreadMessages = download_only_unread_messages;
}

bool TtRssNetworkFactory::forceServerSideUpdate() const {
  return m_forceServerSideUpdate;
}

void TtRssNetworkFactory::setForceServerSideUpdate(bool force_server_side_update) {
  m_forceServerSideUpdate = force_server_side_update;
}

bool TtRssNetworkFactory::authIsUsed() const {
  return m_authIsUsed;
}

void TtRssNetworkFactory::setAuthIsUsed(bool auth_is_used) {
  m_authIsUsed = auth_is_used;
}

QString TtRssNetworkFactory::authUsername() const {
  return m_authUsername;
}

void TtRssNetworkFactory::setAuthUsername(const QString& auth_username) {
  m_authUsername = auth_username;
}

QString TtRssNetworkFactory::authPassword() const {
  return m_authPassword;
}

void TtRssNetworkFactory::setAuthPassword(const QString& auth_password) {
  m_authPassword = auth_password;
}

TtRssResponse::TtRssResponse(const QString& raw_content) {
  m_rawContent = QJsonDocument::fromJson(raw_content.toUtf8()).object();
}

TtRssResponse::~TtRssResponse() = default;

bool TtRssResponse::isLoaded() const {
  return !m_rawContent.isEmpty();
}

int TtRssResponse::seq() const {
  if (!isLoaded()) {
    return TTRSS_CONTENT_NOT_LOADED;
  }
  else {
    return m_rawContent[QSL("seq")].toInt();
  }
}

int TtRssResponse::status() const {
  if (!isLoaded()) {
    return TTRSS_CONTENT_NOT_LOADED;
  }
  else {
    return m_rawContent[QSL("status")].toInt();
  }
}

bool TtRssResponse::isNotLoggedIn() const {
  return status() == TTRSS_API_STATUS_ERR && hasError() && error() == QSL(TTRSS_NOT_LOGGED_IN);
}

bool TtRssResponse::isUnknownMethod() const {
  return status() == TTRSS_API_STATUS_ERR && hasError() && error() == QSL(TTRSS_UNKNOWN_METHOD);
}

QString TtRssResponse::toString() const {
  return QJsonDocument(m_rawContent).toJson(QJsonDocument::JsonFormat::Compact);
}

TtRssLoginResponse::TtRssLoginResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

TtRssLoginResponse::~TtRssLoginResponse() = default;
int TtRssLoginResponse::apiLevel() const {
  if (!isLoaded()) {
    return TTRSS_CONTENT_NOT_LOADED;
  }
  else {
    return m_rawContent[QSL("content")].toObject()[QSL("api_level")].toInt();
  }
}

QString TtRssLoginResponse::sessionId() const {
  if (!isLoaded()) {
    return QString();
  }
  else {
    return m_rawContent[QSL("content")].toObject()[QSL("session_id")].toString();
  }
}

QString TtRssResponse::error() const {
  if (!isLoaded()) {
    return QString();
  }
  else {
    return m_rawContent[QSL("content")].toObject()[QSL("error")].toString();
  }
}

bool TtRssResponse::hasError() const {
  if (!isLoaded()) {
    return false;
  }
  else {
    return m_rawContent[QSL("content")].toObject().contains(QSL("error"));
  }
}

TtRssGetFeedsCategoriesResponse::TtRssGetFeedsCategoriesResponse(const QString& raw_content)
  : TtRssResponse(raw_content) {}

TtRssGetFeedsCategoriesResponse::~TtRssGetFeedsCategoriesResponse() = default;

RootItem* TtRssGetFeedsCategoriesResponse::feedsCategories(TtRssNetworkFactory* network,
                                                           bool obtain_icons,
                                                           const QNetworkProxy& proxy,
                                                           const QString& base_address) const {
  auto* parent = new RootItem();

  // Chop the "api/" from the end of the address.
  qDebugNN << LOGSEC_TTRSS << "Base address to get feed icons is" << QUOTE_W_SPACE_DOT(base_address);

  if (status() == TTRSS_API_STATUS_OK) {
    // We have data, construct object tree according to data.
    QJsonArray items_to_process =
      m_rawContent[QSL("content")].toObject()[QSL("categories")].toObject()[QSL("items")].toArray();
    QVector<QPair<RootItem*, QJsonValue>> pairs;
    pairs.reserve(items_to_process.size());

    for (const QJsonValue& item : items_to_process) {
      pairs.append(QPair<RootItem*, QJsonValue>(parent, item));
    }

    while (!pairs.isEmpty()) {
      QPair<RootItem*, QJsonValue> pair = pairs.takeFirst();
      RootItem* act_parent = pair.first;
      QJsonObject item = pair.second.toObject();
      int item_id = item[QSL("bare_id")].toInt();
      bool is_category = item.contains(QSL("type")) && item[QSL("type")].toString() == QSL(TTRSS_GFT_TYPE_CATEGORY);

      if (item_id >= 0) {
        if (is_category) {
          if (item_id == 0) {
            // This is "Uncategorized" category, all its feeds belong to top-level root.
            if (item.contains(QSL("items"))) {
              auto ite = item[QSL("items")].toArray();

              for (const QJsonValue& child_feed : std::as_const(ite)) {
                pairs.append(QPair<RootItem*, QJsonValue>(parent, child_feed));
              }
            }
          }
          else {
            auto* category = new Category();

            category->setTitle(item[QSL("name")].toString());
            category->setCustomId(QString::number(item_id));
            act_parent->appendChild(category);

            if (item.contains(QSL("items"))) {
              auto ite = item[QSL("items")].toArray();

              for (const QJsonValue& child : std::as_const(ite)) {
                pairs.append(QPair<RootItem*, QJsonValue>(category, child));
              }
            }
          }
        }
        else {
          // We have feed.
          auto* feed = new TtRssFeed();

          if (obtain_icons) {
            QString icon_path =
              item[QSL("icon")].type() == QJsonValue::Type::String ? item[QSL("icon")].toString() : QString();

            if (!icon_path.isEmpty()) {
              QString full_icon_address = QUrl(base_address).resolved(icon_path).toString();
              QPixmap icon;
              QList<QPair<QByteArray, QByteArray>> headers;

              if (network->authIsUsed()) {
                headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                                   network->authUsername(),
                                                                   network->authPassword());
              }

              auto res =
                NetworkFactory::downloadIcon({{full_icon_address, true}}, DOWNLOAD_TIMEOUT, icon, headers, proxy);

              if (res == QNetworkReply::NetworkError::NoError) {
                feed->setIcon(icon);
              }
              else {
                qWarningNN << LOGSEC_TTRSS << "Failed to download icon with error" << QUOTE_W_SPACE_DOT(res);
              }
            }
          }

          feed->setTitle(item[QSL("name")].toString());
          feed->setCustomId(QString::number(item_id));

          act_parent->appendChild(feed);
        }
      }
    }

    // Append special "published" feed to hold "notes" created by user
    // via "shareToPublished" method. These "notes" are not normal articles
    // because they do not belong to any feed.
    // We have feed.
    auto* published_feed = new TtRssFeed();

    published_feed->setTitle(QSL("[SYSTEM] ") + QObject::tr("User-published articles"));
    published_feed->setCustomId(QString::number(0));
    published_feed->setKeepOnTop(true);

    parent->appendChild(published_feed);
  }

  return parent;
}

TtRssGetHeadlinesResponse::TtRssGetHeadlinesResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

TtRssGetHeadlinesResponse::~TtRssGetHeadlinesResponse() = default;

TtRssGetArticleResponse::TtRssGetArticleResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

QList<Message> TtRssGetArticleResponse::messages(ServiceRoot* root) const {
  return {};
}

TtRssGetArticleResponse::~TtRssGetArticleResponse() = default;

QList<Message> TtRssGetHeadlinesResponse::messages(ServiceRoot* root) const {
  QList<Message> messages;
  auto active_labels = root->labelsNode() != nullptr ? root->labelsNode()->labels() : QList<Label*>();
  auto json_msgs = m_rawContent[QSL("content")].toArray();
  auto* published_lbl = boolinq::from(active_labels).firstOrDefault([](const Label* lbl) {
    return lbl->customNumericId() == TTRSS_PUBLISHED_LABEL_ID;
  });

  for (const QJsonValue& item : std::as_const(json_msgs)) {
    QJsonObject mapped = item.toObject();
    Message message;

    message.m_author = mapped[QSL("author")].toString();
    message.m_isRead = !mapped[QSL("unread")].toBool();
    message.m_isImportant = mapped[QSL("marked")].toBool();
    message.m_contents = mapped[QSL("content")].toString();
    message.m_rawContents = QJsonDocument(mapped).toJson(QJsonDocument::JsonFormat::Compact);

    if (published_lbl != nullptr && mapped[QSL("published")].toBool()) {
      // Article is published, set label.
      message.m_assignedLabels.append(published_lbl);
    }

    auto json_labels = mapped[QSL("labels")].toArray();

    for (const QJsonValue& lbl_val : std::as_const(json_labels)) {
      QString lbl_custom_id = QString::number(lbl_val.toArray().at(0).toInt());
      Label* label =
        boolinq::from(active_labels.begin(), active_labels.end()).firstOrDefault([lbl_custom_id](Label* lbl) {
          return lbl->customId() == lbl_custom_id;
        });

      if (label != nullptr) {
        message.m_assignedLabels.append(label);
      }
      else {
        qWarningNN << LOGSEC_TTRSS << "Label with custom ID" << QUOTE_W_SPACE(lbl_custom_id)
                   << "was not found. Maybe you need to perform sync-in to download it from server.";
      }
    }

    // Multiply by 1000 because Tiny Tiny RSS API does not include miliseconds in Unix
    // date/time number.
    const qint64 t = static_cast<qint64>(mapped[QSL("updated")].toDouble()) * 1000;

    message.m_created = TextFactory::parseDateTime(t);
    message.m_createdFromFeed = true;
    message.m_customId = QString::number(mapped[QSL("id")].toInt());
    message.m_feedId = mapped[QSL("feed_id")].type() == QJsonValue::Type::Double
                         ? QString::number(mapped[QSL("feed_id")].toInt())
                         : mapped[QSL("feed_id")].toString();
    message.m_title = mapped[QSL("title")].toString();
    message.m_url = mapped[QSL("link")].toString();

    if (mapped.contains(QSL("attachments"))) {
      // Process enclosures.
      auto json_att = mapped[QSL("attachments")].toArray();

      for (const QJsonValue& attachment : std::as_const(json_att)) {
        QJsonObject mapped_attachemnt = attachment.toObject();
        Enclosure* enclosure = new Enclosure(mapped_attachemnt[QSL("content_type")].toString(),
                                             mapped_attachemnt[QSL("content_url")].toString());

        message.m_enclosures.append(QSharedPointer<Enclosure>(enclosure));
      }
    }

    messages.append(message);
  }

  return messages;
}

TtRssUpdateArticleResponse::TtRssUpdateArticleResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

TtRssUpdateArticleResponse::~TtRssUpdateArticleResponse() = default;

QString TtRssUpdateArticleResponse::updateStatus() const {
  if (m_rawContent.contains(QSL("content"))) {
    return m_rawContent[QSL("content")].toObject()[QSL("status")].toString();
  }
  else {
    return QString();
  }
}

int TtRssUpdateArticleResponse::articlesUpdated() const {
  if (m_rawContent.contains(QSL("content"))) {
    return m_rawContent[QSL("content")].toObject()[QSL("updated")].toInt();
  }
  else {
    return 0;
  }
}

TtRssSubscribeToFeedResponse::TtRssSubscribeToFeedResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

TtRssSubscribeToFeedResponse::~TtRssSubscribeToFeedResponse() = default;
int TtRssSubscribeToFeedResponse::code() const {
  if (m_rawContent.contains(QSL("content"))) {
    return m_rawContent[QSL("content")].toObject()[QSL("status")].toObject()[QSL("code")].toInt();
  }
  else {
    return STF_UNKNOWN;
  }
}

TtRssUnsubscribeFeedResponse::TtRssUnsubscribeFeedResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

TtRssUnsubscribeFeedResponse::~TtRssUnsubscribeFeedResponse() = default;
QString TtRssUnsubscribeFeedResponse::code() const {
  if (m_rawContent.contains(QSL("content"))) {
    QJsonObject map = m_rawContent[QSL("content")].toObject();

    if (map.contains(QSL("error"))) {
      return map[QSL("error")].toString();
    }
    else if (map.contains(QSL("status"))) {
      return map[QSL("status")].toString();
    }
  }

  return QString();
}

TtRssGetLabelsResponse::TtRssGetLabelsResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

QList<RootItem*> TtRssGetLabelsResponse::labels() const {
  QList<RootItem*> labels;
  auto json_labels = m_rawContent[QSL("content")].toArray();

  // Add "Published" label.
  //
  // NOTE: In TT-RSS there is a problem with "published" feature:
  //   1. If user has article in existing feed, he can mark it as "published" and in
  // that case, the "published" behaves more like a label.
  //   2. If user uses feature "shareToPublished", he essentially creates new textual
  // note, which is then assigned to "Published feed" but can be also assigned label from 1).
  //
  // This label solves situation 1). 2) is solved in other way (creating static system feed).
  QString published_caption = QSL("[SYSTEM] ") + QObject::tr("Published articles");
  auto* published_lbl = new Label(published_caption, TextFactory::generateColorFromText(published_caption));

  published_lbl->setKeepOnTop(true);
  published_lbl->setCustomId(QString::number(TTRSS_PUBLISHED_LABEL_ID));
  labels.append(published_lbl);

  for (const QJsonValue& lbl_val : std::as_const(json_labels)) {
    QJsonObject lbl_obj = lbl_val.toObject();
    Label* lbl = new Label(lbl_obj[QSL("caption")].toString(), QColor(lbl_obj[QSL("fg_color")].toString()));

    lbl->setCustomId(QString::number(lbl_obj[QSL("id")].toInt()));
    labels.append(lbl);
  }

  return labels;
}

TtRssGetCompactHeadlinesResponse::TtRssGetCompactHeadlinesResponse(const QString& raw_content)
  : TtRssResponse(raw_content) {}

TtRssGetCompactHeadlinesResponse::~TtRssGetCompactHeadlinesResponse() = default;

QStringList TtRssGetCompactHeadlinesResponse::ids() const {
  auto json_ids = m_rawContent[QSL("content")].toArray();
  QStringList msg_ids;

  for (const QJsonValue& id_val : std::as_const(json_ids)) {
    msg_ids.append(QString::number(id_val.toObject()[QSL("id")].toInt()));
  }

  return msg_ids;
}
