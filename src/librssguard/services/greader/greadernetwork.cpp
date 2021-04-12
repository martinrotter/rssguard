// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/greadernetwork.h"

#include "3rd-party/boolinq/boolinq.h"
#include "miscellaneous/application.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/label.h"
#include "services/abstract/labelsnode.h"
#include "services/greader/definitions.h"
#include "services/greader/greaderfeed.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

GreaderNetwork::GreaderNetwork(QObject* parent)
  : QObject(parent), m_service(GreaderServiceRoot::Service::FreshRss), m_username(QString()), m_password(QString()),
  m_baseUrl(QString()), m_batchSize(GREADER_UNLIMITED_BATCH_SIZE) {
  clearCredentials();
}

QNetworkReply::NetworkError GreaderNetwork::editLabels(const QString& state,
                                                       bool assign,
                                                       const QStringList& msg_custom_ids,
                                                       const QNetworkProxy& proxy) {
  QString full_url = generateFullUrl(Operations::EditTag);
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  QNetworkReply::NetworkError network_err;

  if (!ensureLogin(proxy, &network_err)) {
    return network_err;
  }

  QStringList trimmed_ids;
  QRegularExpression regex_short_id(QSL("[0-9a-zA-Z]+$"));

  for (const QString& id : msg_custom_ids) {
    trimmed_ids.append(QString("i=") + id);
  }

  QStringList working_subset; working_subset.reserve(std::min(GREADER_API_EDIT_TAG_BATCH, trimmed_ids.size()));

  // Now, we perform messages update in batches (max X messages per batch).
  while (!trimmed_ids.isEmpty()) {
    // We take X IDs.
    for (int i = 0; i < GREADER_API_EDIT_TAG_BATCH && !trimmed_ids.isEmpty(); i++) {
      working_subset.append(trimmed_ids.takeFirst());
    }

    QString args;

    if (assign) {
      args = QString("a=") + state + "&";
    }
    else {
      args = QString("r=") + state + "&";
    }

    args += working_subset.join(QL1C('&'));

    if (m_service == GreaderServiceRoot::Service::Reedah) {
      args += QSL("&T=%1").arg(m_authToken);
    }

    // We send this batch.
    QByteArray output;
    auto result_edit = NetworkFactory::performNetworkOperation(full_url,
                                                               timeout,
                                                               args.toUtf8(),
                                                               output,
                                                               QNetworkAccessManager::Operation::PostOperation,
                                                               { authHeader(),
                                                                 { QSL(HTTP_HEADERS_CONTENT_TYPE).toLocal8Bit(),
                                                                   QSL("application/x-www-form-urlencoded").toLocal8Bit() } },
                                                               false,
                                                               {},
                                                               {},
                                                               proxy);

    if (result_edit.first != QNetworkReply::NetworkError::NoError) {
      return result_edit.first;
    }

    // Cleanup for next batch.
    working_subset.clear();
  }

  return QNetworkReply::NetworkError::NoError;
}

QNetworkReply::NetworkError GreaderNetwork::markMessagesRead(RootItem::ReadStatus status,
                                                             const QStringList& msg_custom_ids,
                                                             const QNetworkProxy& proxy) {
  return editLabels(GREADER_API_FULL_STATE_READ, status == RootItem::ReadStatus::Read, msg_custom_ids, proxy);
}

QNetworkReply::NetworkError GreaderNetwork::markMessagesStarred(RootItem::Importance importance,
                                                                const QStringList& msg_custom_ids,
                                                                const QNetworkProxy& proxy) {
  return editLabels(GREADER_API_FULL_STATE_IMPORTANT, importance == RootItem::Importance::Important, msg_custom_ids, proxy);
}

QList<Message> GreaderNetwork::streamContents(ServiceRoot* root, const QString& stream_id,
                                              Feed::Status& error, const QNetworkProxy& proxy) {
  QString full_url = generateFullUrl(Operations::StreamContents).arg(m_service == GreaderServiceRoot::Service::TheOldReader
                                                                     ? stream_id
                                                                     : QUrl::toPercentEncoding(stream_id),
                                                                     QString::number(batchSize() <= 0
                                                                                     ? 2000000
                                                                                     : batchSize()));
  auto timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  if (!ensureLogin(proxy)) {
    error = Feed::Status::AuthError;
    return {};
  }

  QByteArray output_stream;
  auto result_stream = NetworkFactory::performNetworkOperation(full_url,
                                                               timeout,
                                                               {},
                                                               output_stream,
                                                               QNetworkAccessManager::Operation::GetOperation,
                                                               { authHeader() },
                                                               false,
                                                               {},
                                                               {},
                                                               proxy);

  if (result_stream.first != QNetworkReply::NetworkError::NoError) {
    qCriticalNN << LOGSEC_GREADER
                << "Cannot download messages for "
                << QUOTE_NO_SPACE(stream_id)
                << ", network error:"
                << QUOTE_W_SPACE_DOT(result_stream.first);
    error = Feed::Status::NetworkError;
    return {};
  }
  else {
    error = Feed::Status::Normal;
    return decodeStreamContents(root, output_stream, stream_id);
  }
}

RootItem* GreaderNetwork::categoriesFeedsLabelsTree(bool obtain_icons, const QNetworkProxy& proxy) {
  QString full_url = generateFullUrl(Operations::TagList);
  auto timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  if (!ensureLogin(proxy)) {
    return nullptr;
  }

  QByteArray output_labels;
  auto result_labels = NetworkFactory::performNetworkOperation(full_url,
                                                               timeout,
                                                               {},
                                                               output_labels,
                                                               QNetworkAccessManager::Operation::GetOperation,
                                                               { authHeader() },
                                                               false,
                                                               {},
                                                               {},
                                                               proxy);

  if (result_labels.first != QNetworkReply::NetworkError::NoError) {
    return nullptr;
  }

  full_url = generateFullUrl(Operations::SubscriptionList);
  QByteArray output_feeds;
  auto result_feeds = NetworkFactory::performNetworkOperation(full_url,
                                                              timeout,
                                                              {},
                                                              output_feeds,
                                                              QNetworkAccessManager::Operation::GetOperation,
                                                              { authHeader() },
                                                              false,
                                                              {},
                                                              {},
                                                              proxy);

  if (result_feeds.first != QNetworkReply::NetworkError::NoError) {
    return nullptr;
  }

  return decodeTagsSubscriptions(output_labels, output_feeds, obtain_icons, proxy);
}

RootItem* GreaderNetwork::decodeTagsSubscriptions(const QString& categories, const QString& feeds,
                                                  bool obtain_icons, const QNetworkProxy& proxy) {
  auto* parent = new RootItem();
  auto timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QMap<QString, RootItem*> cats;
  QList<RootItem*> lbls;
  QJsonArray json;

  if (m_service == GreaderServiceRoot::Service::Bazqux ||
      m_service == GreaderServiceRoot::Service::Reedah) {
    // We need to process subscription list first and extract categories.
    json = QJsonDocument::fromJson(feeds.toUtf8()).object()["subscriptions"].toArray();

    for (const QJsonValue& feed : json) {
      auto subscription = feed.toObject();

      for (const QJsonValue& cat : subscription["categories"].toArray()) {
        auto cat_obj = cat.toObject();
        auto cat_id = cat_obj["id"].toString();

        if (!cats.contains(cat_id)) {
          auto* category = new Category();

          category->setTitle(cat_id.mid(cat_id.lastIndexOf(QL1C('/')) + 1));
          category->setCustomId(cat_id);

          cats.insert(category->customId(), category);
          parent->appendChild(category);
        }
      }
    }
  }

  json = QJsonDocument::fromJson(categories.toUtf8()).object()["tags"].toArray();
  cats.insert(QString(), parent);

  for (const QJsonValue& obj : json) {
    auto label = obj.toObject();
    QString label_id = label["id"].toString();

    if ((label["type"].toString() == QL1S("folder")) ||
        (m_service == GreaderServiceRoot::Service::TheOldReader &&
         label_id.contains(QSL("/label/")))) {

      // We have category (not "state" or "tag" or "label").
      auto* category = new Category();

      category->setDescription(label["htmlUrl"].toString());
      category->setTitle(label_id.mid(label_id.lastIndexOf(QL1C('/')) + 1));
      category->setCustomId(label_id);

      cats.insert(category->customId(), category);
      parent->appendChild(category);
    }
    else if (label["type"] == QL1S("tag")) {
      QString plain_name = QRegularExpression(".+\\/([^\\/]+)").match(label_id).captured(1);
      auto* new_lbl = new Label(plain_name, TextFactory::generateColorFromText(label_id));

      new_lbl->setCustomId(label_id);
      lbls.append(new_lbl);
    }
    else if ((m_service == GreaderServiceRoot::Service::Bazqux ||
              m_service == GreaderServiceRoot::Service::Reedah) &&
             label_id.contains(QSL("/label/"))) {
      if (!cats.contains(label_id)) {
        // This stream is not a category, it is label, bitches!
        QString plain_name = QRegularExpression(".+\\/([^\\/]+)").match(label_id).captured(1);
        auto* new_lbl = new Label(plain_name, TextFactory::generateColorFromText(label_id));

        new_lbl->setCustomId(label_id);
        lbls.append(new_lbl);
      }
    }
  }

  json = QJsonDocument::fromJson(feeds.toUtf8()).object()["subscriptions"].toArray();

  for (const QJsonValue& obj : json) {
    auto subscription = obj.toObject();
    QString id = subscription["id"].toString();
    QString title = subscription["title"].toString();
    QString url = subscription["htmlUrl"].toString();
    QString parent_label;
    QJsonArray assigned_categories = subscription["categories"].toArray();

    if (id.startsWith(TOR_SPONSORED_STREAM_ID)) {
      continue;
    }

    for (const QJsonValue& cat : assigned_categories) {
      QString potential_id = cat.toObject()["id"].toString();

      if (potential_id.contains(QSL("/label/"))) {
        parent_label = potential_id;
        break;
      }
    }

    // We have label (not "state").
    auto* feed = new GreaderFeed();

    feed->setDescription(url);
    feed->setUrl(url);
    feed->setTitle(title);
    feed->setCustomId(id);

    if (obtain_icons) {
      QString icon_url = subscription.contains(QSL("iconUrl"))
                         ? subscription["iconUrl"].toString()
                         : subscription["htmlUrl"].toString();

      if (!icon_url.isEmpty()) {
        QByteArray icon_data;

        if (icon_url.startsWith(QSL("//"))) {
          icon_url = QUrl(baseUrl()).scheme() + QSL(":") + icon_url;
        }

        QIcon icon;

        if (NetworkFactory::downloadIcon({ { icon_url, true } },
                                         timeout,
                                         icon,
                                         proxy) == QNetworkReply::NetworkError::NoError) {
          feed->setIcon(icon);
        }
      }
    }

    if (cats.contains(parent_label)) {
      cats[parent_label]->appendChild(feed);
    }
  }

  auto* lblroot = new LabelsNode(parent);

  lblroot->setChildItems(lbls);
  parent->appendChild(lblroot);

  return parent;
}

QNetworkReply::NetworkError GreaderNetwork::clientLogin(const QNetworkProxy& proxy) {
  QString full_url = generateFullUrl(Operations::ClientLogin);
  auto timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;
  QByteArray args = QSL("Email=%1&Passwd=%2").arg(username(), password()).toUtf8();
  auto network_result = NetworkFactory::performNetworkOperation(full_url,
                                                                timeout,
                                                                args,
                                                                output,
                                                                QNetworkAccessManager::Operation::PostOperation,
                                                                {},
                                                                false,
                                                                {},
                                                                {},
                                                                proxy);

  if (network_result.first == QNetworkReply::NetworkError::NoError) {
    // Save credentials.
    auto lines = QString::fromUtf8(output).replace(QSL("\r"), QString()).split('\n');

    for (const QString& line : lines) {
      int eq = line.indexOf('=');

      if (eq > 0) {
        QString id = line.mid(0, eq);

        if (id == QSL("SID")) {
          m_authSid = line.mid(eq + 1);
        }
        else if (id == QSL("Auth")) {
          m_authAuth = line.mid(eq + 1);
        }
      }
    }

    QRegularExpression exp("^(NA|unused|none|null)$");

    if (exp.match(m_authSid).hasMatch()) {
      m_authSid = QString();
    }

    if (exp.match(m_authAuth).hasMatch()) {
      m_authAuth = QString();
    }

    if (m_authAuth.isEmpty()) {
      clearCredentials();
      return QNetworkReply::NetworkError::InternalServerError;
    }

    if (m_service == GreaderServiceRoot::Service::Reedah) {
      // We need "T=" token for editing.
      full_url = generateFullUrl(Operations::Token);

      network_result = NetworkFactory::performNetworkOperation(full_url,
                                                               timeout,
                                                               args,
                                                               output,
                                                               QNetworkAccessManager::Operation::GetOperation,
                                                               { authHeader() },
                                                               false,
                                                               {},
                                                               {},
                                                               proxy);

      if (network_result.first == QNetworkReply::NetworkError::NoError) {
        m_authToken = output;
      }
      else {
        clearCredentials();
      }
    }
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

    case GreaderServiceRoot::Service::Reedah:
      return QSL("Reedah");

    case GreaderServiceRoot::Service::TheOldReader:
      return QSL("The Old Reader");

    default:
      return tr("Other services");
  }
}

QPair<QByteArray, QByteArray> GreaderNetwork::authHeader() const {
  return { QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), QSL("GoogleLogin auth=%1").arg(m_authAuth).toLocal8Bit() };
}

bool GreaderNetwork::ensureLogin(const QNetworkProxy& proxy, QNetworkReply::NetworkError* output) {
  if (m_authSid.isEmpty()) {
    auto login = clientLogin(proxy);

    if (output != nullptr) {
      *output = login;
    }

    if (login != QNetworkReply::NetworkError::NoError) {
      qCriticalNN << LOGSEC_GREADER
                  << "Login failed with error:"
                  << QUOTE_W_SPACE_DOT(NetworkFactory::networkErrorText(login));
      return false;
    }
  }

  return true;
}

QString GreaderNetwork::simplifyStreamId(const QString& stream_id) const {
  return QString(stream_id).replace(QRegularExpression("\\/\\d+\\/"), QSL("/-/"));
}

QList<Message> GreaderNetwork::decodeStreamContents(ServiceRoot* root,
                                                    const QString& stream_json_data,
                                                    const QString& stream_id) {
  QList<Message> messages;
  QJsonArray json = QJsonDocument::fromJson(stream_json_data.toUtf8()).object()["items"].toArray();
  auto active_labels = root->labelsNode() != nullptr ? root->labelsNode()->labels() : QList<Label*>();

  messages.reserve(json.count());

  for (const QJsonValue& obj : json) {
    auto message_obj = obj.toObject();
    Message message;

    message.m_title = qApp->web()->unescapeHtml(message_obj["title"].toString());
    message.m_author = qApp->web()->unescapeHtml(message_obj["author"].toString());
    message.m_created = QDateTime::fromSecsSinceEpoch(message_obj["published"].toInt(), Qt::UTC);
    message.m_createdFromFeed = true;
    message.m_customId = message_obj["id"].toString();

    auto alternates = message_obj["alternate"].toArray();
    auto enclosures = message_obj["enclosure"].toArray();
    auto categories = message_obj["categories"].toArray();

    for (const QJsonValue& alt : alternates) {
      auto alt_obj = alt.toObject();
      QString mime = alt_obj["type"].toString();
      QString href = alt_obj["href"].toString();

      if (mime.isEmpty() || mime == QL1S("text/html")) {
        message.m_url = href;
      }
      else {
        message.m_enclosures.append(Enclosure(href, mime));
      }
    }

    for (const QJsonValue& enc : enclosures) {
      auto enc_obj = enc.toObject();
      QString mime = enc_obj["type"].toString();
      QString href = enc_obj["href"].toString();

      message.m_enclosures.append(Enclosure(href, mime));
    }

    for (const QJsonValue& cat : categories) {
      QString category = cat.toString();

      if (category.endsWith(GREADER_API_STATE_READ)) {
        message.m_isRead = true;
      }
      else if (category.endsWith(GREADER_API_STATE_IMPORTANT)) {
        message.m_isImportant = true;
      }
      else if (category.contains(QSL("label"))) {
        Label* label = boolinq::from(active_labels.begin(), active_labels.end()).firstOrDefault([category](Label* lbl) {
          return lbl->customId() == category;
        });

        if (label != nullptr) {
          // We found live Label object for our assigned label.
          message.m_assignedLabels.append(label);
        }
      }
    }

    message.m_contents = message_obj["summary"].toObject()["content"].toString();
    message.m_feedId = stream_id;

    messages.append(message);
  }

  return messages;
}

int GreaderNetwork::batchSize() const {
  return m_batchSize;
}

void GreaderNetwork::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

void GreaderNetwork::clearCredentials() {
  m_authAuth = m_authSid = m_authToken = QString();
}

QString GreaderNetwork::sanitizedBaseUrl() const {
  auto base_url = m_baseUrl;

  if (!base_url.endsWith('/')) {
    base_url = base_url + QL1C('/');
  }

  switch (m_service) {
    case GreaderServiceRoot::Service::FreshRss:
      base_url += FRESHRSS_BASE_URL_PATH;
      break;

    default:
      break;
  }

  return base_url;
}

QString GreaderNetwork::generateFullUrl(GreaderNetwork::Operations operation) const {
  switch (operation) {
    case Operations::ClientLogin:
      return sanitizedBaseUrl() + GREADER_API_CLIENT_LOGIN;

    case Operations::Token:
      return sanitizedBaseUrl() + GREADER_API_TOKEN;

    case Operations::TagList:
      return sanitizedBaseUrl() + GREADER_API_TAG_LIST;

    case Operations::SubscriptionList:
      return sanitizedBaseUrl() + GREADER_API_SUBSCRIPTION_LIST;

    case Operations::StreamContents:
      return sanitizedBaseUrl() + GREADER_API_STREAM_CONTENTS;

    case Operations::EditTag:
      return sanitizedBaseUrl() + GREADER_API_EDIT_TAG;

    default:
      return sanitizedBaseUrl();
  }
}
