// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/nextcloudnetworkfactory.h"

#include "src/definitions.h"
#include "src/nextcloudfeed.h"

#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/network-web/networkfactory.h>
#include <librssguard/services/abstract/category.h>
#include <librssguard/services/abstract/rootitem.h>
#include <utility>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPixmap>

NextcloudNetworkFactory::NextcloudNetworkFactory()
  : m_url(QString()), m_fixedUrl(QString()), m_downloadOnlyUnreadMessages(false), m_forceServerSideUpdate(false),
    m_authUsername(QString()), m_authPassword(QString()), m_batchSize(NEXTCLOUD_DEFAULT_BATCH_SIZE),
    m_urlUser(QString()), m_urlStatus(QString()), m_urlFolders(QString()), m_urlFeeds(QString()),
    m_urlMessages(QString()), m_urlFeedsUpdate(QString()), m_urlDeleteFeed(QString()), m_urlRenameFeed(QString()) {}

NextcloudNetworkFactory::~NextcloudNetworkFactory() = default;

QString NextcloudNetworkFactory::url() const {
  return m_url;
}

void NextcloudNetworkFactory::setUrl(const QString& url) {
  m_url = url;

  if (url.endsWith('/')) {
    m_fixedUrl = url;
  }
  else {
    m_fixedUrl = url + '/';
  }

  // Store endpoints.
  m_urlUser = m_fixedUrl + NEXTCLOUD_API_PATH + "user";
  m_urlStatus = m_fixedUrl + NEXTCLOUD_API_PATH + "status";
  m_urlFolders = m_fixedUrl + NEXTCLOUD_API_PATH + "folders";
  m_urlFeeds = m_fixedUrl + NEXTCLOUD_API_PATH + "feeds";
  m_urlMessages = m_fixedUrl + NEXTCLOUD_API_PATH + "items?id=%1&batchSize=%2&type=%3&getRead=%4";
  m_urlFeedsUpdate = m_fixedUrl + NEXTCLOUD_API_PATH + "feeds/update?userId=%1&feedId=%2";
  m_urlDeleteFeed = m_fixedUrl + NEXTCLOUD_API_PATH + "feeds/%1";
  m_urlRenameFeed = m_fixedUrl + NEXTCLOUD_API_PATH + "feeds/%1/rename";
}

bool NextcloudNetworkFactory::forceServerSideUpdate() const {
  return m_forceServerSideUpdate;
}

void NextcloudNetworkFactory::setForceServerSideUpdate(bool force_update) {
  m_forceServerSideUpdate = force_update;
}

QString NextcloudNetworkFactory::authUsername() const {
  return m_authUsername;
}

void NextcloudNetworkFactory::setAuthUsername(const QString& auth_username) {
  m_authUsername = auth_username;
}

QString NextcloudNetworkFactory::authPassword() const {
  return m_authPassword;
}

void NextcloudNetworkFactory::setAuthPassword(const QString& auth_password) {
  m_authPassword = auth_password;
}

NextcloudStatusResponse NextcloudNetworkFactory::status(const QNetworkProxy& custom_proxy) {
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, NEXTCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_urlStatus,
                                            qApp->settings()
                                              ->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout))
                                              .toInt(),
                                            QByteArray(),
                                            result_raw,
                                            QNetworkAccessManager::Operation::GetOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            custom_proxy);
  NextcloudStatusResponse status_response(network_reply.m_networkError, QString::fromUtf8(result_raw));

  qDebugNN << LOGSEC_NEXTCLOUD << "Raw status data is:" << QUOTE_W_SPACE_DOT(result_raw);

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD << "Obtaining status info failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  return status_response;
}

NextcloudGetFeedsCategoriesResponse NextcloudNetworkFactory::feedsCategories(const QNetworkProxy& custom_proxy) {
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, NEXTCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_urlFolders,
                                            qApp->settings()
                                              ->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout))
                                              .toInt(),
                                            QByteArray(),
                                            result_raw,
                                            QNetworkAccessManager::Operation::GetOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            custom_proxy);

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD << "Obtaining of categories failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
    return NextcloudGetFeedsCategoriesResponse(network_reply.m_networkError);
  }

  QString content_categories = QString::fromUtf8(result_raw);

  // Now, obtain feeds.
  network_reply = NetworkFactory::performNetworkOperation(m_urlFeeds,
                                                          qApp->settings()
                                                            ->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout))
                                                            .toInt(),
                                                          QByteArray(),
                                                          result_raw,
                                                          QNetworkAccessManager::Operation::GetOperation,
                                                          headers,
                                                          false,
                                                          {},
                                                          {},
                                                          custom_proxy);

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD << "Obtaining of feeds failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
    return NextcloudGetFeedsCategoriesResponse(network_reply.m_networkError);
  }

  QString content_feeds = QString::fromUtf8(result_raw);

  return NextcloudGetFeedsCategoriesResponse(network_reply.m_networkError, content_categories, content_feeds);
}

bool NextcloudNetworkFactory::deleteFeed(const QString& feed_id, const QNetworkProxy& custom_proxy) {
  QString final_url = m_urlDeleteFeed.arg(feed_id);
  QByteArray raw_output;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, NEXTCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(final_url,
                                            qApp->settings()
                                              ->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout))
                                              .toInt(),
                                            QByteArray(),
                                            raw_output,
                                            QNetworkAccessManager::Operation::DeleteOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            custom_proxy);

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD << "Obtaining of categories failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
    return false;
  }
  else {
    return true;
  }
}

bool NextcloudNetworkFactory::createFeed(const QString& url, int parent_id, const QNetworkProxy& custom_proxy) {
  QJsonObject json;

  json[QSL("url")] = url;

  auto nextcloud_version = status(custom_proxy).version();

  if (SystemFactory::isVersionEqualOrNewer(nextcloud_version, QSL("15.1.0"))) {
    json[QSL("folderId")] = parent_id == 0 ? QJsonValue(QJsonValue::Type::Null) : parent_id;
  }
  else {
    json[QSL("folderId")] = parent_id;
  }

  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, NEXTCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_urlFeeds,
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
                                            custom_proxy);

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD << "Creating of category failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
    return false;
  }
  else {
    return true;
  }
}

bool NextcloudNetworkFactory::renameFeed(const QString& new_name,
                                         const QString& custom_feed_id,
                                         const QNetworkProxy& custom_proxy) {
  QString final_url = m_urlRenameFeed.arg(custom_feed_id);
  QByteArray result_raw;
  QJsonObject json;

  json[QSL("feedTitle")] = new_name;

  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, NEXTCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(final_url,
                                            qApp->settings()
                                              ->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout))
                                              .toInt(),
                                            QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                            result_raw,
                                            QNetworkAccessManager::Operation::PutOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            custom_proxy);

  if (network_reply.m_networkError != QNetworkReply::NetworkError::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD << "Renaming of feed failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
    return false;
  }
  else {
    return true;
  }
}

NextcloudGetMessagesResponse NextcloudNetworkFactory::getMessages(int feed_id, const QNetworkProxy& custom_proxy) {
  if (forceServerSideUpdate()) {
    triggerFeedUpdate(feed_id, custom_proxy);
  }

  QString final_url = m_urlMessages.arg(QString::number(feed_id),
                                        QString::number(batchSize() <= 0 ? -1 : batchSize()),
                                        QString::number(0),
                                        m_downloadOnlyUnreadMessages ? QSL("false") : QSL("true"));
  QByteArray result_raw;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, NEXTCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(final_url,
                                            qApp->settings()
                                              ->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout))
                                              .toInt(),
                                            QByteArray(),
                                            result_raw,
                                            QNetworkAccessManager::Operation::GetOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            custom_proxy);
  NextcloudGetMessagesResponse msgs_response(network_reply.m_networkError, QString::fromUtf8(result_raw));

  if (network_reply.m_networkError != QNetworkReply::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD << "Obtaining messages failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  return msgs_response;
}

QNetworkReply::NetworkError NextcloudNetworkFactory::triggerFeedUpdate(int feed_id, const QNetworkProxy& custom_proxy) {
  // Now, we can trigger the update.
  QByteArray raw_output;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, NEXTCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  NetworkResult network_reply =
    NetworkFactory::performNetworkOperation(m_urlFeedsUpdate.arg(authUsername(), QString::number(feed_id)),
                                            qApp->settings()
                                              ->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout))
                                              .toInt(),
                                            QByteArray(),
                                            raw_output,
                                            QNetworkAccessManager::Operation::GetOperation,
                                            headers,
                                            false,
                                            {},
                                            {},
                                            custom_proxy);

  if (network_reply.m_networkError != QNetworkReply::NetworkError::NoError) {
    qCriticalNN << LOGSEC_NEXTCLOUD << "Feeds update failed with error"
                << QUOTE_W_SPACE_DOT(network_reply.m_networkError);
  }

  return network_reply.m_networkError;
}

NetworkResult NextcloudNetworkFactory::markMessagesRead(RootItem::ReadStatus status,
                                                        const QStringList& custom_ids,
                                                        const QNetworkProxy& custom_proxy) {
  QJsonObject json;
  QJsonArray ids;
  QString final_url;

  if (status == RootItem::ReadStatus::Read) {
    final_url = m_fixedUrl + QSL(NEXTCLOUD_API_PATH) + QSL("items/read/multiple");
  }
  else {
    final_url = m_fixedUrl + QSL(NEXTCLOUD_API_PATH) + QSL("items/unread/multiple");
  }

  for (const QString& id : custom_ids) {
    ids.append(QJsonValue(id.toInt()));
  }

  json[QSL("items")] = ids;

  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, NEXTCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  QByteArray output;

  return NetworkFactory::performNetworkOperation(final_url,
                                                 qApp->settings()
                                                   ->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout))
                                                   .toInt(),
                                                 QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                                 output,
                                                 QNetworkAccessManager::Operation::PutOperation,
                                                 headers,
                                                 false,
                                                 {},
                                                 {},
                                                 custom_proxy);
}

NetworkResult NextcloudNetworkFactory::markMessagesStarred(RootItem::Importance importance,
                                                           const QStringList& feed_ids,
                                                           const QStringList& guid_hashes,
                                                           const QNetworkProxy& custom_proxy) {
  QJsonObject json;
  QJsonArray ids;
  QString final_url;

  if (importance == RootItem::Importance::Important) {
    final_url = m_fixedUrl + NEXTCLOUD_API_PATH + "items/star/multiple";
  }
  else {
    final_url = m_fixedUrl + NEXTCLOUD_API_PATH + "items/unstar/multiple";
  }

  for (int i = 0; i < feed_ids.size(); i++) {
    QJsonObject item;

    item[QSL("feedId")] = feed_ids.at(i);
    item[QSL("guidHash")] = guid_hashes.at(i);
    ids.append(item);
  }

  json[QSL("items")] = ids;

  QList<QPair<QByteArray, QByteArray>> headers;

  headers << QPair<QByteArray, QByteArray>(HTTP_HEADERS_CONTENT_TYPE, NEXTCLOUD_CONTENT_TYPE_JSON);
  headers << NetworkFactory::generateBasicAuthHeader(NetworkFactory::NetworkAuthentication::Basic,
                                                     m_authUsername,
                                                     m_authPassword);

  QByteArray output;

  return NetworkFactory::performNetworkOperation(final_url,
                                                 qApp->settings()
                                                   ->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout))
                                                   .toInt(),
                                                 QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                                 output,
                                                 QNetworkAccessManager::Operation::PutOperation,
                                                 headers,
                                                 false,
                                                 {},
                                                 {},
                                                 custom_proxy);
}

int NextcloudNetworkFactory::batchSize() const {
  return m_batchSize;
}

void NextcloudNetworkFactory::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

bool NextcloudNetworkFactory::downloadOnlyUnreadMessages() const {
  return m_downloadOnlyUnreadMessages;
}

void NextcloudNetworkFactory::setDownloadOnlyUnreadMessages(bool dowload_only_unread_messages) {
  m_downloadOnlyUnreadMessages = dowload_only_unread_messages;
}

NextcloudResponse::NextcloudResponse(QNetworkReply::NetworkError response, const QString& raw_content)
  : m_networkError(response), m_rawContent(QJsonDocument::fromJson(raw_content.toUtf8()).object()),
    m_emptyString(raw_content.isEmpty()) {}

NextcloudResponse::~NextcloudResponse() = default;

bool NextcloudResponse::isLoaded() const {
  return !m_emptyString && !m_rawContent.isEmpty();
}

QString NextcloudResponse::toString() const {
  return QJsonDocument(m_rawContent).toJson(QJsonDocument::JsonFormat::Compact);
}

QNetworkReply::NetworkError NextcloudResponse::networkError() const {
  return m_networkError;
}

NextcloudStatusResponse::NextcloudStatusResponse(QNetworkReply::NetworkError response, const QString& raw_content)
  : NextcloudResponse(response, raw_content) {}

NextcloudStatusResponse::~NextcloudStatusResponse() = default;

QString NextcloudStatusResponse::version() const {
  if (isLoaded()) {
    return m_rawContent[QSL("version")].toString();
  }
  else {
    return QString();
  }
}

NextcloudGetFeedsCategoriesResponse::NextcloudGetFeedsCategoriesResponse(QNetworkReply::NetworkError response,
                                                                         QString raw_categories,
                                                                         QString raw_feeds)
  : NextcloudResponse(response), m_contentCategories(std::move(raw_categories)), m_contentFeeds(std::move(raw_feeds)) {}

NextcloudGetFeedsCategoriesResponse::~NextcloudGetFeedsCategoriesResponse() = default;

RootItem* NextcloudGetFeedsCategoriesResponse::feedsCategories(bool obtain_icons) const {
  auto* parent = new RootItem();
  QMap<QString, RootItem*> cats;

  // Top-level feed have "folderId" set to "0" or JSON "null" value.
  cats.insert(QSL("0"), parent);

  // Process categories first, then process feeds.
  auto json_folders = QJsonDocument::fromJson(m_contentCategories.toUtf8()).object()[QSL("folders")].toArray();

  for (const QJsonValue& cat : std::as_const(json_folders)) {
    QJsonObject item = cat.toObject();
    auto* category = new Category();

    category->setTitle(item[QSL("name")].toString());
    category->setCustomId(QString::number(item[QSL("id")].toInt()));
    cats.insert(category->customId(), category);

    // All categories in Nextcloud are top-level.
    parent->appendChild(category);
  }

  // We have categories added, now add all feeds.
  auto json_feeds = QJsonDocument::fromJson(m_contentFeeds.toUtf8()).object()[QSL("feeds")].toArray();

  for (const QJsonValue& fed : std::as_const(json_feeds)) {
    QJsonObject item = fed.toObject();
    auto* feed = new NextcloudFeed();

    if (obtain_icons) {
      QString icon_path = item[QSL("faviconLink")].toString();

      if (!icon_path.isEmpty()) {
        QByteArray icon_data;

        if (NetworkFactory::performNetworkOperation(icon_path,
                                                    DOWNLOAD_TIMEOUT,
                                                    QByteArray(),
                                                    icon_data,
                                                    QNetworkAccessManager::Operation::GetOperation)
              .m_networkError == QNetworkReply::NetworkError::NoError) {
          // Icon downloaded, set it up.
          QPixmap icon_pixmap;

          icon_pixmap.loadFromData(icon_data);
          feed->setIcon(QIcon(icon_pixmap));
        }
      }
    }

    feed->setCustomId(QString::number(item[QSL("id")].toInt()));
    feed->setSource(item[QSL("url")].toString());

    if (feed->source().isEmpty()) {
      feed->setSource(item[QSL("link")].toString());
    }

    feed->setTitle(item[QSL("title")].toString());

    if (feed->title().isEmpty()) {
      if (feed->source().isEmpty()) {
        // We cannot add feed which has no title and no url to RSS Guard!!!
        qCriticalNN << LOGSEC_NEXTCLOUD << "Skipping feed with custom ID" << QUOTE_W_SPACE(feed->customId())
                    << "from adding to RSS Guard because it has no title and url.";
        continue;
      }
      else {
        feed->setTitle(feed->source());
      }
    }

    // NOTE: Starting with News 15.1.0, top-level feeds do not have parent folder ID 0, but JSON "null".
    // Luckily, if folder ID is not convertible to int, then default 0 value is returned.
    cats.value(QString::number(item[QSL("folderId")].toInt(0)))->appendChild(feed);
    qDebugNN << LOGSEC_NEXTCLOUD << "Custom ID of next fetched processed feed is"
             << QUOTE_W_SPACE_DOT(feed->customId());
  }

  return parent;
}

NextcloudGetMessagesResponse::NextcloudGetMessagesResponse(QNetworkReply::NetworkError response,
                                                           const QString& raw_content)
  : NextcloudResponse(response, raw_content) {}

NextcloudGetMessagesResponse::~NextcloudGetMessagesResponse() = default;

QList<Message> NextcloudGetMessagesResponse::messages() const {
  QList<Message> msgs;
  auto json_items = m_rawContent[QSL("items")].toArray();

  for (const QJsonValue& message : std::as_const(json_items)) {
    QJsonObject message_map = message.toObject();
    Message msg;

    msg.m_author = message_map[QSL("author")].toString();
    msg.m_contents = message_map[QSL("body")].toString();
    msg.m_created = TextFactory::parseDateTime(message_map[QSL("pubDate")].toDouble() * 1000);
    msg.m_createdFromFeed = true;
    msg.m_customId = message_map[QSL("id")].toVariant().toString();
    msg.m_customHash = message_map[QSL("guidHash")].toString();
    msg.m_rawContents = QJsonDocument(message_map).toJson(QJsonDocument::JsonFormat::Compact);

    // In case body is empty, check for content in mediaDescription if item is available.
    if (msg.m_contents.isEmpty() && !message_map[QSL("mediaDescription")].isUndefined()) {
      msg.m_contents = message_map[QSL("mediaDescription")].toString();
    }

    // Check for mediaThumbnail and append as first enclosure to be viewed in internal viewer.
    if (!message_map[QSL("mediaThumbnail")].isUndefined()) {
      MessageEnclosure* enclosure =
        new MessageEnclosure(message_map[QSL("mediaThumbnail")].toString(), QSL("image/jpg"));

      msg.m_enclosures.append(QSharedPointer<MessageEnclosure>(enclosure));
    }

    QString enclosure_link = message_map[QSL("enclosureLink")].toString();

    if (!enclosure_link.isEmpty()) {
      MessageEnclosure* enclosure = new MessageEnclosure();

      enclosure->setMimeType(message_map[QSL("enclosureMime")].toString());
      enclosure->setUrl(enclosure_link);

      if (enclosure->mimeType().isEmpty()) {
        enclosure->setMimeType(QSL("image/png"));
      }

      if (!message_map[QSL("enclosureMime")].toString().isEmpty() ||
          !enclosure_link.startsWith(QSL("https://www.youtube.com/v/"))) {
        msg.m_enclosures.append(QSharedPointer<MessageEnclosure>(enclosure));
      }
    }

    msg.m_isImportant = message_map[QSL("starred")].toBool();
    msg.m_isRead = !message_map[QSL("unread")].toBool();
    msg.m_title = message_map[QSL("title")].toString();
    msg.m_url = message_map[QSL("url")].toString();

    if (msg.m_title.simplified().isEmpty()) {
      msg.m_title = message_map[QSL("mediaDescription")].toString();
    }

    if (msg.m_title.simplified().isEmpty()) {
      msg.m_title = msg.m_url;
    }

    msgs.append(msg);
  }

  return msgs;
}
