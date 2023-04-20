// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gmailnetworkfactory.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/networkexception.h"
#include "gui/dialogs/formmain.h"
#include "gui/tabwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "network-web/oauth2service.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webfactory.h"
#include "services/abstract/category.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailserviceroot.h"

#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>

GmailNetworkFactory::GmailNetworkFactory(QObject* parent)
  : QObject(parent), m_service(nullptr), m_username(QString()), m_batchSize(GMAIL_DEFAULT_BATCH_SIZE),
    m_downloadOnlyUnreadMessages(false), m_oauth2(new OAuth2Service(QSL(GMAIL_OAUTH_AUTH_URL),
                                                                    QSL(GMAIL_OAUTH_TOKEN_URL),
                                                                    {},
                                                                    {},
                                                                    QSL(GMAIL_OAUTH_SCOPE),
                                                                    this)) {
  initializeOauth();
}

void GmailNetworkFactory::setService(GmailServiceRoot* service) {
  m_service = service;
}

OAuth2Service* GmailNetworkFactory::oauth() const {
  return m_oauth2;
}

QString GmailNetworkFactory::username() const {
  return m_username;
}

int GmailNetworkFactory::batchSize() const {
  return m_batchSize;
}

void GmailNetworkFactory::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

QString GmailNetworkFactory::sendEmail(Mimesis::Message msg,
                                       const QNetworkProxy& custom_proxy,
                                       Message* reply_to_message) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    // throw ApplicationException(tr("you aren't logged in"));
  }

  if (reply_to_message != nullptr) {
    // We need to obtain some extra information.
    auto metadata =
      getMessageMetadata(reply_to_message->m_customId, {QSL("References"), QSL("Message-ID")}, custom_proxy);

    if (metadata.contains(QSL("Message-ID"))) {
      msg["References"] = metadata.value(QSL("Message-ID")).toStdString();
      msg["In-Reply-To"] = metadata.value(QSL("Message-ID")).toStdString();
    }
  }

  QString rfc_email = QString::fromStdString(msg.to_string());
  QByteArray input_data = rfc_email.toUtf8();
  QList<QPair<QByteArray, QByteArray>> headers;

  headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));
  headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_CONTENT_TYPE).toLocal8Bit(),
                                               QSL("message/rfc822").toLocal8Bit()));

  QByteArray out;
  auto result = NetworkFactory::performNetworkOperation(QSL(GMAIL_API_SEND_MESSAGE),
                                                        DOWNLOAD_TIMEOUT,
                                                        input_data,
                                                        out,
                                                        QNetworkAccessManager::Operation::PostOperation,
                                                        headers,
                                                        false,
                                                        {},
                                                        {},
                                                        custom_proxy);

  if (result.m_networkError != QNetworkReply::NetworkError::NoError) {
    if (!out.isEmpty()) {
      QJsonDocument doc = QJsonDocument::fromJson(out);
      auto json_message = doc.object()[QSL("error")].toObject()[QSL("message")].toString();

      throw ApplicationException(json_message);
    }
    else {
      throw ApplicationException(QString::fromUtf8(out));
    }
  }
  else {
    QJsonDocument doc = QJsonDocument::fromJson(out);
    auto msg_id = doc.object()[QSL("id")].toString();

    return msg_id;
  }
}

void GmailNetworkFactory::initializeOauth() {
#if defined(GMAIL_OFFICIAL_SUPPORT)
  m_oauth2->setClientSecretId(TextFactory::decrypt(QSL(GMAIL_CLIENT_ID), OAUTH_DECRYPTION_KEY));
  m_oauth2->setClientSecretSecret(TextFactory::decrypt(QSL(GMAIL_CLIENT_SECRET), OAUTH_DECRYPTION_KEY));
#endif

  m_oauth2->setRedirectUrl(QSL(OAUTH_REDIRECT_URI) + QL1C(':') + QString::number(GMAIL_OAUTH_REDIRECT_URI_PORT), true);

  connect(m_oauth2, &OAuth2Service::tokensRetrieveError, this, &GmailNetworkFactory::onTokensError);
  connect(m_oauth2, &OAuth2Service::authFailed, this, &GmailNetworkFactory::onAuthFailed);
  connect(m_oauth2,
          &OAuth2Service::tokensRetrieved,
          this,
          [this](QString access_token, QString refresh_token, int expires_in) {
            Q_UNUSED(expires_in)
            Q_UNUSED(access_token)

            if (m_service != nullptr && !refresh_token.isEmpty()) {
              QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

              DatabaseQueries::storeNewOauthTokens(database, refresh_token, m_service->accountId());
            }
          });
}

bool GmailNetworkFactory::downloadOnlyUnreadMessages() const {
  return m_downloadOnlyUnreadMessages;
}

void GmailNetworkFactory::setDownloadOnlyUnreadMessages(bool download_only_unread_messages) {
  m_downloadOnlyUnreadMessages = download_only_unread_messages;
}

QNetworkRequest GmailNetworkFactory::requestForAttachment(const QString& email_id, const QString& attachment_id) {
  QString target_url = QSL(GMAIL_API_GET_ATTACHMENT).arg(email_id, attachment_id);
  QNetworkRequest req(target_url);
  QByteArray bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  req.setRawHeader(QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), bearer);

  return req;
}

void GmailNetworkFactory::setOauth(OAuth2Service* oauth) {
  m_oauth2 = oauth;
}

void GmailNetworkFactory::setUsername(const QString& username) {
  m_username = username;
}

QList<Message> GmailNetworkFactory::messages(const QString& stream_id,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             Feed::Status& error,
                                             const QNetworkProxy& custom_proxy) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    error = Feed::Status::AuthError;
    return {};
  }

  const bool is_spam_feed = QString::compare(stream_id, QSL("SPAM"), Qt::CaseSensitivity::CaseInsensitive) == 0;

  // 1. Get unread IDs for a feed.
  // 2. Get read IDs for a feed.
  // 3. Get starred IDs for a feed.
  // 4. Download messages/contents for missing or changed IDs.
  QStringList remote_read_ids_list, remote_unread_ids_list, remote_starred_ids_list;

  try {
    remote_starred_ids_list = list(stream_id, {}, 0, is_spam_feed, QSL("is:starred"), custom_proxy);
    remote_unread_ids_list = list(stream_id, {}, batchSize(), is_spam_feed, QSL("is:unread"), custom_proxy);

    if (!downloadOnlyUnreadMessages()) {
      remote_read_ids_list = list(stream_id, {}, batchSize(), is_spam_feed, QSL("is:read"), custom_proxy);
    }
  }
  catch (const NetworkException& net_ex) {
    qCriticalNN << LOGSEC_GMAIL << "Failed to get list of e-mail IDs:" << QUOTE_W_SPACE_DOT(net_ex.message());
    return {};
  }

  // 1.
  auto local_unread_ids_list = stated_messages.value(ServiceRoot::BagOfMessages::Unread);
  QSet<QString> remote_unread_ids = FROM_LIST_TO_SET(QSet<QString>, remote_unread_ids_list);
  QSet<QString> local_unread_ids = FROM_LIST_TO_SET(QSet<QString>, local_unread_ids_list);

  // 2.
  auto local_read_ids_list = stated_messages.value(ServiceRoot::BagOfMessages::Read);
  QSet<QString> remote_read_ids = FROM_LIST_TO_SET(QSet<QString>, remote_read_ids_list);
  QSet<QString> local_read_ids = FROM_LIST_TO_SET(QSet<QString>, local_read_ids_list);

  // 3.
  auto local_starred_ids_list = stated_messages.value(ServiceRoot::BagOfMessages::Starred);
  QSet<QString> remote_starred_ids = FROM_LIST_TO_SET(QSet<QString>, remote_starred_ids_list);
  QSet<QString> local_starred_ids = FROM_LIST_TO_SET(QSet<QString>, local_starred_ids_list);

  // 4.
  QSet<QString> to_download;

  // Undownloaded unread e-mails.
  to_download += remote_unread_ids - local_unread_ids;

  // Undownloaded read e-mails.
  if (!m_downloadOnlyUnreadMessages) {
    to_download += remote_read_ids - local_read_ids;
  }

  // Undownloaded starred e-mails.
  to_download += remote_starred_ids - local_starred_ids;

  // Read e-mails newly marked as unread in service.
  auto moved_read = local_read_ids.intersect(remote_unread_ids);

  to_download += moved_read;

  // Unread e-mails newly marked as read in service.
  if (!m_downloadOnlyUnreadMessages) {
    auto moved_unread = local_unread_ids.intersect(remote_read_ids);

    to_download += moved_unread;
  }

  qDebugNN << LOGSEC_GMAIL << "Will download" << QUOTE_W_SPACE(to_download.size()) << "e-mails.";

  auto messages = obtainAndDecodeFullMessages(QList<QString>(to_download.values()), stream_id, custom_proxy);

  error = Feed::Status::Normal;
  return messages;
}

QNetworkReply::NetworkError GmailNetworkFactory::markMessagesRead(RootItem::ReadStatus status,
                                                                  const QStringList& custom_ids,
                                                                  const QNetworkProxy& custom_proxy) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    return QNetworkReply::NetworkError::AuthenticationRequiredError;
  }

  QList<QPair<QByteArray, QByteArray>> headers;

  headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));
  headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_CONTENT_TYPE).toLocal8Bit(),
                                               QSL(GMAIL_CONTENT_TYPE_JSON).toLocal8Bit()));

  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QJsonObject param_obj;
  QJsonArray param_add, param_remove;

  if (status == RootItem::ReadStatus::Read) {
    // We remove label UNREAD.
    param_remove.append(GMAIL_SYSTEM_LABEL_UNREAD);
  }
  else {
    // We add label UNREAD.
    param_add.append(GMAIL_SYSTEM_LABEL_UNREAD);
  }

  param_obj[QSL("addLabelIds")] = param_add;
  param_obj[QSL("removeLabelIds")] = param_remove;

  // We need to operate withing allowed batches.
  for (int i = 0; i < custom_ids.size(); i += GMAIL_MAX_BATCH_SIZE) {
    auto batch = custom_ids.mid(i, GMAIL_MAX_BATCH_SIZE);

    param_obj[QSL("ids")] = QJsonArray::fromStringList(batch);

    QJsonDocument param_doc(param_obj);
    QByteArray output;
    auto result = NetworkFactory::performNetworkOperation(QSL(GMAIL_API_BATCH_UPD_LABELS),
                                                          timeout,
                                                          param_doc.toJson(QJsonDocument::JsonFormat::Compact),
                                                          output,
                                                          QNetworkAccessManager::Operation::PostOperation,
                                                          headers,
                                                          false,
                                                          {},
                                                          {},
                                                          custom_proxy)
                    .m_networkError;

    if (result != QNetworkReply::NetworkError::NoError) {
      return result;
    }
  }

  return QNetworkReply::NetworkError::NoError;
}

QNetworkReply::NetworkError GmailNetworkFactory::markMessagesStarred(RootItem::Importance importance,
                                                                     const QStringList& custom_ids,
                                                                     const QNetworkProxy& custom_proxy) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    return QNetworkReply::NetworkError::AuthenticationRequiredError;
  }

  QList<QPair<QByteArray, QByteArray>> headers;

  headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));
  headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_CONTENT_TYPE).toLocal8Bit(),
                                               QSL(GMAIL_CONTENT_TYPE_JSON).toLocal8Bit()));

  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QJsonObject param_obj;
  QJsonArray param_add, param_remove;

  if (importance == RootItem::Importance::Important) {
    // We add label STARRED.
    param_add.append(GMAIL_SYSTEM_LABEL_STARRED);
  }
  else {
    // We remove label STARRED.
    param_remove.append(GMAIL_SYSTEM_LABEL_STARRED);
  }

  param_obj[QSL("addLabelIds")] = param_add;
  param_obj[QSL("removeLabelIds")] = param_remove;

  // We need to operate withing allowed batches.
  for (int i = 0; i < custom_ids.size(); i += GMAIL_MAX_BATCH_SIZE) {
    auto batch = custom_ids.mid(i, GMAIL_MAX_BATCH_SIZE);

    param_obj[QSL("ids")] = QJsonArray::fromStringList(batch);

    QJsonDocument param_doc(param_obj);
    QByteArray output;
    auto result = NetworkFactory::performNetworkOperation(QSL(GMAIL_API_BATCH_UPD_LABELS),
                                                          timeout,
                                                          param_doc.toJson(QJsonDocument::JsonFormat::Compact),
                                                          output,
                                                          QNetworkAccessManager::Operation::PostOperation,
                                                          headers,
                                                          false,
                                                          {},
                                                          {},
                                                          custom_proxy)
                    .m_networkError;

    if (result != QNetworkReply::NetworkError::NoError) {
      return result;
    }
  }

  return QNetworkReply::NetworkError::NoError;
}

QStringList GmailNetworkFactory::list(const QString& stream_id,
                                      const QStringList& label_ids,
                                      int max_results,
                                      bool include_spam,
                                      const QString& query,
                                      const QNetworkProxy& custom_proxy) {
  QList<QString> message_ids;
  QString next_page_token;
  QString bearer = m_oauth2->bearer().toLocal8Bit();
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  do {
    QString target_url = QSL(GMAIL_API_MSGS_LIST);

    target_url += QSL("?labelIds=%1").arg(stream_id);

    if (!label_ids.isEmpty()) {
      for (const QString& label_id : label_ids) {
        target_url += QSL("&labelIds=%1").arg(label_id);
      }
    }

    if (!query.isEmpty()) {
      target_url += QSL("&q=%1").arg(query);
    }

    if (include_spam) {
      target_url += QSL("&includeSpamTrash=true");
    }

    int remaining = max_results - message_ids.size();

    if (max_results <= 0 || remaining > 500) {
      target_url += QSL("&maxResults=500");
    }
    else {
      target_url += QSL("&maxResults=%1").arg(remaining);
    }

    if (!next_page_token.isEmpty()) {
      target_url += QString("&pageToken=%1").arg(next_page_token);
    }

    QByteArray messages_raw_data;
    auto netw =
      NetworkFactory::performNetworkOperation(target_url,
                                              timeout,
                                              {},
                                              messages_raw_data,
                                              QNetworkAccessManager::Operation::GetOperation,
                                              {{QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), bearer.toLocal8Bit()}},
                                              false,
                                              {},
                                              {},
                                              custom_proxy);

    if (netw.m_networkError == QNetworkReply::NetworkError::NoError) {
      // We parse this chunk.
      QString messages_data = QString::fromUtf8(messages_raw_data);

      message_ids << decodeLiteMessages(messages_data, next_page_token);
    }
    else {
      throw NetworkException(netw.m_networkError, tr("failed to download IDs of e-mail messages"));
    }
  }
  while (!next_page_token.isEmpty() && (max_results <= 0 || message_ids.size() < max_results));

  return message_ids;
}

QVariantHash GmailNetworkFactory::getProfile(const QNetworkProxy& custom_proxy) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    throw ApplicationException(tr("you are not logged in"));
  }

  QList<QPair<QByteArray, QByteArray>> headers;

  headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));

  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;
  auto result = NetworkFactory::performNetworkOperation(QSL(GMAIL_API_GET_PROFILE),
                                                        timeout,
                                                        {},
                                                        output,
                                                        QNetworkAccessManager::Operation::GetOperation,
                                                        headers,
                                                        false,
                                                        {},
                                                        {},
                                                        custom_proxy)
                  .m_networkError;

  if (result != QNetworkReply::NetworkError::NoError) {
    throw NetworkException(result, output);
  }
  else {
    QJsonDocument doc = QJsonDocument::fromJson(output);

    return doc.object().toVariantHash();
  }
}

void GmailNetworkFactory::onTokensError(const QString& error, const QString& error_description) {
  Q_UNUSED(error)

  qApp->showGuiMessage(Notification::Event::LoginFailure,
                       {tr("Gmail: authentication error"),
                        tr("Click this to login again. Error is: '%1'").arg(error_description),
                        QSystemTrayIcon::MessageIcon::Critical},
                       {},
                       {tr("Login"), [this]() {
                          m_oauth2->setAccessToken(QString());
                          m_oauth2->setRefreshToken(QString());
                          m_oauth2->login();
                        }});
}

void GmailNetworkFactory::onAuthFailed() {
  qApp->showGuiMessage(Notification::Event::LoginFailure,
                       {tr("Gmail: authorization denied"),
                        tr("Click this to login again."),
                        QSystemTrayIcon::MessageIcon::Critical},
                       {},
                       {tr("Login"), [this]() {
                          m_oauth2->login();
                        }});
}

bool GmailNetworkFactory::fillFullMessage(Message& msg, const QJsonObject& json, const QString& feed_id) const {
  QHash<QString, QString> headers;
  auto json_headers = json[QSL("payload")].toObject()[QSL("headers")].toArray();

  for (const QJsonValue& header : qAsConst(json_headers)) {
    headers.insert(header.toObject()[QSL("name")].toString(), header.toObject()["value"].toString());
  }

  msg.m_isRead = true;
  msg.m_rawContents = QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact);

  // Assign correct main labels/states.
  auto labelids = json[QSL("labelIds")].toArray().toVariantList();

  for (const QVariant& label : qAsConst(labelids)) {
    QString lbl = label.toString();

    if (lbl == QSL(GMAIL_SYSTEM_LABEL_UNREAD)) {
      msg.m_isRead = false;
    }
    else if (lbl == QSL(GMAIL_SYSTEM_LABEL_STARRED)) {
      msg.m_isImportant = true;
    }

    // RSS Guard does not support multi-labeling of messages, thus each message can have MAX single label.
    // Every message which is in INBOX, must be in INBOX, even if Gmail API returns more labels for the message.
    // I have to always decide which single label is most important one.
    if (lbl == QSL(GMAIL_SYSTEM_LABEL_INBOX) && feed_id != QSL(GMAIL_SYSTEM_LABEL_INBOX)) {
      // This message is in INBOX label too, but this updated feed is not INBOX,
      // we want to leave this message in INBOX and not duplicate it to other feed/label.
      return false;
    }

    if (lbl == QSL(GMAIL_SYSTEM_LABEL_TRASH) && feed_id != QSL(GMAIL_SYSTEM_LABEL_TRASH)) {
      // This message is in trash, but this updated feed is not recycle bin, we do not want
      // this message to appear anywhere.
      return false;
    }
  }

  msg.m_author = sanitizeEmailAuthor(headers[QSL("From")]);
  msg.m_title = headers[QSL("Subject")];

  // NOTE: Provide link to web-based GUI for the message.
  msg.m_url = QSL("https://mail.google.com/mail/u/0/#all/%1").arg(msg.m_customId);

  msg.m_createdFromFeed = true;
  msg.m_created = TextFactory::parseDateTime(headers[QSL("Date")]);

  if (!msg.m_created.isValid()) {
    msg.m_created = TextFactory::parseDateTime(headers[QSL("date")]);
  }

  if (msg.m_title.isEmpty()) {
    msg.m_title = tr("No subject");
  }

  QString backup_contents;
  QList<QJsonObject> parts_to_process, parts;

  parts_to_process.append(json[QSL("payload")].toObject());

  while (!parts_to_process.isEmpty()) {
    auto this_part = parts_to_process.takeFirst();
    auto nested_parts = this_part[QSL("parts")].toArray();

    for (const QJsonValue& prt : qAsConst(nested_parts)) {
      auto prt_obj = prt.toObject();

      parts.append(prt_obj);
      parts_to_process.append(prt_obj);
    }
  }

  if (json[QSL("payload")].toObject().contains(QSL("body"))) {
    parts.prepend(json[QSL("payload")].toObject());
  }

  for (const QJsonObject& part : qAsConst(parts)) {
    QJsonObject body = part[QSL("body")].toObject();
    QString mime = part[QSL("mimeType")].toString();
    QString filename = part[QSL("filename")].toString();

    if (filename.isEmpty() && mime.startsWith(QSL("text/"))) {
      // We have textual data of e-mail.
      // We check if it is HTML.
      if (msg.m_contents.isEmpty()) {
        if (mime.contains(QL1S("text/html"))) {
          msg.m_contents =
            QByteArray::fromBase64(body[QSL("data")].toString().toUtf8(), QByteArray::Base64Option::Base64UrlEncoding);

          if (msg.m_contents.contains(QSL("<body>"))) {
            int strt = msg.m_contents.indexOf(QSL("<body>"));
            int end = msg.m_contents.indexOf(QSL("</body>"));

            if (strt > 0 && end > strt) {
              msg.m_contents = msg.m_contents.mid(strt + 6, end - strt - 6);
            }
          }
        }
        else if (backup_contents.isEmpty()) {
          backup_contents =
            QByteArray::fromBase64(body[QSL("data")].toString().toUtf8(), QByteArray::Base64Option::Base64UrlEncoding);

          backup_contents = backup_contents.replace(QSL("\r\n"), QSL("\n"))
                              .replace(QSL("\n"), QSL("\n"))
                              .replace(QSL("\n"), QSL("<br/>"));
        }
      }
    }
    else if (!filename.isEmpty()) {
      // We have attachment.
      msg.m_enclosures.append(Enclosure(filename + QSL(GMAIL_ATTACHMENT_SEP) + body[QSL("attachmentId")].toString(),
                                        filename +
                                          QSL(" (%1 KB)").arg(QString::number(body["size"].toInt() / 1000.0))));
    }
  }

  if (msg.m_contents.isEmpty() && !backup_contents.isEmpty()) {
    msg.m_contents = backup_contents;
  }

  return true;
}

QMap<QString, QString> GmailNetworkFactory::getMessageMetadata(const QString& msg_id,
                                                               const QStringList& metadata,
                                                               const QNetworkProxy& custom_proxy) {
  QString bearer = m_oauth2->bearer();

  if (bearer.isEmpty()) {
    throw ApplicationException(tr("you are not logged in"));
  }

  QList<QPair<QByteArray, QByteArray>> headers;
  QByteArray output;
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), bearer.toLocal8Bit()));

  QString query = QString("%1/%2?format=metadata&metadataHeaders=%3")
                    .arg(QSL(GMAIL_API_MSGS_LIST), msg_id, metadata.join(QSL("&metadataHeaders=")));
  NetworkResult res = NetworkFactory::performNetworkOperation(query,
                                                              timeout,
                                                              QByteArray(),
                                                              output,
                                                              QNetworkAccessManager::Operation::GetOperation,
                                                              headers,
                                                              false,
                                                              {},
                                                              {},
                                                              custom_proxy);

  if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
    QJsonDocument doc = QJsonDocument::fromJson(output);
    QMap<QString, QString> result;
    auto json_headers = doc.object()[QSL("payload")].toObject()[QSL("headers")].toArray();

    for (const auto& header : json_headers) {
      QJsonObject obj_header = header.toObject();

      result.insert(obj_header[QSL("name")].toString(), obj_header[QSL("value")].toString());
    }

    return result;
  }
  else {
    throw ApplicationException(tr("failed to get metadata"));
  }
}

QList<Message> GmailNetworkFactory::obtainAndDecodeFullMessages(const QStringList& message_ids,
                                                                const QString& feed_id,
                                                                const QNetworkProxy& custom_proxy) const {
  QHash<QString, Message> msgs;
  int next_message = 0;
  QString bearer = m_oauth2->bearer();

  if (bearer.isEmpty()) {
    return {};
  }

  do {
    QHttpMultiPart multi;

    multi.setContentType(QHttpMultiPart::ContentType::MixedType);

    for (int window = next_message + 100; next_message < window && next_message < message_ids.size(); next_message++) {
      QString msg_id = message_ids[next_message];
      Message msg;
      QHttpPart part;

      msg.m_customId = msg_id;

      part.setRawHeader(HTTP_HEADERS_CONTENT_TYPE, GMAIL_CONTENT_TYPE_HTTP);
      QString full_msg_endpoint = QSL("GET /gmail/v1/users/me/messages/%1\r\n").arg(msg_id);

      part.setBody(full_msg_endpoint.toUtf8());
      multi.append(part);
      msgs.insert(msg_id, msg);
    }

    QList<QPair<QByteArray, QByteArray>> headers;
    QList<HttpResponse> output;
    int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

    headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), bearer.toLocal8Bit()));

    NetworkResult res = NetworkFactory::performNetworkOperation(GMAIL_API_BATCH,
                                                                timeout,
                                                                &multi,
                                                                output,
                                                                QNetworkAccessManager::Operation::PostOperation,
                                                                headers,
                                                                false,
                                                                {},
                                                                {},
                                                                custom_proxy);

    if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
      // We parse each part of HTTP response (it contains HTTP headers and payload with msg full data).
      for (const HttpResponse& part : qAsConst(output)) {
        QJsonObject msg_doc = QJsonDocument::fromJson(part.body().toUtf8()).object();
        QString msg_id = msg_doc[QSL("id")].toString();

        if (msgs.contains(msg_id)) {
          Message& msg = msgs[msg_id];

          if (!fillFullMessage(msg, msg_doc, feed_id)) {
            qWarningNN << LOGSEC_GMAIL << "Failed to get (or deliberately skipped) full message for custom ID:"
                       << QUOTE_W_SPACE_DOT(msg.m_customId);

            msgs.remove(msg_id);
          }
        }
      }
    }
    else {
      return {};
    }
  }
  while (next_message < message_ids.size());

  return msgs.values();
}

QStringList GmailNetworkFactory::decodeLiteMessages(const QString& messages_json_data, QString& next_page_token) const {
  QList<QString> message_ids;
  QJsonObject top_object = QJsonDocument::fromJson(messages_json_data.toUtf8()).object();
  QJsonArray json_msgs = top_object[QSL("messages")].toArray();

  next_page_token = top_object[QSL("nextPageToken")].toString();
  message_ids.reserve(json_msgs.count());

  for (const QJsonValue& obj : json_msgs) {
    auto message_obj = obj.toObject();

    message_ids << message_obj[QSL("id")].toString();
  }

  return message_ids;
}

QString GmailNetworkFactory::sanitizeEmailAuthor(const QString& author) const {
  return author.mid(0, author.indexOf(QL1S(" <"))).replace(QL1S("\""), QString());
}
