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
    void setOauth(OAuth2Service* oauth);

    QString username() const;
    void setUsername(const QString& username);

    int batchSize() const;
    void setBatchSize(int batch_size);

    bool downloadOnlyUnreadMessages() const;
    void setDownloadOnlyUnreadMessages(bool download_only_unread_messages);

    // API methods.
    QString sendEmail(Mimesis::Message msg, const QNetworkProxy& custom_proxy, Message* reply_to_message = nullptr);
    Downloader* downloadAttachment(const QString& msg_id, const QString& attachment_id, const QNetworkProxy& custom_proxy);
    QList<Message> messages(const QString& stream_id, Feed::Status& error, const QNetworkProxy& custom_proxy);
    QNetworkReply::NetworkError markMessagesRead(RootItem::ReadStatus status,
                                                 const QStringList& custom_ids,
                                                 const QNetworkProxy& custom_proxy);
    QNetworkReply::NetworkError markMessagesStarred(RootItem::Importance importance,
                                                    const QStringList& custom_ids,
                                                    const QNetworkProxy& custom_proxy);
    QVariantHash getProfile(const QNetworkProxy& custom_proxy);

  private slots:
    void onTokensError(const QString& error, const QString& error_description);
    void onAuthFailed();

  private:
    bool fillFullMessage(Message& msg, const QJsonObject& json, const QString& feed_id);
    QMap<QString, QString> getMessageMetadata(const QString& msg_id,
                                              const QStringList& metadata,
                                              const QNetworkProxy& custom_proxy);
    bool obtainAndDecodeFullMessages(QList<Message>& lite_messages,
                                     const QString& feed_id,
                                     const QNetworkProxy& custom_proxy);
    QList<Message> decodeLiteMessages(const QString& messages_json_data, const QString& stream_id, QString& next_page_token);

    void initializeOauth();

  private:
    GmailServiceRoot* m_service;
    QString m_username;
    int m_batchSize;
    bool m_downloadOnlyUnreadMessages;
    OAuth2Service* m_oauth2;
};

#endif // GMAILNETWORKFACTORY_H
