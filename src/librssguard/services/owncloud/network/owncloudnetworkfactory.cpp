// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/owncloud/network/owncloudnetworkfactory.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/rootitem.h"
#include "services/owncloud/definitions.h"
#include "services/owncloud/owncloudfeed.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPixmap>
#include <utility>

OwnCloudNetworkFactory::OwnCloudNetworkFactory()
  : m_url(QString()), m_fixedUrl(QString()), m_downloadOnlyUnreadMessages(false), m_forceServerSideUpdate(false),
  m_authUsername(QString()), m_authPassword(QString()), m_batchSize(OWNCLOUD_UNLIMITED_BATCH_SIZE), m_urlUser(QString()),
  m_urlStatus(QString()), m_urlFolders(QString()), m_urlFeeds(QString()), m_urlMessages(QString()),
  m_urlFeedsUpdate(QString()), m_urlDeleteFeed(QString()), m_urlRenameFeed(QString()) {}

OwnCloudNetworkFactory::~OwnCloudNetworkFactory() = default;

QString OwnCloudNetworkFactory::url() const {
  return m_url;
}

void OwnCloudNetworkFactory::setUrl(const QString& url) {
  m_url = url;

  if (url.endsWith('/')) {
    m_fixedUrl = url;
  }
  else {
    m_fixedUrl = url + '/';
  }

  // Store endpoints.
  m_urlUser = m_fixedUrl + OWNCLOUD_API_PATH + "user";
  m_urlStatus = m_fixedUrl + OWNCLOUD_API_PATH + "status";
  m_urlFolders = m_fixedUrl + OWNCLOUD_API_PATH + "folders";
  m_urlFeeds = m_fixedUrl + OWNCLOUD_API_PATH + "feeds";
  m_urlMessages = m_fixedUrl + OWNCLOUD_API_PATH + "items?id=%1&batchSize=%2&type=%3&getRead=%4";
  m_urlFeedsUpdate = m_fixedUrl + OWNCLOUD_API_PATH + "feeds/update?userId=%1&feedId=%2";
  m_urlDeleteFeed = m_fixedUrl + OWNCLOUD_API_PATH + "feeds/%1";
  m_urlRenameFeed = m_fixedUrl + OWNCLOUD_API_PATH + "feeds/%1/rename";
}

bool OwnCloudNetworkFactory::forceServerSideUpdate() const {
  return m_forceServerSideUpdate;
}

void OwnCloudNetworkFactory::setForceServerSideUpdate(bool force_update) {
  m_forceServerSideUpdate = force_update;
}

QString OwnCloudNetworkFactory::authUsername() const {
  return m_authUsername;
}

void OwnCloudNetworkFactory::setAuthUsername(const QString& auth_username) {
  m_authUsername = auth_username;
}

QString OwnCloudNetworkFactory::authPassword() const {
  return m_authPassword;
}

void OwnCloudNetworkFactory::setAuthPassword(const QString& auth_password) {
  m_authPassword = auth_password;
}

OwnCloudStatusResponse OwnCloudNetworkFactory::status(const QNetworkProxy& custom_proxy) {
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, OWNCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_urlStatus,
                                                                        qApp->settings()->value(GROUP(Feeds),
                                                                                                SETTING(Feeds::UpdateTimeout)).toInt(),
                                                                        QByteArray(),
                                                                        result_raw,
                                                                        QNetworkAccessManager::Operation::GetOperation,
                                                                        headers,
                                                                        false,
                                                                        {},
                                                                        {},
                                                                        custom_proxy);
  OwnCloudStatusResponse status_response(network_reply.first, QString::fromUtf8(result_raw));

  qDebugNN << LOGSEC_NEXTCLOUD
           << "Raw status data is:" << QUOTE_W_SPACE_DOT(result_raw);

  if (network_reply.first != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD
                << "Obtaining status info failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.first);
  }

  return status_response;
}

OwnCloudGetFeedsCategoriesResponse OwnCloudNetworkFactory::feedsCategories(const QNetworkProxy& custom_proxy) {
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, OWNCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_urlFolders,
                                                                        qApp->settings()->value(GROUP(Feeds),
                                                                                                SETTING(Feeds::UpdateTimeout)).toInt(),
                                                                        QByteArray(),
                                                                        result_raw,
                                                                        QNetworkAccessManager::Operation::GetOperation,
                                                                        headers,
                                                                        false,
                                                                        {},
                                                                        {},
                                                                        custom_proxy);

  if (network_reply.first != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD
                << "Obtaining of categories failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.first);
    return OwnCloudGetFeedsCategoriesResponse(network_reply.first);
  }

  QString content_categories = QString::fromUtf8(result_raw);

  // Now, obtain feeds.
  network_reply = NetworkFactory::performNetworkOperation(m_urlFeeds,
                                                          qApp->settings()->value(GROUP(Feeds),
                                                                                  SETTING(Feeds::UpdateTimeout)).toInt(),
                                                          QByteArray(),
                                                          result_raw,
                                                          QNetworkAccessManager::Operation::GetOperation,
                                                          headers,
                                                          false,
                                                          {},
                                                          {},
                                                          custom_proxy);

  if (network_reply.first != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD
                << "Obtaining of feeds failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.first);
    return OwnCloudGetFeedsCategoriesResponse(network_reply.first);
  }

  QString content_feeds = QString::fromUtf8(result_raw);

  return OwnCloudGetFeedsCategoriesResponse(network_reply.first, content_categories, content_feeds);
}

bool OwnCloudNetworkFactory::deleteFeed(const QString& feed_id, const QNetworkProxy& custom_proxy) {
  QString final_url = m_urlDeleteFeed.arg(feed_id);
  QByteArray raw_output;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, OWNCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(final_url,
                                                                        qApp->settings()->value(GROUP(Feeds),
                                                                                                SETTING(Feeds::UpdateTimeout)).toInt(),
                                                                        QByteArray(),
                                                                        raw_output,
                                                                        QNetworkAccessManager::Operation::DeleteOperation,
                                                                        headers,
                                                                        false,
                                                                        {},
                                                                        {},
                                                                        custom_proxy);

  if (network_reply.first != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD
                << "Obtaining of categories failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.first);
    return false;
  }
  else {
    return true;
  }
}

bool OwnCloudNetworkFactory::createFeed(const QString& url, int parent_id, const QNetworkProxy& custom_proxy) {
  QJsonObject json;

  json["url"] = url;

  auto nextcloud_version = status(custom_proxy).version();

  if (SystemFactory::isVersionEqualOrNewer(nextcloud_version, QSL("15.1.0"))) {
    json["folderId"] = parent_id == 0 ? QJsonValue(QJsonValue::Type::Null) : parent_id;
  }
  else {
    json["folderId"] = parent_id;
  }

  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, OWNCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_urlFeeds,
                                                                        qApp->settings()->value(GROUP(Feeds),
                                                                                                SETTING(Feeds::UpdateTimeout)).toInt(),
                                                                        QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                                                        result_raw,
                                                                        QNetworkAccessManager::Operation::PostOperation,
                                                                        headers,
                                                                        false,
                                                                        {},
                                                                        {},
                                                                        custom_proxy);

  if (network_reply.first != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD
                << "Creating of category failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.first);
    return false;
  }
  else {
    return true;
  }
}

bool OwnCloudNetworkFactory::renameFeed(const QString& new_name,
                                        const QString& custom_feed_id,
                                        const QNetworkProxy& custom_proxy) {
  QString final_url = m_urlRenameFeed.arg(custom_feed_id);
  QByteArray result_raw;
  QJsonObject json;

  json["feedTitle"] = new_name;

  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, OWNCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(
    final_url,
    qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt(),
    QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
    result_raw,
    QNetworkAccessManager::PutOperation,
    headers,
    false,
    {},
    {},
    custom_proxy);

  if (network_reply.first != QNetworkReply::NetworkError::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD
                << "Renaming of feed failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.first);
    return false;
  }
  else {
    return true;
  }
}

OwnCloudGetMessagesResponse OwnCloudNetworkFactory::getMessages(int feed_id, const QNetworkProxy& custom_proxy) {
  if (forceServerSideUpdate()) {
    triggerFeedUpdate(feed_id, custom_proxy);
  }

  QString final_url = m_urlMessages.arg(QString::number(feed_id),
                                        QString::number(batchSize() <= 0 ? -1 : batchSize()),
                                        QString::number(0),
                                        m_downloadOnlyUnreadMessages ? QSL("false") : QSL("true"));
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, OWNCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(final_url,
                                                                        qApp->settings()->value(GROUP(Feeds),
                                                                                                SETTING(Feeds::UpdateTimeout)).toInt(),
                                                                        QByteArray(),
                                                                        result_raw,
                                                                        QNetworkAccessManager::Operation::GetOperation,
                                                                        headers,
                                                                        false,
                                                                        {},
                                                                        {},
                                                                        custom_proxy);
  OwnCloudGetMessagesResponse msgs_response(network_reply.first, QString::fromUtf8(result_raw));

  if (network_reply.first != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD
                << "Obtaining messages failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.first);
  }

  return msgs_response;
}

QNetworkReply::NetworkError OwnCloudNetworkFactory::triggerFeedUpdate(int feed_id, const QNetworkProxy& custom_proxy) {
  // Now, we can trigger the update.
  QByteArray raw_output;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, OWNCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_urlFeedsUpdate.arg(authUsername(),
                                                                                             QString::number(feed_id)),
                                                                        qApp->settings()->value(GROUP(Feeds),
                                                                                                SETTING(Feeds::UpdateTimeout)).toInt(),
                                                                        QByteArray(),
                                                                        raw_output,
                                                                        QNetworkAccessManager::Operation::GetOperation,
                                                                        headers,
                                                                        false,
                                                                        {},
                                                                        {},
                                                                        custom_proxy);

  if (network_reply.first != QNetworkReply::NetworkError::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD
                << "Feeds update failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.first);
  }

  return network_reply.first;
}

NetworkResult OwnCloudNetworkFactory::markMessagesRead(RootItem::ReadStatus status,
                                                       const QStringList& custom_ids,
                                                       const QNetworkProxy& custom_proxy) {
  QJsonObject json;
  QJsonArray ids;
  QString final_url;

  if (status == RootItem::ReadStatus::Read) {
    final_url = m_fixedUrl + OWNCLOUD_API_PATH + "items/read/multiple";
  }
  else {
    final_url = m_fixedUrl + OWNCLOUD_API_PATH + "items/unread/multiple";
  }

  for (const QString& id : custom_ids) {
    ids.append(QJsonValue(id.toInt()));
  }

  json["items"] = ids;

  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, OWNCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  QByteArray output;

  return NetworkFactory::performNetworkOperation(final_url,
                                                 qApp->settings()->value(GROUP(Feeds),
                                                                         SETTING(Feeds::UpdateTimeout)).toInt(),
                                                 QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                                 output,
                                                 QNetworkAccessManager::Operation::PutOperation,
                                                 headers,
                                                 false,
                                                 {},
                                                 {},
                                                 custom_proxy);
}

NetworkResult OwnCloudNetworkFactory::markMessagesStarred(RootItem::Importance importance,
                                                          const QStringList& feed_ids,
                                                          const QStringList& guid_hashes,
                                                          const QNetworkProxy& custom_proxy) {
  QJsonObject json;
  QJsonArray ids;
  QString final_url;

  if (importance == RootItem::Importance::Important) {
    final_url = m_fixedUrl + OWNCLOUD_API_PATH + "items/star/multiple";
  }
  else {
    final_url = m_fixedUrl + OWNCLOUD_API_PATH + "items/unstar/multiple";
  }

  for (int i = 0; i < feed_ids.size(); i++) {
    QJsonObject item;

    item["feedId"] = feed_ids.at(i);
    item["guidHash"] = guid_hashes.at(i);
    ids.append(item);
  }

  json["items"] = ids;

  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, OWNCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(m_authUsername, m_authPassword);

  QByteArray output;

  return NetworkFactory::performNetworkOperation(final_url,
                                                 qApp->settings()->value(GROUP(Feeds),
                                                                         SETTING(Feeds::UpdateTimeout)).toInt(),
                                                 QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                                 output,
                                                 QNetworkAccessManager::Operation::PutOperation,
                                                 headers,
                                                 false,
                                                 {},
                                                 {},
                                                 custom_proxy);
}

int OwnCloudNetworkFactory::batchSize() const {
  return m_batchSize;
}

void OwnCloudNetworkFactory::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

bool OwnCloudNetworkFactory::downloadOnlyUnreadMessages() const {
  return m_downloadOnlyUnreadMessages;
}

void OwnCloudNetworkFactory::setDownloadOnlyUnreadMessages(bool dowload_only_unread_messages) {
  m_downloadOnlyUnreadMessages = dowload_only_unread_messages;
}

OwnCloudResponse::OwnCloudResponse(QNetworkReply::NetworkError response, const QString& raw_content) :
  m_networkError(response), m_rawContent(QJsonDocument::fromJson(raw_content.toUtf8()).object()),
  m_emptyString(raw_content.isEmpty()) {}

OwnCloudResponse::~OwnCloudResponse() = default;

bool OwnCloudResponse::isLoaded() const {
  return !m_emptyString && !m_rawContent.isEmpty();
}

QString OwnCloudResponse::toString() const {
  return QJsonDocument(m_rawContent).toJson(QJsonDocument::JsonFormat::Compact);
}

QNetworkReply::NetworkError OwnCloudResponse::networkError() const {
  return m_networkError;
}

OwnCloudStatusResponse::OwnCloudStatusResponse(QNetworkReply::NetworkError response, const QString& raw_content)
  : OwnCloudResponse(response, raw_content) {}

OwnCloudStatusResponse::~OwnCloudStatusResponse() = default;

QString OwnCloudStatusResponse::version() const {
  if (isLoaded()) {
    return m_rawContent["version"].toString();
  }
  else {
    return QString();
  }
}

bool OwnCloudStatusResponse::misconfiguredCron() const {
  if (isLoaded()) {
    return m_rawContent["warnings"].toObject()["improperlyConfiguredCron"].toBool();
  }
  else {
    return false;
  }
}

OwnCloudGetFeedsCategoriesResponse::OwnCloudGetFeedsCategoriesResponse(QNetworkReply::NetworkError response,
                                                                       QString raw_categories,
                                                                       QString raw_feeds)
  : OwnCloudResponse(response), m_contentCategories(std::move(raw_categories)), m_contentFeeds(std::move(raw_feeds)) {}

OwnCloudGetFeedsCategoriesResponse::~OwnCloudGetFeedsCategoriesResponse() = default;

RootItem* OwnCloudGetFeedsCategoriesResponse::feedsCategories(bool obtain_icons) const {
  auto* parent = new RootItem();
  QMap<QString, RootItem*> cats;

  // Top-level feed have "folderId" set to "0" or JSON "null" value.
  cats.insert(QSL("0"), parent);

  // Process categories first, then process feeds.
  for (const QJsonValue& cat : QJsonDocument::fromJson(m_contentCategories.toUtf8()).object()["folders"].toArray()) {
    QJsonObject item = cat.toObject();
    auto* category = new Category();

    category->setTitle(item["name"].toString());
    category->setCustomId(QString::number(item["id"].toInt()));
    cats.insert(category->customId(), category);

    // All categories in Nextcloud are top-level.
    parent->appendChild(category);
  }

  // We have categories added, now add all feeds.
  for (const QJsonValue& fed : QJsonDocument::fromJson(m_contentFeeds.toUtf8()).object()["feeds"].toArray()) {
    QJsonObject item = fed.toObject();
    auto* feed = new OwnCloudFeed();

    if (obtain_icons) {
      QString icon_path = item["faviconLink"].toString();

      if (!icon_path.isEmpty()) {
        QByteArray icon_data;

        if (NetworkFactory::performNetworkOperation(icon_path, DOWNLOAD_TIMEOUT,
                                                    QByteArray(), icon_data,
                                                    QNetworkAccessManager::GetOperation).first ==
            QNetworkReply::NetworkError::NoError) {
          // Icon downloaded, set it up.
          QPixmap icon_pixmap;

          icon_pixmap.loadFromData(icon_data);
          feed->setIcon(QIcon(icon_pixmap));
        }
      }
    }

    feed->setCustomId(QString::number(item["id"].toInt()));
    feed->setUrl(item["url"].toString());

    if (feed->url().isEmpty()) {
      feed->setUrl(item["link"].toString());
    }

    feed->setTitle(item["title"].toString());

    if (feed->title().isEmpty()) {
      if (feed->url().isEmpty()) {
        // We cannot add feed which has no title and no url to RSS Guard!!!
        qCriticalNN << LOGSEC_NEXTCLOUD
                    << "Skipping feed with custom ID"
                    << QUOTE_W_SPACE(feed->customId())
                    << "from adding to RSS Guard because it has no title and url.";
        continue;
      }
      else {
        feed->setTitle(feed->url());
      }
    }

    // NOTE: Starting with News 15.1.0, top-level feeds do not have parent folder ID 0, but JSON "null".
    // Luckily, if folder ID is not convertible to int, then default 0 value is returned.
    cats.value(QString::number(item["folderId"].toInt(0)))->appendChild(feed);
    qDebugNN << LOGSEC_NEXTCLOUD
             << "Custom ID of next fetched processed feed is"
             << QUOTE_W_SPACE_DOT(feed->customId());
  }

  return parent;
}

OwnCloudGetMessagesResponse::OwnCloudGetMessagesResponse(QNetworkReply::NetworkError response, const QString& raw_content)
  : OwnCloudResponse(response, raw_content) {}

OwnCloudGetMessagesResponse::~OwnCloudGetMessagesResponse() = default;

QList<Message>OwnCloudGetMessagesResponse::messages() const {
  QList<Message>msgs;

  for (const QJsonValue& message : m_rawContent["items"].toArray()) {
    QJsonObject message_map = message.toObject();
    Message msg;

    msg.m_author = message_map["author"].toString();
    msg.m_contents = message_map["body"].toString();
    msg.m_created = TextFactory::parseDateTime(message_map["pubDate"].toDouble() * 1000);
    msg.m_createdFromFeed = true;
    msg.m_customId = message_map["id"].isString() ? message_map["id"].toString() : QString::number(message_map["id"].toInt());
    msg.m_customHash = message_map["guidHash"].toString();

    QString enclosure_link = message_map["enclosureLink"].toString();

    if (!enclosure_link.isEmpty()) {
      Enclosure enclosure;

      enclosure.m_mimeType = message_map["enclosureMime"].toString();
      enclosure.m_url = enclosure_link;
      msg.m_enclosures.append(enclosure);
    }

    msg.m_feedId = message_map["feedId"].toString();
    msg.m_isImportant = message_map["starred"].toBool();
    msg.m_isRead = !message_map["unread"].toBool();
    msg.m_title = message_map["title"].toString();
    msg.m_url = message_map["url"].toString();
    msgs.append(msg);
  }

  return msgs;
}
