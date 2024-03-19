// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GMAILNETWORKFACTORY_H
#define GMAILNETWORKFACTORY_H

#include "3rd-party/mimesis/mimesis.hpp"
#include "core/message.h"
#include "services/abstract/feed.h"
#include "services/abstract/rootitem.h"
#include "services/abstract/serviceroot.h"

#include <QNetworkReply>
#include <QObject>

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
    QList<RootItem*> labels(bool only_user_labels, const QNetworkProxy& custom_proxy);
    QMap<QString, QString> getMessageMetadata(const QString& msg_id,
                                              const QStringList& metadata,
                                              const QNetworkProxy& custom_proxy);
    QNetworkRequest requestForAttachment(const QString& email_id, const QString& attachment_id);
    QString sendEmail(Mimesis::Message msg, const QNetworkProxy& custom_proxy, Message* reply_to_message = nullptr);
    QList<Message> messages(const QString& stream_id,
                            const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                            Feed::Status& error,
                            const QNetworkProxy& custom_proxy);
    QNetworkReply::NetworkError batchModify(const QString& label,
                                            const QStringList& custom_ids,
                                            bool assign,
                                            const QNetworkProxy& custom_proxy);
    QStringList list(const QString& stream_id,
                     const QStringList& label_ids,
                     int max_results,
                     bool include_spam,
                     const QString& query,
                     const QNetworkProxy& custom_proxy);
    QVariantHash getProfile(const QNetworkProxy& custom_proxy);

    // Top-level methods.
    QNetworkReply::NetworkError markMessagesRead(RootItem::ReadStatus status,
                                                 const QStringList& custom_ids,
                                                 const QNetworkProxy& custom_proxy);
    QNetworkReply::NetworkError markMessagesStarred(RootItem::Importance importance,
                                                    const QStringList& custom_ids,
                                                    const QNetworkProxy& custom_proxy);

  private slots:
    void onTokensError(const QString& error, const QString& error_description);
    void onAuthFailed();

  private:
    bool fillFullMessage(Message& msg, const QJsonObject& json, const QString& feed_id) const;
    QList<Message> obtainAndDecodeFullMessages(const QStringList& message_ids,
                                               const QString& feed_id,
                                               const QNetworkProxy& custom_proxy) const;
    QStringList decodeLiteMessages(const QString& messages_json_data, QString& next_page_token) const;
    QString sanitizeEmailAuthor(const QString& author) const;

    void initializeOauth();

  private:
    GmailServiceRoot* m_service;
    QString m_username;
    int m_batchSize;
    bool m_downloadOnlyUnreadMessages;
    OAuth2Service* m_oauth2;
};

#endif // GMAILNETWORKFACTORY_H
