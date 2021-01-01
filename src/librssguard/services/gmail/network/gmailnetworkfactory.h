// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GMAILNETWORKFACTORY_H
#define GMAILNETWORKFACTORY_H

#include <QObject>

#include "core/message.h"

#include "3rd-party/mimesis/mimesis.hpp"
#include "services/abstract/feed.h"
#include "services/abstract/rootitem.h"

#include <QNetworkReply>

class RootItem;
class GmailServiceRoot;
class OAuth2Service;
class Downloader;

class GmailNetworkFactory : public QObject {
  Q_OBJECT

  public:
    explicit GmailNetworkFactory(QObject* parent = nullptr);

    void setService(GmailServiceRoot* service);

    OAuth2Service* oauth() const;

    QString username() const;
    void setUsername(const QString& username);

    // Gets/sets the amount of messages to obtain during single feed update.
    int batchSize() const;
    void setBatchSize(int batch_size);

    // Sends e-mail, returns its ID.
    QString sendEmail(Mimesis::Message msg, Message* reply_to_message = nullptr);

    Downloader* downloadAttachment(const QString& msg_id, const QString& attachment_id);

    QList<Message> messages(const QString& stream_id, Feed::Status& error);
    QNetworkReply::NetworkError markMessagesRead(RootItem::ReadStatus status, QStringList custom_ids);
    QNetworkReply::NetworkError markMessagesStarred(RootItem::Importance importance, const QStringList& custom_ids);

  private slots:
    void onTokensError(const QString& error, const QString& error_description);
    void onAuthFailed();

  private:
    bool fillFullMessage(Message& msg, const QJsonObject& json, const QString& feed_id);
    QMap<QString, QString> getMessageMetadata(const QString& msg_id, const QStringList& metadata);
    bool obtainAndDecodeFullMessages(QList<Message>& lite_messages, const QString& feed_id);
    QList<Message> decodeLiteMessages(const QString& messages_json_data, const QString& stream_id, QString& next_page_token);

    //RootItem* decodeFeedCategoriesData(const QString& categories);

    void initializeOauth();

  private:
    GmailServiceRoot* m_service;
    QString m_username;
    int m_batchSize;
    OAuth2Service* m_oauth2;
};

#endif // GMAILNETWORKFACTORY_H
