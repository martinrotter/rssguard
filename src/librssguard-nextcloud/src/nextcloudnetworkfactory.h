// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEXTCLOUDNETWORKFACTORY_H
#define NEXTCLOUDNETWORKFACTORY_H

#include <librssguard/core/message.h>
#include <librssguard/network-web/networkfactory.h>
#include <librssguard/services/abstract/rootitem.h>

#include <QDateTime>
#include <QIcon>
#include <QJsonObject>
#include <QNetworkReply>
#include <QString>

class NextcloudNetworkFactory {
  public:
    explicit NextcloudNetworkFactory();
    virtual ~NextcloudNetworkFactory();

    QString url() const;
    void setUrl(const QString& url);

    bool forceServerSideUpdate() const;
    void setForceServerSideUpdate(bool force_update);

    QString authUsername() const;
    void setAuthUsername(const QString& auth_username);

    QString authPassword() const;
    void setAuthPassword(const QString& auth_password);

    // Gets/sets the amount of messages to obtain during single feed update.
    int batchSize() const;
    void setBatchSize(int batch_size);

    bool downloadOnlyUnreadMessages() const;
    void setDownloadOnlyUnreadMessages(bool dowload_only_unread_messages);

    // Operations.

    // Get version info.
    QString status(const QNetworkProxy& custom_proxy);

    // Get feeds & categories (used for sync-in).
    RootItem* feedsCategories(const QNetworkProxy& custom_proxy);

    // Feed operations.
    void deleteFeed(const QString& feed_id, const QNetworkProxy& custom_proxy);
    void createFeed(const QString& url, int parent_id, const QNetworkProxy& custom_proxy);
    void obtainIcons(const QList<Feed*>& feeds, const QNetworkProxy& custom_proxy);

    // Get messages for given feed.
    QList<Message> getMessages(int feed_id, const QNetworkProxy& custom_proxy);

    // Misc methods.
    QNetworkReply::NetworkError triggerFeedUpdate(int feed_id, const QNetworkProxy& custom_proxy);

    NetworkResult markMessagesRead(RootItem::ReadStatus status,
                                   const QStringList& custom_ids,
                                   const QNetworkProxy& custom_proxy);

    NetworkResult markMessagesStarred(RootItem::Importance importance,
                                      const QStringList& custom_ids,
                                      const QNetworkProxy& custom_proxy);

  private:
    QString m_url;
    QString m_fixedUrl;
    bool m_downloadOnlyUnreadMessages;
    bool m_forceServerSideUpdate;
    QString m_authUsername;
    QString m_authPassword;
    int m_batchSize;

    // Endpoints.
    QString m_urlUser;
    QString m_urlStatus;
    QString m_urlFolders;
    QString m_urlFeeds;
    QString m_urlMessages;
    QString m_urlFeedsUpdate;
    QString m_urlDeleteFeed;
    QString m_urlRenameFeed;
    QString m_urlFavIcon;
};

#endif // NEXTCLOUDNETWORKFACTORY_H
