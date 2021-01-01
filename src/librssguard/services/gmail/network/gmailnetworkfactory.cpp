// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/network/gmailnetworkfactory.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "gui/dialogs/formmain.h"
#include "gui/tabwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "network-web/oauth2service.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webfactory.h"
#include "services/abstract/category.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailfeed.h"
#include "services/gmail/gmailserviceroot.h"

#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>

GmailNetworkFactory::GmailNetworkFactory(QObject* parent) : QObject(parent),
  m_service(nullptr), m_username(QString()), m_batchSize(GMAIL_DEFAULT_BATCH_SIZE),
  m_oauth2(new OAuth2Service(GMAIL_OAUTH_AUTH_URL, GMAIL_OAUTH_TOKEN_URL,
                             QString(), QString(), GMAIL_OAUTH_SCOPE, this)) {
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

QString GmailNetworkFactory::sendEmail(Mimesis::Message msg, Message* reply_to_message) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    //throw ApplicationException(tr("you aren't logged in"));
  }

  if (reply_to_message != nullptr) {
    // We need to obtain some extra information.

    auto metadata = getMessageMetadata(reply_to_message->m_customId, {
      QSL("References"),
      QSL("Message-ID")
    });

    /*if (metadata.contains(QSL("References"))) {

       }*/

    if (metadata.contains(QSL("Message-ID"))) {
      msg["References"] = metadata.value(QSL("Message-ID")).toStdString();
      msg["In-Reply-To"] = metadata.value(QSL("Message-ID")).toStdString();
    }
  }

  QString rfc_email = QString::fromStdString(msg.to_string());
  QByteArray input_data = rfc_email.toUtf8();
  QList<QPair<QByteArray, QByteArray>> headers;

  headers.append(QPair<QByteArray, QByteArray>(QString(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));
  headers.append(QPair<QByteArray, QByteArray>(QString(HTTP_HEADERS_CONTENT_TYPE).toLocal8Bit(),
                                               QString("message/rfc822").toLocal8Bit()));

  QByteArray out;
  auto result = NetworkFactory::performNetworkOperation(GMAIL_API_SEND_MESSAGE,
                                                        DOWNLOAD_TIMEOUT,
                                                        input_data,
                                                        out,
                                                        QNetworkAccessManager::Operation::PostOperation,
                                                        headers);

  if (result.first != QNetworkReply::NetworkError::NoError) {
    if (!out.isEmpty()) {
      QJsonDocument doc = QJsonDocument::fromJson(out);
      auto json_message = doc.object()["error"].toObject()["message"].toString();

      throw ApplicationException(json_message);
    }
    else {
      throw ApplicationException(QString::fromUtf8(out));
    }
  }
  else {
    QJsonDocument doc = QJsonDocument::fromJson(out);
    auto msg_id = doc.object()["id"].toString();

    return msg_id;
  }
}

void GmailNetworkFactory::initializeOauth() {
  connect(m_oauth2, &OAuth2Service::tokensRetrieveError, this, &GmailNetworkFactory::onTokensError);
  connect(m_oauth2, &OAuth2Service::authFailed, this, &GmailNetworkFactory::onAuthFailed);
  connect(m_oauth2, &OAuth2Service::tokensReceived, this, [this](QString access_token, QString refresh_token, int expires_in) {
    Q_UNUSED(expires_in)
    Q_UNUSED(access_token)

    if (m_service != nullptr && !refresh_token.isEmpty()) {
      QSqlDatabase database = qApp->database()->connection(metaObject()->className());
      DatabaseQueries::storeNewGmailTokens(database, refresh_token, m_service->accountId());

      qApp->showGuiMessage(tr("Logged in successfully"),
                           tr("Your login to Gmail was authorized."),
                           QSystemTrayIcon::MessageIcon::Information);
    }
  });
}

void GmailNetworkFactory::setUsername(const QString& username) {
  m_username = username;
}

Downloader* GmailNetworkFactory::downloadAttachment(const QString& msg_id, const QString& attachment_id) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    return nullptr;
  }
  else {
    auto* downloader = new Downloader();
    QString target_url = QString(GMAIL_API_GET_ATTACHMENT).arg(msg_id, attachment_id);

    downloader->appendRawHeader(QString(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), bearer.toLocal8Bit());
    downloader->downloadFile(target_url);

    return downloader;
  }
}

QList<Message> GmailNetworkFactory::messages(const QString& stream_id, Feed::Status& error) {
  Downloader downloader;
  QEventLoop loop;
  QString bearer = m_oauth2->bearer().toLocal8Bit();
  QString next_page_token;
  QList<Message> messages;

  if (bearer.isEmpty()) {
    error = Feed::Status::AuthError;
    return QList<Message>();
  }

  downloader.appendRawHeader(QString(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), bearer.toLocal8Bit());

  // We need to quit event loop when the download finishes.
  connect(&downloader, &Downloader::completed, &loop, &QEventLoop::quit);
  QString target_url;

  do {
    target_url = GMAIL_API_MSGS_LIST;
    target_url += QString("?labelIds=%1").arg(stream_id);

    if (batchSize() > 0) {
      target_url += QString("&maxResults=%1").arg(batchSize());
    }

    if (!next_page_token.isEmpty()) {
      target_url += QString("&pageToken=%1").arg(next_page_token);
    }

    downloader.manipulateData(target_url, QNetworkAccessManager::Operation::GetOperation);
    loop.exec();

    if (downloader.lastOutputError() == QNetworkReply::NetworkError::NoError) {
      // We parse this chunk.
      QString messages_data = downloader.lastOutputData();
      QList<Message> more_messages = decodeLiteMessages(messages_data, stream_id, next_page_token);

      if (!more_messages.isEmpty()) {
        // Now, we via batch HTTP request obtain full data for each message.
        bool obtained = obtainAndDecodeFullMessages(more_messages, stream_id);

        if (obtained) {
          messages.append(more_messages);

          // New batch of messages was obtained, check if we have enough.
          if (batchSize() > 0 && batchSize() <= messages.size()) {
            // We have enough messages.
            break;
          }
        }
        else {

          error = Feed::Status::NetworkError;
          return messages;
        }
      }
    }
    else {
      error = Feed::Status::NetworkError;
      return messages;
    }
  } while (!next_page_token.isEmpty());

  error = Feed::Status::Normal;
  return messages;
}

QNetworkReply::NetworkError GmailNetworkFactory::markMessagesRead(RootItem::ReadStatus status, QStringList custom_ids) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    return QNetworkReply::NetworkError::AuthenticationRequiredError;
  }

  QList<QPair<QByteArray, QByteArray>> headers;

  headers.append(QPair<QByteArray, QByteArray>(QString(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));
  headers.append(QPair<QByteArray, QByteArray>(QString(HTTP_HEADERS_CONTENT_TYPE).toLocal8Bit(),
                                               QString(GMAIL_CONTENT_TYPE_JSON).toLocal8Bit()));

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

  param_obj["addLabelIds"] = param_add;
  param_obj["removeLabelIds"] = param_remove;

  // We need to operate withing allowed batches.
  for (int i = 0; i < custom_ids.size(); i += GMAIL_MAX_BATCH_SIZE) {
    auto batch = custom_ids.mid(i, GMAIL_MAX_BATCH_SIZE);

    param_obj["ids"] = QJsonArray::fromStringList(batch);

    QJsonDocument param_doc(param_obj);
    QByteArray output;
    auto result = NetworkFactory::performNetworkOperation(GMAIL_API_BATCH_UPD_LABELS,
                                                          timeout,
                                                          param_doc.toJson(QJsonDocument::JsonFormat::Compact),
                                                          output,
                                                          QNetworkAccessManager::Operation::PostOperation,
                                                          headers).first;

    if (result != QNetworkReply::NetworkError::NoError) {
      return result;
    }
  }

  return QNetworkReply::NetworkError::NoError;
}

QNetworkReply::NetworkError GmailNetworkFactory::markMessagesStarred(RootItem::Importance importance, const QStringList& custom_ids) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    return QNetworkReply::NetworkError::AuthenticationRequiredError;
  }

  QList<QPair<QByteArray, QByteArray>> headers;

  headers.append(QPair<QByteArray, QByteArray>(QString(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));
  headers.append(QPair<QByteArray, QByteArray>(QString(HTTP_HEADERS_CONTENT_TYPE).toLocal8Bit(),
                                               QString(GMAIL_CONTENT_TYPE_JSON).toLocal8Bit()));

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

  param_obj["addLabelIds"] = param_add;
  param_obj["removeLabelIds"] = param_remove;

  // We need to operate withing allowed batches.
  for (int i = 0; i < custom_ids.size(); i += GMAIL_MAX_BATCH_SIZE) {
    auto batch = custom_ids.mid(i, GMAIL_MAX_BATCH_SIZE);

    param_obj["ids"] = QJsonArray::fromStringList(batch);

    QJsonDocument param_doc(param_obj);
    QByteArray output;
    auto result = NetworkFactory::performNetworkOperation(GMAIL_API_BATCH_UPD_LABELS,
                                                          timeout,
                                                          param_doc.toJson(QJsonDocument::JsonFormat::Compact),
                                                          output,
                                                          QNetworkAccessManager::Operation::PostOperation,
                                                          headers).first;

    if (result != QNetworkReply::NetworkError::NoError) {
      return result;
    }
  }

  return QNetworkReply::NetworkError::NoError;
}

void GmailNetworkFactory::onTokensError(const QString& error, const QString& error_description) {
  Q_UNUSED(error)

  qApp->showGuiMessage(tr("Gmail: authentication error"),
                       tr("Click this to login again. Error is: '%1'").arg(error_description),
                       QSystemTrayIcon::Critical,
                       nullptr, false,
                       [this]() {
    m_oauth2->setAccessToken(QString());
    m_oauth2->setRefreshToken(QString());
    m_oauth2->login();
  });
}

void GmailNetworkFactory::onAuthFailed() {
  qApp->showGuiMessage(tr("Gmail: authorization denied"),
                       tr("Click this to login again."),
                       QSystemTrayIcon::Critical,
                       nullptr, false,
                       [this]() {
    m_oauth2->login();
  });
}

bool GmailNetworkFactory::fillFullMessage(Message& msg, const QJsonObject& json, const QString& feed_id) {
  QHash<QString, QString> headers;

  for (const QJsonValue& header : json["payload"].toObject()["headers"].toArray()) {
    headers.insert(header.toObject()["name"].toString(), header.toObject()["value"].toString());
  }

  msg.m_isRead = true;

  // Assign correct main labels/states.
  for (const QVariant& label : json["labelIds"].toArray().toVariantList()) {
    QString lbl = label.toString();

    if (lbl == QL1S(GMAIL_SYSTEM_LABEL_UNREAD)) {
      msg.m_isRead = false;
    }
    else if (lbl == QL1S(GMAIL_SYSTEM_LABEL_STARRED)) {
      msg.m_isImportant = true;
    }

    // RSS Guard does not support multi-labeling of messages, thus each message can have MAX single label.
    // Every message which is in INBOX, must be in INBOX, even if Gmail API returns more labels for the message.
    // I have to always decide which single label is most important one.
    if (lbl == QL1S(GMAIL_SYSTEM_LABEL_INBOX) && feed_id != QL1S(GMAIL_SYSTEM_LABEL_INBOX)) {
      // This message is in INBOX label too, but this updated feed is not INBOX,
      // we want to leave this message in INBOX and not duplicate it to other feed/label.
      return false;
    }

    if (lbl == QL1S(GMAIL_SYSTEM_LABEL_TRASH) && feed_id != QL1S(GMAIL_SYSTEM_LABEL_TRASH)) {
      // This message is in trash, but this updated feed is not recycle bin, we do not want
      // this message to appear anywhere.
      return false;
    }
  }

  msg.m_author = headers["From"];
  msg.m_title = headers["Subject"];
  msg.m_createdFromFeed = true;
  msg.m_created = TextFactory::parseDateTime(headers["Date"]);

  if (msg.m_title.isEmpty()) {
    msg.m_title = tr("No subject");
  }

  QString backup_contents;
  QJsonArray parts = json["payload"].toObject()["parts"].toArray();

  if (parts.isEmpty()) {
    parts.append(json["payload"].toObject());
  }

  for (const QJsonValue& part : parts) {
    QJsonObject part_obj = part.toObject();
    QJsonObject body = part_obj["body"].toObject();
    QString filename = part_obj["filename"].toString();

    if (filename.isEmpty() && body.contains(QL1S("data"))) {
      // We have textual data of e-mail.
      // We check if it is HTML.
      if (msg.m_contents.isEmpty()) {
        if (part_obj["mimeType"].toString().contains(QL1S("text/html"))) {
          msg.m_contents = QByteArray::fromBase64(body["data"].toString().toUtf8(), QByteArray::Base64Option::Base64UrlEncoding);
        }
        else {
          backup_contents = QByteArray::fromBase64(body["data"].toString().toUtf8(), QByteArray::Base64Option::Base64UrlEncoding);
        }
      }
    }
    else if (!filename.isEmpty()) {
      // We have attachment.
      msg.m_enclosures.append(Enclosure(filename +
                                        QL1S(GMAIL_ATTACHMENT_SEP) + msg.m_customId +
                                        QL1S(GMAIL_ATTACHMENT_SEP) + body["attachmentId"].toString(),
                                        filename + QString(" (%1 KB)").arg(QString::number(body["size"].toInt() / 1000.0))));
    }
  }

  if (msg.m_contents.isEmpty() && !backup_contents.isEmpty()) {
    msg.m_contents = backup_contents;
  }

  return true;
}

QMap<QString, QString> GmailNetworkFactory::getMessageMetadata(const QString& msg_id, const QStringList& metadata) {
  QString bearer = m_oauth2->bearer();

  if (bearer.isEmpty()) {
    throw ApplicationException(tr("you are not logged in"));
  }

  QList<QPair<QByteArray, QByteArray>> headers;
  QByteArray output;
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  headers.append(QPair<QByteArray, QByteArray>(QString(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               bearer.toLocal8Bit()));

  QString query = QString("%1/%2?format=metadata&metadataHeaders=%3").arg(GMAIL_API_MSGS_LIST,
                                                                          msg_id,
                                                                          metadata.join(QSL("&metadataHeaders=")));
  NetworkResult res = NetworkFactory::performNetworkOperation(query,
                                                              timeout,
                                                              QByteArray(),
                                                              output,
                                                              QNetworkAccessManager::Operation::GetOperation,
                                                              headers);

  if (res.first == QNetworkReply::NetworkError::NoError) {
    QJsonDocument doc = QJsonDocument::fromJson(output);
    QMap<QString, QString> result;
    auto json_headers = doc.object()["payload"].toObject()["headers"].toArray();

    for (const auto& header : json_headers) {
      QJsonObject obj_header = header.toObject();

      result.insert(obj_header["name"].toString(), obj_header["value"].toString());
    }

    return result;
  }
  else {
    throw ApplicationException(tr("failed to get metadata"));
  }
}

bool GmailNetworkFactory::obtainAndDecodeFullMessages(QList<Message>& messages, const QString& feed_id) {
  QHash<QString, int> msgs;
  int next_message = 0;
  QString bearer = m_oauth2->bearer();

  if (bearer.isEmpty()) {
    return false;
  }

  do {
    auto* multi = new QHttpMultiPart();

    multi->setContentType(QHttpMultiPart::ContentType::MixedType);

    for (int window = next_message + 100; next_message < window && next_message < messages.size(); next_message++ ) {
      Message msg = messages[next_message];
      QHttpPart part;

      part.setRawHeader(HTTP_HEADERS_CONTENT_TYPE, GMAIL_CONTENT_TYPE_HTTP);
      QString full_msg_endpoint = QString("GET /gmail/v1/users/me/messages/%1\r\n").arg(msg.m_customId);

      part.setBody(full_msg_endpoint.toUtf8());
      multi->append(part);
      msgs.insert(msg.m_customId, next_message);
    }

    QList<QPair<QByteArray, QByteArray>> headers;
    QList<HttpResponse> output;
    int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

    headers.append(QPair<QByteArray, QByteArray>(QString(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                                 bearer.toLocal8Bit()));

    NetworkResult res = NetworkFactory::performNetworkOperation(GMAIL_API_BATCH,
                                                                timeout,
                                                                multi,
                                                                output,
                                                                QNetworkAccessManager::Operation::PostOperation,
                                                                headers);

    if (res.first == QNetworkReply::NetworkError::NoError) {
      // We parse each part of HTTP response (it contains HTTP headers and payload with msg full data).
      for (const HttpResponse& part : output) {
        auto xx = part.body();
        QJsonObject msg_doc = QJsonDocument::fromJson(part.body().toUtf8()).object();
        QString msg_id = msg_doc["id"].toString();

        if (msgs.contains(msg_id)) {
          Message& msg = messages[msgs.value(msg_id)];

          if (!fillFullMessage(msg, msg_doc, feed_id)) {
            qWarningNN << "Failed to get full message for custom ID:" << QUOTE_W_SPACE_DOT(msg.m_customId);
          }
        }
      }
    }
    else {
      return false;
    }
  }
  while (next_message < messages.size());

  return true;
}

QList<Message> GmailNetworkFactory::decodeLiteMessages(const QString& messages_json_data,
                                                       const QString& stream_id,
                                                       QString& next_page_token) {
  QList<Message> messages;
  QJsonObject top_object = QJsonDocument::fromJson(messages_json_data.toUtf8()).object();
  QJsonArray json_msgs = top_object["messages"].toArray();

  next_page_token = top_object["nextPageToken"].toString();
  messages.reserve(json_msgs.count());

  for (const QJsonValue& obj : json_msgs) {
    auto message_obj = obj.toObject();
    Message message;

    message.m_customId = message_obj["id"].toString();
    message.m_feedId = stream_id;

    messages.append(message);
  }

  return messages;
}
