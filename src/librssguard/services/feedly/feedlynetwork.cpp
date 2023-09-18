// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/feedlynetwork.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "exceptions/networkexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/label.h"
#include "services/abstract/labelsnode.h"
#include "services/feedly/definitions.h"
#include "services/feedly/feedlyserviceroot.h"

#if defined(FEEDLY_OFFICIAL_SUPPORT)
#include "network-web/oauth2service.h"
#endif

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

FeedlyNetwork::FeedlyNetwork(QObject* parent)
  : QObject(parent), m_service(nullptr),
#if defined(FEEDLY_OFFICIAL_SUPPORT)
    m_oauth(new OAuth2Service(QSL(FEEDLY_API_URL_BASE) + QSL(FEEDLY_API_URL_AUTH),
                              QSL(FEEDLY_API_URL_BASE) + QSL(FEEDLY_API_URL_TOKEN),
                              TextFactory::decrypt(QSL(FEEDLY_CLIENT_ID), OAUTH_DECRYPTION_KEY),
                              TextFactory::decrypt(QSL(FEEDLY_CLIENT_SECRET), OAUTH_DECRYPTION_KEY),
                              QSL(FEEDLY_API_SCOPE),
                              this)),
#endif
    m_username(QString()), m_developerAccessToken(QString()), m_batchSize(FEEDLY_DEFAULT_BATCH_SIZE),
    m_downloadOnlyUnreadMessages(false), m_intelligentSynchronization(true) {

#if defined(FEEDLY_OFFICIAL_SUPPORT)
  m_oauth->setRedirectUrl(QSL(OAUTH_REDIRECT_URI) + QL1C(':') + QString::number(FEEDLY_API_REDIRECT_URI_PORT), true);

  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, &FeedlyNetwork::onTokensError);
  connect(m_oauth, &OAuth2Service::authFailed, this, &FeedlyNetwork::onAuthFailed);
  connect(m_oauth, &OAuth2Service::tokensRetrieved, this, &FeedlyNetwork::onTokensRetrieved);
#endif
}

QList<Message> FeedlyNetwork::messages(const QString& stream_id,
                                       const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages) {
  if (!m_intelligentSynchronization) {
    return streamContents(stream_id);
  }

  // 1. Get unread IDs for a feed.
  // 2. Get read IDs for a feed.
  // 3. Download messages/contents for missing or changed IDs.
  QStringList remote_all_ids_list, remote_unread_ids_list;

  remote_unread_ids_list = streamIds(stream_id, true, batchSize());

  if (!downloadOnlyUnreadMessages()) {
    remote_all_ids_list = streamIds(stream_id, false, batchSize());
  }

  // 1.
  auto local_unread_ids_list = stated_messages.value(ServiceRoot::BagOfMessages::Unread);
  QSet<QString> local_unread_ids = FROM_LIST_TO_SET(QSet<QString>, local_unread_ids_list);
  QSet<QString> remote_unread_ids = FROM_LIST_TO_SET(QSet<QString>, remote_unread_ids_list);

  // 2.
  auto local_read_ids_list = stated_messages.value(ServiceRoot::BagOfMessages::Read);
  QSet<QString> local_read_ids = FROM_LIST_TO_SET(QSet<QString>, local_read_ids_list);
  QSet<QString> remote_read_ids = FROM_LIST_TO_SET(QSet<QString>, remote_all_ids_list) - remote_unread_ids;

  // 3.
  QSet<QString> to_download;

  // Undownloaded unread articles.
  to_download += remote_unread_ids - local_unread_ids;

  // Undownloaded read articles.
  if (!m_downloadOnlyUnreadMessages) {
    to_download += remote_read_ids - local_read_ids;
  }

  // Read articles newly marked as unread in service.
  auto moved_read = local_read_ids.intersect(remote_unread_ids);

  to_download += moved_read;

  // Unread articles newly marked as read in service.
  if (!m_downloadOnlyUnreadMessages) {
    auto moved_unread = local_unread_ids.intersect(remote_read_ids);

    to_download += moved_unread;
  }

  qDebugNN << LOGSEC_FEEDLY << "Will download" << QUOTE_W_SPACE(to_download.size()) << "articles.";

  if (to_download.isEmpty()) {
    return {};
  }
  else {
    return entries(QStringList(to_download.values()));
  }
}

void FeedlyNetwork::untagEntries(const QString& tag_id, const QStringList& msg_custom_ids) {
  if (msg_custom_ids.isEmpty()) {
    return;
  }

  QString bear = bearer();

  if (bear.isEmpty()) {
    qCriticalNN << LOGSEC_FEEDLY << "Cannot untag entries, because bearer is empty.";
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  QString target_url = fullUrl(Service::TagEntries) + QSL("/%1/").arg(QString(QUrl::toPercentEncoding(tag_id)));
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;
  int i = 0;

  do {
    auto msg_batch = msg_custom_ids.mid(i, FEEDLY_UNTAG_BATCH_SIZE);

    i += FEEDLY_UNTAG_BATCH_SIZE;

    auto ids = boolinq::from(msg_batch)
                 .select([](const QString& msg_id) {
                   return QString(QUrl::toPercentEncoding(msg_id));
                 })
                 .toStdList();
    QString final_url = target_url + FROM_STD_LIST(QStringList, ids).join(',');
    auto result = NetworkFactory::performNetworkOperation(final_url,
                                                          timeout,
                                                          {},
                                                          output,
                                                          QNetworkAccessManager::Operation::DeleteOperation,
                                                          {bearerHeader(bear)},
                                                          false,
                                                          {},
                                                          {},
                                                          m_service->networkProxy());

    if (result.m_networkError != QNetworkReply::NetworkError::NoError) {
      throw NetworkException(result.m_networkError, output);
    }
  }
  while (i < msg_custom_ids.size());
}

void FeedlyNetwork::tagEntries(const QString& tag_id, const QStringList& msg_custom_ids) {
  if (msg_custom_ids.isEmpty()) {
    return;
  }

  QString bear = bearer();

  if (bear.isEmpty()) {
    qCriticalNN << LOGSEC_FEEDLY << "Cannot tag entries, because bearer is empty.";
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  QString target_url = fullUrl(Service::TagEntries) + QSL("/%1").arg(QString(QUrl::toPercentEncoding(tag_id)));
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;
  QByteArray input_data;
  QJsonObject input;

  input[QSL("entryIds")] = QJsonArray::fromStringList(msg_custom_ids);
  input_data = QJsonDocument(input).toJson(QJsonDocument::JsonFormat::Compact);

  auto result =
    NetworkFactory::performNetworkOperation(target_url,
                                            timeout,
                                            input_data,
                                            output,
                                            QNetworkAccessManager::Operation::PutOperation,
                                            {bearerHeader(bear), {HTTP_HEADERS_CONTENT_TYPE, "application/json"}},
                                            false,
                                            {},
                                            {},
                                            m_service->networkProxy());

  if (result.m_networkError != QNetworkReply::NetworkError::NoError) {
    throw NetworkException(result.m_networkError, output);
  }
}

void FeedlyNetwork::markers(const QString& action, const QStringList& msg_custom_ids) {
  if (msg_custom_ids.isEmpty()) {
    return;
  }

  QString bear = bearer();

  if (bear.isEmpty()) {
    qCriticalNN << LOGSEC_FEEDLY << "Cannot mark entries, because bearer is empty.";
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  QString target_url = fullUrl(Service::Markers);
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;

  for (int i = 0; i < msg_custom_ids.size(); i += 500) {
    QJsonObject input;

    input[QSL("action")] = action;
    input[QSL("type")] = QSL("entries");
    input[QSL("entryIds")] = QJsonArray::fromStringList(msg_custom_ids.mid(i, 500));

    QByteArray input_data = QJsonDocument(input).toJson(QJsonDocument::JsonFormat::Compact);
    auto result =
      NetworkFactory::performNetworkOperation(target_url,
                                              timeout,
                                              input_data,
                                              output,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              {bearerHeader(bear), {HTTP_HEADERS_CONTENT_TYPE, "application/json"}},
                                              false,
                                              {},
                                              {},
                                              m_service->networkProxy());

    if (result.m_networkError != QNetworkReply::NetworkError::NoError) {
      throw NetworkException(result.m_networkError, output);
    }
  }
}

QList<Message> FeedlyNetwork::entries(const QStringList& ids) {
  const QString bear = bearer();

  if (bear.isEmpty()) {
    qCriticalNN << LOGSEC_FEEDLY << "Cannot obtain personal collections, because bearer is empty.";
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  QList<Message> msgs;
  int next_message = 0;
  QString continuation;
  const QString target_url = fullUrl(Service::Entries);
  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  do {
    QJsonArray json;

    for (int window = next_message + 1000; next_message < window && next_message < ids.size(); next_message++) {
      json.append(QJsonValue(ids.at(next_message)));
    }

    QByteArray output;
    auto result =
      NetworkFactory::performNetworkOperation(target_url,
                                              timeout,
                                              QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact),
                                              output,
                                              QNetworkAccessManager::Operation::PostOperation,
                                              {bearerHeader(bear)},
                                              false,
                                              {},
                                              {},
                                              m_service->networkProxy());

    if (result.m_networkError != QNetworkReply::NetworkError::NoError) {
      throw NetworkException(result.m_networkError, output);
    }

    msgs += decodeStreamContents(output, false, continuation);
  }
  while (next_message < ids.size());

  return msgs;
}

QList<Message> FeedlyNetwork::streamContents(const QString& stream_id) {
  QString bear = bearer();

  if (bear.isEmpty()) {
    qCriticalNN << LOGSEC_FEEDLY << "Cannot obtain personal collections, because bearer is empty.";
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;
  QString continuation;
  QList<Message> messages;

  // We download in batches.
  do {
    QString target_url = fullUrl(Service::StreamContents).arg(QString(QUrl::toPercentEncoding(stream_id)));

    if (m_downloadOnlyUnreadMessages) {
      target_url += QSL("&unreadOnly=true");
    }

    if (!continuation.isEmpty()) {
      target_url += QSL("&continuation=%1").arg(continuation);
    }

    if (m_batchSize > 0) {
      target_url += QSL("&count=%1").arg(QString::number(m_batchSize));
    }
    else {
      // User wants to download all messages. Make sure we use large batches
      // to limit network requests.
      target_url += QSL("&count=%1").arg(QString::number(FEEDLY_MAX_BATCH_SIZE));
    }

    auto result = NetworkFactory::performNetworkOperation(target_url,
                                                          timeout,
                                                          {},
                                                          output,
                                                          QNetworkAccessManager::Operation::GetOperation,
                                                          {bearerHeader(bear)},
                                                          false,
                                                          {},
                                                          {},
                                                          m_service->networkProxy());

    if (result.m_networkError != QNetworkReply::NetworkError::NoError) {
      throw NetworkException(result.m_networkError, output);
    }

    messages += decodeStreamContents(output, true, continuation);
  }
  while (!continuation.isEmpty() && (m_batchSize <= 0 || messages.size() < m_batchSize) &&
         messages.size() <= FEEDLY_MAX_TOTAL_SIZE);

  return messages;
}

QStringList FeedlyNetwork::streamIds(const QString& stream_id, bool unread_only, int batch_size) {
  QString bear = bearer();

  if (bear.isEmpty()) {
    qCriticalNN << LOGSEC_FEEDLY << "Cannot obtain stream IDs, because bearer is empty.";
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;
  QString continuation;
  QStringList messages;

  // We download in batches.
  do {
    QString target_url = fullUrl(Service::StreamIds).arg(QString(QUrl::toPercentEncoding(stream_id)));

    if (batch_size > 0) {
      target_url += QSL("?count=%1").arg(QString::number(batch_size));
    }
    else {
      // User wants to download all messages. Make sure we use large batches
      // to limit network requests.
      target_url += QSL("?count=%1").arg(QString::number(10000));
    }

    if (unread_only) {
      target_url += QSL("&unreadOnly=true");
    }

    if (!continuation.isEmpty()) {
      target_url += QSL("&continuation=%1").arg(continuation);
    }

    auto result = NetworkFactory::performNetworkOperation(target_url,
                                                          timeout,
                                                          {},
                                                          output,
                                                          QNetworkAccessManager::Operation::GetOperation,
                                                          {bearerHeader(bear)},
                                                          false,
                                                          {},
                                                          {},
                                                          m_service->networkProxy());

    if (result.m_networkError != QNetworkReply::NetworkError::NoError) {
      throw NetworkException(result.m_networkError, output);
    }

    messages += decodeStreamIds(output, continuation);
  }
  while (!continuation.isEmpty() && (batch_size <= 0 || messages.size() < batch_size));

  return messages;
}

QStringList FeedlyNetwork::decodeStreamIds(const QByteArray& stream_ids, QString& continuation) const {
  QStringList messages;
  QJsonDocument json = QJsonDocument::fromJson(stream_ids);

  continuation = json.object()[QSL("continuation")].toString();

  for (const QJsonValue& id_val : json.object()[QSL("ids")].toArray()) {
    messages << id_val.toString();
  }

  return messages;
}

QList<Message> FeedlyNetwork::decodeStreamContents(const QByteArray& stream_contents,
                                                   bool nested_items,
                                                   QString& continuation) const {
  QList<Message> messages;
  QJsonDocument json = QJsonDocument::fromJson(stream_contents);
  auto active_labels = m_service->labelsNode() != nullptr ? m_service->labelsNode()->labels() : QList<Label*>();

  continuation = json.object()[QSL("continuation")].toString();

  auto items = nested_items ? json.object()[QSL("items")].toArray() : json.array();

  for (const QJsonValue& entry : qAsConst(items)) {
    const QJsonObject& entry_obj = entry.toObject();
    Message message;

    message.m_feedId = entry_obj[QSL("origin")].toObject()[QSL("streamId")].toString();
    message.m_title = qApp->web()->stripTags(entry_obj[QSL("title")].toString());
    message.m_author = entry_obj[QSL("author")].toString();
    message.m_contents = entry_obj[QSL("content")].toObject()[QSL("content")].toString();
    message.m_rawContents = QJsonDocument(entry_obj).toJson(QJsonDocument::JsonFormat::Compact);

    if (message.m_contents.isEmpty()) {
      message.m_contents = entry_obj[QSL("summary")].toObject()[QSL("content")].toString();
    }

    message.m_createdFromFeed = true;
    message.m_created =
      QDateTime::fromMSecsSinceEpoch(entry_obj[QSL("published")].toVariant().toLongLong(), Qt::TimeSpec::UTC);
    message.m_customId = entry_obj[QSL("id")].toString();
    message.m_isRead = !entry_obj[QSL("unread")].toBool();
    message.m_url = entry_obj[QSL("canonicalUrl")].toString();

    if (message.m_url.isEmpty()) {
      auto canonical_arr = entry_obj[QSL("canonical")].toArray();

      if (!canonical_arr.isEmpty()) {
        message.m_url = canonical_arr.first().toObject()[QSL("href")].toString();
      }
      else {
        auto alternate_arr = entry_obj[QSL("alternate")].toArray();

        if (!alternate_arr.isEmpty()) {
          message.m_url = alternate_arr.first().toObject()[QSL("href")].toString();
        }
      }
    }

    auto enclosures = entry_obj[QSL("enclosure")].toArray();

    for (const QJsonValue& enc : qAsConst(enclosures)) {
      const QJsonObject& enc_obj = enc.toObject();
      const QString& enc_href = enc_obj[QSL("href")].toString();

      if (!boolinq::from(message.m_enclosures).any([enc_href](const Enclosure& existing_enclosure) {
            return existing_enclosure.m_url == enc_href;
          })) {
        message.m_enclosures.append(Enclosure(enc_href, enc_obj[QSL("type")].toString()));
      }
    }

    auto tags = entry_obj[QSL("tags")].toArray();

    for (const QJsonValue& tag : qAsConst(tags)) {
      const QJsonObject& tag_obj = tag.toObject();
      const QString& tag_id = tag_obj[QSL("id")].toString();

      if (tag_id.endsWith(FEEDLY_API_SYSTEM_TAG_SAVED)) {
        message.m_isImportant = true;
      }
      else if (tag_id.endsWith(FEEDLY_API_SYSTEM_TAG_READ)) {
        // NOTE: We don't do anything with "global read" tag.
      }
      else {
        Label* label = boolinq::from(active_labels.begin(), active_labels.end()).firstOrDefault([tag_id](Label* lbl) {
          return lbl->customId() == tag_id;
        });

        if (label != nullptr) {
          message.m_assignedLabels.append(label);
        }
        else {
          qCriticalNN << LOGSEC_FEEDLY << "Failed to find live Label object for tag" << QUOTE_W_SPACE_DOT(tag_id);
        }
      }
    }

    messages.append(message);
  }

  return messages;
}

RootItem* FeedlyNetwork::collections(bool obtain_icons) {
  QString bear = bearer();

  if (bear.isEmpty()) {
    qCriticalNN << LOGSEC_FEEDLY << "Cannot obtain personal collections, because bearer is empty.";
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  QString target_url = fullUrl(Service::Collections);
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;
  auto result = NetworkFactory::performNetworkOperation(target_url,
                                                        timeout,
                                                        {},
                                                        output,
                                                        QNetworkAccessManager::Operation::GetOperation,
                                                        {bearerHeader(bear)},
                                                        false,
                                                        {},
                                                        {},
                                                        m_service->networkProxy());

  if (result.m_networkError != QNetworkReply::NetworkError::NoError) {
    throw NetworkException(result.m_networkError, output);
  }

  return decodeCollections(output, obtain_icons, m_service->networkProxy(), timeout);
}

RootItem* FeedlyNetwork::decodeCollections(const QByteArray& json,
                                           bool obtain_icons,
                                           const QNetworkProxy& proxy,
                                           int timeout) const {
  QJsonDocument doc = QJsonDocument::fromJson(json);
  auto* parent = new RootItem();
  QList<QString> used_feeds;
  auto coll = doc.array();

  for (const QJsonValue& cat : qAsConst(coll)) {
    QJsonObject cat_obj = cat.toObject();
    auto* category = new Category(parent);

    category->setTitle(cat_obj[QSL("label")].toString());
    category->setCustomId(cat_obj[QSL("id")].toString());

    auto feeds = cat[QSL("feeds")].toArray();

    for (const QJsonValue& fee : qAsConst(feeds)) {
      QJsonObject fee_obj = fee.toObject();

      if (used_feeds.contains(fee_obj[QSL("id")].toString())) {
        qWarningNN << LOGSEC_FEEDLY << "Feed" << QUOTE_W_SPACE(fee_obj[QSL("id")].toString())
                   << "is already decoded and cannot be placed under several categories.";
        continue;
      }

      auto* feed = new Feed(category);

      feed->setSource(fee_obj[QSL("website")].toString());
      feed->setTitle(fee_obj[QSL("title")].toString());
      feed->setDescription(qApp->web()->stripTags(fee_obj[QSL("description")].toString()));
      feed->setCustomId(fee_obj[QSL("id")].toString());

      if (feed->title().isEmpty()) {
        feed->setTitle(feed->description());
      }

      if (feed->title().isEmpty()) {
        feed->setTitle(feed->source());
      }

      if (feed->title().isEmpty()) {
        feed->setTitle(feed->customId());
        qWarningNN << LOGSEC_FEEDLY
                   << "Some feed does not have nor title, neither description. Using its ID for its title.";
      }

      if (obtain_icons) {
        QPixmap icon;
        auto result = NetworkFactory::downloadIcon({{fee_obj[QSL("iconUrl")].toString(), true},
                                                    {fee_obj[QSL("website")].toString(), false},
                                                    {fee_obj[QSL("logo")].toString(), true}},
                                                   timeout,
                                                   icon,
                                                   {},
                                                   proxy);

        if (result == QNetworkReply::NetworkError::NoError && !icon.isNull()) {
          feed->setIcon(icon);
        }
      }

      used_feeds.append(feed->customId());
      category->appendChild(feed);
    }

    if (category->childCount() == 0) {
      delete category;
    }
    else {
      parent->appendChild(category);
    }
  }

  return parent;
}

QVariantHash FeedlyNetwork::profile(const QNetworkProxy& network_proxy) {
  QString bear = bearer();

  if (bear.isEmpty()) {
    qCriticalNN << LOGSEC_FEEDLY << "Cannot obtain profile information, because bearer is empty.";
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  QString target_url = fullUrl(Service::Profile);
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;

  // This method uses proxy via parameter,
  // not via "m_service" field.
  auto result = NetworkFactory::performNetworkOperation(target_url,
                                                        timeout,
                                                        {},
                                                        output,
                                                        QNetworkAccessManager::Operation::GetOperation,
                                                        {bearerHeader(bear)},
                                                        false,
                                                        {},
                                                        {},
                                                        network_proxy);

  if (result.m_networkError != QNetworkReply::NetworkError::NoError) {
    throw NetworkException(result.m_networkError, output);
  }

  return QJsonDocument::fromJson(output).object().toVariantHash();
}

QList<RootItem*> FeedlyNetwork::tags() {
  QString bear = bearer();

  if (bear.isEmpty()) {
    qCriticalNN << LOGSEC_FEEDLY << "Cannot obtain tags, because bearer is empty.";
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  QString target_url = fullUrl(Service::Tags);
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;
  auto result = NetworkFactory::performNetworkOperation(target_url,
                                                        timeout,
                                                        {},
                                                        output,
                                                        QNetworkAccessManager::Operation::GetOperation,
                                                        {bearerHeader(bear)},
                                                        false,
                                                        {},
                                                        {},
                                                        m_service->networkProxy());

  if (result.m_networkError != QNetworkReply::NetworkError::NoError) {
    throw NetworkException(result.m_networkError, output);
  }

  QJsonDocument json = QJsonDocument::fromJson(output);
  QList<RootItem*> lbls;
  auto tags = json.array();

  for (const QJsonValue& tag : qAsConst(tags)) {
    const QJsonObject& tag_obj = tag.toObject();
    QString name_id = tag_obj[QSL("id")].toString();

    if (name_id.endsWith(FEEDLY_API_SYSTEM_TAG_READ) || name_id.endsWith(FEEDLY_API_SYSTEM_TAG_SAVED)) {
      continue;
    }

    QString plain_name = tag_obj[QSL("label")].toString();
    auto* new_lbl = new Label(plain_name, TextFactory::generateColorFromText(name_id));

    new_lbl->setCustomId(name_id);
    lbls.append(new_lbl);
  }

  return lbls;
}

QString FeedlyNetwork::username() const {
  return m_username;
}

void FeedlyNetwork::setUsername(const QString& username) {
  m_username = username;
}

QString FeedlyNetwork::developerAccessToken() const {
  return m_developerAccessToken;
}

void FeedlyNetwork::setDeveloperAccessToken(const QString& dev_acc_token) {
  m_developerAccessToken = dev_acc_token;
}

int FeedlyNetwork::batchSize() const {
  return m_batchSize;
}

void FeedlyNetwork::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

#if defined(FEEDLY_OFFICIAL_SUPPORT)

void FeedlyNetwork::onTokensError(const QString& error, const QString& error_description) {
  Q_UNUSED(error)

  qApp->showGuiMessage(Notification::Event::LoginFailure,
                       {tr("Feedly: authentication error"),
                        tr("Click this to login again. Error is: '%1'").arg(error_description),
                        QSystemTrayIcon::MessageIcon::Critical},
                       {},
                       {tr("Login"), [this]() {
                          m_oauth->setAccessToken(QString());
                          m_oauth->setRefreshToken(QString());

                          // m_oauth->logout(false);
                          m_oauth->login();
                        }});
}

void FeedlyNetwork::onAuthFailed() {
  qApp->showGuiMessage(Notification::Event::LoginFailure,
                       {tr("Feedly: authorization denied"),
                        tr("Click this to login again."),
                        QSystemTrayIcon::MessageIcon::Critical},
                       {},
                       {tr("Login"), [this]() {
                          // m_oauth->logout(false);
                          m_oauth->login();
                        }});
}

void FeedlyNetwork::onTokensRetrieved(const QString& access_token, const QString& refresh_token, int expires_in) {
  Q_UNUSED(expires_in)
  Q_UNUSED(access_token)

  if (m_service != nullptr && !refresh_token.isEmpty()) {
    QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

    DatabaseQueries::storeNewOauthTokens(database, refresh_token, m_service->accountId());
  }
}

OAuth2Service* FeedlyNetwork::oauth() const {
  return m_oauth;
}

void FeedlyNetwork::setOauth(OAuth2Service* oauth) {
  m_oauth = oauth;
}

#endif

QString FeedlyNetwork::fullUrl(FeedlyNetwork::Service service) const {
  switch (service) {
    case Service::Profile:
      return QSL(FEEDLY_API_URL_BASE) + QSL(FEEDLY_API_URL_PROFILE);

    case Service::Collections:
      return QSL(FEEDLY_API_URL_BASE) + QSL(FEEDLY_API_URL_COLLETIONS);

    case Service::Tags:
    case Service::TagEntries:
      return QSL(FEEDLY_API_URL_BASE) + QSL(FEEDLY_API_URL_TAGS);

    case Service::StreamContents:
      return QSL(FEEDLY_API_URL_BASE) + QSL(FEEDLY_API_URL_STREAM_CONTENTS);

    case Service::StreamIds:
      return QSL(FEEDLY_API_URL_BASE) + QSL(FEEDLY_API_URL_STREAM_IDS);

    case Service::Entries:
      return QSL(FEEDLY_API_URL_BASE) + QSL(FEEDLY_API_URL_ENTRIES);

    case Service::Markers:
      return QSL(FEEDLY_API_URL_BASE) + QSL(FEEDLY_API_URL_MARKERS);

    default:
      return QSL(FEEDLY_API_URL_BASE);
  }
}

QString FeedlyNetwork::bearer() const {
#if defined(FEEDLY_OFFICIAL_SUPPORT)
  if (m_developerAccessToken.simplified().isEmpty()) {
    return m_oauth->bearer().toLocal8Bit();
  }
#endif

  return QSL("Bearer %1").arg(m_developerAccessToken);
}

QPair<QByteArray, QByteArray> FeedlyNetwork::bearerHeader(const QString& bearer) const {
  return {QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), bearer.toLocal8Bit()};
}

void FeedlyNetwork::setIntelligentSynchronization(bool intelligent_sync) {
  m_intelligentSynchronization = intelligent_sync;
}

bool FeedlyNetwork::intelligentSynchronization() const {
  return m_intelligentSynchronization;
}

bool FeedlyNetwork::downloadOnlyUnreadMessages() const {
  return m_downloadOnlyUnreadMessages;
}

void FeedlyNetwork::setDownloadOnlyUnreadMessages(bool download_only_unread_messages) {
  m_downloadOnlyUnreadMessages = download_only_unread_messages;
}

void FeedlyNetwork::setService(FeedlyServiceRoot* service) {
  m_service = service;
}
