// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/network/ttrssnetworkfactory.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/label.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/rootitem.h"
#include "services/abstract/serviceroot.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssfeed.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QPair>
#include <QVariant>

TtRssNetworkFactory::TtRssNetworkFactory()
  : m_bareUrl(QString()), m_fullUrl(QString()), m_username(QString()), m_password(QString()), m_forceServerSideUpdate(false),
  m_authIsUsed(false), m_authUsername(QString()), m_authPassword(QString()), m_sessionId(QString()),
  m_lastError(QNetworkReply::NoError) {}

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

TtRssLoginResponse TtRssNetworkFactory::login() {
  if (!m_sessionId.isEmpty()) {
    qWarningNN << LOGSEC_TTRSS
               << "Session ID is not empty before login, logging out first.";
    logout();
  }

  QJsonObject json;

  json["op"] = QSL("login");
  json["user"] = m_username;
  json["password"] = m_password;

  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl,
                                                                        qApp->settings()->value(GROUP(Feeds),
                                                                                                SETTING(
                                                                                                  Feeds::UpdateTimeout)).toInt(),
                                                                        QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                                        result_raw,
                                                                        QNetworkAccessManager::PostOperation,
                                                                        headers);
  TtRssLoginResponse login_response(QString::fromUtf8(result_raw));

  if (network_reply.first == QNetworkReply::NoError) {
    m_sessionId = login_response.sessionId();
    m_lastLoginTime = QDateTime::currentDateTime();
  }
  else {
    qWarningNN << LOGSEC_TTRSS
               << "Login failed with error:"
               << QUOTE_W_SPACE_DOT(network_reply.first);
  }

  m_lastError = network_reply.first;
  return login_response;
}

TtRssResponse TtRssNetworkFactory::logout() {
  if (!m_sessionId.isEmpty()) {
    QJsonObject json;

    json["op"] = QSL("logout");
    json["sid"] = m_sessionId;
    QByteArray result_raw;
    QList<QPair<QByteArray, QByteArray>> headers;

    headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
    headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

    NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl,
                                                                          qApp->settings()->value(GROUP(Feeds),
                                                                                                  SETTING(
                                                                                                    Feeds::UpdateTimeout)).toInt(),
                                                                          QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                                          result_raw,
                                                                          QNetworkAccessManager::PostOperation,
                                                                          headers);

    m_lastError = network_reply.first;

    if (m_lastError == QNetworkReply::NoError) {
      m_sessionId.clear();
    }
    else {
      qWarningNN << LOGSEC_TTRSS
                 << "Logout failed with error:"
                 << QUOTE_W_SPACE_DOT(network_reply.first);
    }

    return TtRssResponse(QString::fromUtf8(result_raw));
  }
  else {
    qWarningNN << LOGSEC_TTRSS
               << "Cannot logout because session ID is empty.";
    m_lastError = QNetworkReply::NoError;
    return TtRssResponse();
  }
}

TtRssGetLabelsResponse TtRssNetworkFactory::getLabels() {
  QJsonObject json;

  json["op"] = QSL("getLabels");
  json["sid"] = m_sessionId;

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout,
                                                                        QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                                        result_raw,
                                                                        QNetworkAccessManager::PostOperation,
                                                                        headers);
  TtRssGetLabelsResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login();
    json["sid"] = m_sessionId;
    network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                            result_raw,
                                                            QNetworkAccessManager::PostOperation,
                                                            headers);
    result = TtRssGetLabelsResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.first != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS
               << "getLabels failed with error:"
               << QUOTE_W_SPACE_DOT(network_reply.first);
  }

  m_lastError = network_reply.first;
  return result;
}

TtRssGetFeedsCategoriesResponse TtRssNetworkFactory::getFeedsCategories() {
  QJsonObject json;

  json["op"] = QSL("getFeedTree");
  json["sid"] = m_sessionId;
  json["include_empty"] = true;
  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout,
                                                                        QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                                        result_raw,
                                                                        QNetworkAccessManager::PostOperation,
                                                                        headers);
  TtRssGetFeedsCategoriesResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login();
    json["sid"] = m_sessionId;
    network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                            result_raw,
                                                            QNetworkAccessManager::PostOperation,
                                                            headers);
    result = TtRssGetFeedsCategoriesResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.first != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS
               << "getFeedTree failed with error:"
               << QUOTE_W_SPACE_DOT(network_reply.first);
  }

  m_lastError = network_reply.first;
  return result;
}

TtRssGetHeadlinesResponse TtRssNetworkFactory::getHeadlines(int feed_id, int limit, int skip,
                                                            bool show_content, bool include_attachments,
                                                            bool sanitize, bool unread_only) {
  QJsonObject json;

  json["op"] = QSL("getHeadlines");
  json["sid"] = m_sessionId;
  json["feed_id"] = feed_id;
  json["force_update"] = m_forceServerSideUpdate;
  json["limit"] = limit;
  json["skip"] = skip;
  json["view_mode"] = unread_only ? QSL("unread") : QSL("all_articles");
  json["show_content"] = show_content;
  json["include_attachments"] = include_attachments;
  json["sanitize"] = sanitize;
  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout,
                                                                        QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                                        result_raw,
                                                                        QNetworkAccessManager::PostOperation,
                                                                        headers);
  TtRssGetHeadlinesResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login();
    json["sid"] = m_sessionId;
    network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                            result_raw,
                                                            QNetworkAccessManager::PostOperation,
                                                            headers);
    result = TtRssGetHeadlinesResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.first != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS
               << "getHeadlines failed with error:"
               << QUOTE_W_SPACE_DOT(network_reply.first);
  }

  m_lastError = network_reply.first;
  return result;
}

TtRssResponse TtRssNetworkFactory::setArticleLabel(const QStringList& article_ids, const QString& label_custom_id, bool assign) {
  QJsonObject json;

  json["op"] = QSL("setArticleLabel");
  json["sid"] = m_sessionId;
  json["article_ids"] = article_ids.join(QSL(","));
  json["label_id"] = label_custom_id.toInt();
  json["assign"] = assign;

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout,
                                                                        QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                                        result_raw,
                                                                        QNetworkAccessManager::PostOperation,
                                                                        headers);
  TtRssResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login();
    json["sid"] = m_sessionId;
    network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                            result_raw,
                                                            QNetworkAccessManager::PostOperation,
                                                            headers);
    result = TtRssResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.first != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS
               << "setArticleLabel failed with error"
               << QUOTE_W_SPACE_DOT(network_reply.first);
  }

  m_lastError = network_reply.first;
  return result;
}

TtRssUpdateArticleResponse TtRssNetworkFactory::updateArticles(const QStringList& ids,
                                                               UpdateArticle::OperatingField field,
                                                               UpdateArticle::Mode mode) {
  QJsonObject json;

  json["op"] = QSL("updateArticle");
  json["sid"] = m_sessionId;
  json["article_ids"] = ids.join(QSL(","));
  json["mode"] = (int) mode;
  json["field"] = (int) field;

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout,
                                                                        QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                                        result_raw,
                                                                        QNetworkAccessManager::PostOperation,
                                                                        headers);
  TtRssUpdateArticleResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login();
    json["sid"] = m_sessionId;
    network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                            result_raw,
                                                            QNetworkAccessManager::PostOperation,
                                                            headers);
    result = TtRssUpdateArticleResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.first != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS
               << "updateArticle failed with error"
               << QUOTE_W_SPACE_DOT(network_reply.first);
  }

  m_lastError = network_reply.first;
  return result;
}

TtRssSubscribeToFeedResponse TtRssNetworkFactory::subscribeToFeed(const QString& url, int category_id,
                                                                  bool protectd, const QString& username,
                                                                  const QString& password) {
  QJsonObject json;

  json["op"] = QSL("subscribeToFeed");
  json["sid"] = m_sessionId;
  json["feed_url"] = url;
  json["category_id"] = category_id;

  if (protectd) {
    json["login"] = username;
    json["password"] = password;
  }

  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout,
                                                                        QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                                        result_raw,
                                                                        QNetworkAccessManager::PostOperation,
                                                                        headers);
  TtRssSubscribeToFeedResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login();
    json["sid"] = m_sessionId;
    network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                            result_raw,
                                                            QNetworkAccessManager::PostOperation,
                                                            headers);
    result = TtRssSubscribeToFeedResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.first != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS
               << "updateArticle failed with error"
               << QUOTE_W_SPACE_DOT(network_reply.first);
  }

  m_lastError = network_reply.first;
  return result;
}

TtRssUnsubscribeFeedResponse TtRssNetworkFactory::unsubscribeFeed(int feed_id) {
  QJsonObject json;

  json["op"] = QSL("unsubscribeFeed");
  json["sid"] = m_sessionId;
  json["feed_id"] = feed_id;
  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, TTRSS_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl,
                                                                        timeout,
                                                                        QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                                        result_raw,
                                                                        QNetworkAccessManager::PostOperation,
                                                                        headers);
  TtRssUnsubscribeFeedResponse result(QString::fromUtf8(result_raw));

  if (result.isNotLoggedIn()) {
    // We are not logged in.
    login();
    json["sid"] = m_sessionId;
    network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
                                                            result_raw,
                                                            QNetworkAccessManager::PostOperation,
                                                            headers);
    result = TtRssUnsubscribeFeedResponse(QString::fromUtf8(result_raw));
  }

  if (network_reply.first != QNetworkReply::NoError) {
    qWarningNN << LOGSEC_TTRSS
               << "getFeeds failed with error"
               << QUOTE_W_SPACE_DOT(network_reply.first);
  }

  m_lastError = network_reply.first;
  return result;
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
    return m_rawContent["seq"].toInt();
  }
}

int TtRssResponse::status() const {
  if (!isLoaded()) {
    return TTRSS_CONTENT_NOT_LOADED;
  }
  else {
    return m_rawContent["status"].toInt();
  }
}

bool TtRssResponse::isNotLoggedIn() const {
  return status() == TTRSS_API_STATUS_ERR && hasError() && error() == TTRSS_NOT_LOGGED_IN;
}

QString TtRssResponse::toString() const {
  return QJsonDocument(m_rawContent).toJson(QJsonDocument::Compact);
}

TtRssLoginResponse::TtRssLoginResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

TtRssLoginResponse::~TtRssLoginResponse() = default;
int TtRssLoginResponse::apiLevel() const {
  if (!isLoaded()) {
    return TTRSS_CONTENT_NOT_LOADED;
  }
  else {
    return m_rawContent["content"].toObject()["api_level"].toInt();
  }
}

QString TtRssLoginResponse::sessionId() const {
  if (!isLoaded()) {
    return QString();
  }
  else {
    return m_rawContent["content"].toObject()["session_id"].toString();
  }
}

QString TtRssResponse::error() const {
  if (!isLoaded()) {
    return QString();
  }
  else {
    return m_rawContent["content"].toObject()["error"].toString();
  }
}

bool TtRssResponse::hasError() const {
  if (!isLoaded()) {
    return false;
  }
  else {
    return m_rawContent["content"].toObject().contains("error");
  }
}

TtRssGetFeedsCategoriesResponse::TtRssGetFeedsCategoriesResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

TtRssGetFeedsCategoriesResponse::~TtRssGetFeedsCategoriesResponse() = default;

RootItem* TtRssGetFeedsCategoriesResponse::feedsCategories(bool obtain_icons, QString base_address) const {
  auto* parent = new RootItem();

  // Chop the "api/" from the end of the address.
  base_address.chop(4);
  qDebug("TT-RSS: Chopped base address to '%s' to get feed icons.", qPrintable(base_address));

  if (status() == TTRSS_API_STATUS_OK) {
    // We have data, construct object tree according to data.
    QJsonArray items_to_process = m_rawContent["content"].toObject()["categories"].toObject()["items"].toArray();
    QVector<QPair<RootItem*, QJsonValue>> pairs;

    for (const QJsonValue& item : items_to_process) {
      pairs.append(QPair<RootItem*, QJsonValue>(parent, item));
    }

    while (!pairs.isEmpty()) {
      QPair<RootItem*, QJsonValue> pair = pairs.takeFirst();
      RootItem* act_parent = pair.first;
      QJsonObject item = pair.second.toObject();
      int item_id = item["bare_id"].toInt();
      bool is_category = item.contains("type") && item["type"].toString() == TTRSS_GFT_TYPE_CATEGORY;

      if (item_id >= 0) {
        if (is_category) {
          if (item_id == 0) {
            // This is "Uncategorized" category, all its feeds belong to top-level root.
            if (item.contains("items")) {
              for (const QJsonValue& child_feed : item["items"].toArray()) {
                pairs.append(QPair<RootItem*, QJsonValue>(parent, child_feed));
              }
            }
          }
          else {
            auto* category = new Category();

            category->setTitle(item["name"].toString());
            category->setCustomId(QString::number(item_id));
            act_parent->appendChild(category);

            if (item.contains("items")) {
              for (const QJsonValue& child : item["items"].toArray()) {
                pairs.append(QPair<RootItem*, QJsonValue>(category, child));
              }
            }
          }
        }
        else {
          // We have feed.
          auto* feed = new TtRssFeed();

          if (obtain_icons) {
            QString icon_path = item["icon"].type() == QJsonValue::String ? item["icon"].toString() : QString();

            if (!icon_path.isEmpty()) {
              // Chop the "api/" suffix out and append
              QString full_icon_address = base_address + QL1C('/') + icon_path;
              QByteArray icon_data;

              if (NetworkFactory::performNetworkOperation(full_icon_address, DOWNLOAD_TIMEOUT,
                                                          QByteArray(), icon_data,
                                                          QNetworkAccessManager::GetOperation).first == QNetworkReply::NoError) {
                // Icon downloaded, set it up.
                QPixmap icon_pixmap;

                icon_pixmap.loadFromData(icon_data);
                feed->setIcon(QIcon(icon_pixmap));
              }
            }
          }

          feed->setTitle(item["name"].toString());
          feed->setCustomId(QString::number(item_id));
          act_parent->appendChild(feed);
        }
      }
    }
  }

  return parent;
}

TtRssGetHeadlinesResponse::TtRssGetHeadlinesResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

TtRssGetHeadlinesResponse::~TtRssGetHeadlinesResponse() = default;

QList<Message> TtRssGetHeadlinesResponse::messages(ServiceRoot* root) const {
  QList<Message> messages;
  auto active_labels = root->labelsNode() != nullptr ? root->labelsNode()->labels() : QList<Label*>();

  for (const QJsonValue& item : m_rawContent["content"].toArray()) {
    QJsonObject mapped = item.toObject();
    Message message;

    message.m_author = mapped["author"].toString();
    message.m_isRead = !mapped["unread"].toBool();
    message.m_isImportant = mapped["marked"].toBool();
    message.m_contents = mapped["content"].toString();

    for (const QJsonValue& lbl_val : mapped["labels"].toArray()) {
      QString lbl_custom_id = QString::number(lbl_val.toArray().at(0).toInt());
      Label* label = boolinq::from(active_labels.begin(), active_labels.end()).firstOrDefault([lbl_custom_id](Label* lbl) {
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
    const qint64 t = static_cast<qint64>(mapped["updated"].toDouble()) * 1000;

    message.m_created = TextFactory::parseDateTime(t);
    message.m_createdFromFeed = true;
    message.m_customId = QString::number(mapped["id"].toInt());
    message.m_feedId = mapped["feed_id"].toString();
    message.m_title = mapped["title"].toString();
    message.m_url = mapped["link"].toString();

    if (mapped.contains(QSL("attachments"))) {
      // Process enclosures.
      for (const QJsonValue& attachment : mapped["attachments"].toArray()) {
        QJsonObject mapped_attachemnt = attachment.toObject();
        Enclosure enclosure;

        enclosure.m_mimeType = mapped_attachemnt["content_type"].toString();
        enclosure.m_url = mapped_attachemnt["content_url"].toString();
        message.m_enclosures.append(enclosure);
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
    return m_rawContent["content"].toObject()["status"].toString();
  }
  else {
    return QString();
  }
}

int TtRssUpdateArticleResponse::articlesUpdated() const {
  if (m_rawContent.contains(QSL("content"))) {
    return m_rawContent["content"].toObject()["updated"].toInt();
  }
  else {
    return 0;
  }
}

TtRssSubscribeToFeedResponse::TtRssSubscribeToFeedResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

TtRssSubscribeToFeedResponse::~TtRssSubscribeToFeedResponse() = default;
int TtRssSubscribeToFeedResponse::code() const {
  if (m_rawContent.contains(QSL("content"))) {
    return m_rawContent["content"].toObject()["status"].toObject()["code"].toInt();
  }
  else {
    return STF_UNKNOWN;
  }
}

TtRssUnsubscribeFeedResponse::TtRssUnsubscribeFeedResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

TtRssUnsubscribeFeedResponse::~TtRssUnsubscribeFeedResponse() = default;
QString TtRssUnsubscribeFeedResponse::code() const {
  if (m_rawContent.contains(QSL("content"))) {
    QJsonObject map = m_rawContent["content"].toObject();

    if (map.contains(QSL("error"))) {
      return map["error"].toString();
    }
    else if (map.contains(QSL("status"))) {
      return map["status"].toString();
    }
  }

  return QString();
}

TtRssGetLabelsResponse::TtRssGetLabelsResponse(const QString& raw_content) : TtRssResponse(raw_content) {}

QList<RootItem*> TtRssGetLabelsResponse::labels() const {
  QList<RootItem*> labels;

  for (const QJsonValue& lbl_val : m_rawContent["content"].toArray()) {
    QJsonObject lbl_obj = lbl_val.toObject();
    Label* lbl = new Label(lbl_obj["caption"].toString(), QColor(lbl_obj["fg_color"].toString()));

    lbl->setCustomId(QString::number(lbl_obj["id"].toInt()));
    labels.append(lbl);
  }

  return labels;
}
