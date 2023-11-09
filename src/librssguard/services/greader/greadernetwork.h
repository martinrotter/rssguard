// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GREADERNETWORK_H
#define GREADERNETWORK_H

#include <QObject>

#include "network-web/networkfactory.h"
#include "services/abstract/feed.h"
#include "services/greader/greaderserviceroot.h"

class OAuth2Service;

class GreaderNetwork : public QObject {
    Q_OBJECT

  public:
    enum class Operations {
      ClientLogin,
      TagList,
      SubscriptionList,
      StreamContents,
      EditTag,
      Token,
      UserInfo,
      ItemIds,
      ItemContents,
      SubscriptionExport,
      SubscriptionImport
    };

    explicit GreaderNetwork(QObject* parent = nullptr);

    // Convenience methods.
    QNetworkReply::NetworkError markMessagesRead(RootItem::ReadStatus status,
                                                 const QStringList& msg_custom_ids,
                                                 const QNetworkProxy& proxy);
    QNetworkReply::NetworkError markMessagesStarred(RootItem::Importance importance,
                                                    const QStringList& msg_custom_ids,
                                                    const QNetworkProxy& proxy);

    void prepareFeedFetching(GreaderServiceRoot* root,
                             const QList<Feed*>& feeds,
                             const QHash<QString, QHash<ServiceRoot::BagOfMessages, QStringList>>& stated_messages,
                             const QHash<QString, QStringList>& tagged_messages,
                             const QNetworkProxy& proxy);

    QList<Message> getMessagesIntelligently(ServiceRoot* root,
                                            const QString& stream_id,
                                            const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                            const QHash<QString, QStringList>& tagged_messages,
                                            const QNetworkProxy& proxy);

    RootItem* categoriesFeedsLabelsTree(bool obtain_icons, const QNetworkProxy& proxy);

    void clearCredentials();

    void clearPrefetchedMessages();

    GreaderServiceRoot::Service service() const;
    void setService(GreaderServiceRoot::Service service);

    QString username() const;
    void setUsername(const QString& username);

    QString password() const;
    void setPassword(const QString& password);

    QString baseUrl() const;
    void setBaseUrl(const QString& base_url);

    int batchSize() const;
    void setBatchSize(int batch_size);

    bool downloadOnlyUnreadMessages() const;
    void setDownloadOnlyUnreadMessages(bool download_only_unread);

    bool intelligentSynchronization() const;
    void setIntelligentSynchronization(bool intelligent_synchronization);

    void setRoot(GreaderServiceRoot* root);

    OAuth2Service* oauth() const;
    void setOauth(OAuth2Service* oauth);

    // API methods.
    void subscriptionImport(const QByteArray& opml_data, const QNetworkProxy& proxy);
    QByteArray subscriptionExport(const QNetworkProxy& proxy);
    QNetworkReply::NetworkError editLabels(const QString& state,
                                           bool assign,
                                           const QStringList& msg_custom_ids,
                                           const QNetworkProxy& proxy);
    QVariantHash userInfo(const QNetworkProxy& proxy);
    QStringList itemIds(const QString& stream_id,
                        bool unread_only,
                        const QNetworkProxy& proxy,
                        int max_count = -1,
                        QDate newer_than = {});
    QList<Message> itemContents(ServiceRoot* root, const QList<QString>& stream_ids, const QNetworkProxy& proxy);
    QList<Message> streamContents(ServiceRoot* root, const QString& stream_id, const QNetworkProxy& proxy);
    QNetworkReply::NetworkError clientLogin(const QNetworkProxy& proxy);

    QDate newerThanFilter() const;
    void setNewerThanFilter(const QDate& newer_than);

  private slots:
    void onTokensError(const QString& error, const QString& error_description);
    void onAuthFailed();

  private:
    QPair<QByteArray, QByteArray> authHeader() const;
    QString tokenParameter() const;

    // Make sure we are logged in and if we are not, return error.
    bool ensureLogin(const QNetworkProxy& proxy, QNetworkReply::NetworkError* output = nullptr);

    QString convertLongStreamIdToShortStreamId(const QString& stream_id) const;
    QString convertShortStreamIdToLongStreamId(const QString& stream_id) const;
    QString simplifyStreamId(const QString& stream_id) const;

    QStringList decodeItemIds(const QString& stream_json_data, QString& continuation);
    QList<Message> decodeStreamContents(ServiceRoot* root,
                                        const QString& stream_json_data,
                                        const QString& stream_id,
                                        QString& continuation);
    RootItem* decodeTagsSubscriptions(const QString& categories,
                                      const QString& feeds,
                                      bool obtain_icons,
                                      const QNetworkProxy& proxy);

    QString sanitizedBaseUrl() const;
    QString generateFullUrl(Operations operation) const;

    void initializeOauth();

  private:
    GreaderServiceRoot* m_root;
    GreaderServiceRoot::Service m_service;
    QString m_username;
    QString m_password;
    QString m_baseUrl;
    int m_batchSize;
    bool m_downloadOnlyUnreadMessages;
    QString m_authSid;
    QString m_authAuth;
    QString m_authToken;
    QList<Message> m_prefetchedMessages;
    QMutex m_mutexPrefetchedMessages;
    bool m_performGlobalFetching;
    bool m_intelligentSynchronization;
    QDate m_newerThanFilter;
    OAuth2Service* m_oauth;
};

#endif // GREADERNETWORK_H
