// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef OWNCLOUDNETWORKFACTORY_H
#define OWNCLOUDNETWORKFACTORY_H

#include "core/message.h"
#include "services/abstract/rootitem.h"

#include <QDateTime>
#include <QIcon>
#include <QJsonObject>
#include <QNetworkReply>
#include <QString>

class OwnCloudResponse {
  public:
    explicit OwnCloudResponse(const QString& raw_content = QString());
    virtual ~OwnCloudResponse();

    bool isLoaded() const;
    QString toString() const;

  protected:
    QJsonObject m_rawContent;
    bool m_emptyString;
};

class OwnCloudGetMessagesResponse : public OwnCloudResponse {
  public:
    explicit OwnCloudGetMessagesResponse(const QString& raw_content = QString());
    virtual ~OwnCloudGetMessagesResponse();

    QList<Message> messages() const;
};

class OwnCloudStatusResponse : public OwnCloudResponse {
  public:
    explicit OwnCloudStatusResponse(const QString& raw_content = QString());
    virtual ~OwnCloudStatusResponse();

    QString version() const;
    bool misconfiguredCron() const;
};

class RootItem;

class OwnCloudGetFeedsCategoriesResponse {
  public:
    explicit OwnCloudGetFeedsCategoriesResponse(QString raw_categories = QString(), QString raw_feeds = QString());
    virtual ~OwnCloudGetFeedsCategoriesResponse();

    // Returns tree of feeds/categories.
    // Top-level root of the tree is not needed here.
    // Returned items do not have primary IDs assigned.
    RootItem* feedsCategories(bool obtain_icons) const;

  private:
    QString m_contentCategories;
    QString m_contentFeeds;
};

class OwnCloudNetworkFactory {
  public:
    explicit OwnCloudNetworkFactory();
    virtual ~OwnCloudNetworkFactory();

    QString url() const;
    void setUrl(const QString& url);

    bool forceServerSideUpdate() const;
    void setForceServerSideUpdate(bool force_update);

    QString authUsername() const;
    void setAuthUsername(const QString& auth_username);

    QString authPassword() const;
    void setAuthPassword(const QString& auth_password);

    QNetworkReply::NetworkError lastError() const;

    // Operations.

    // Get version info.
    OwnCloudStatusResponse status();

    // Get feeds & categories (used for sync-in).
    OwnCloudGetFeedsCategoriesResponse feedsCategories();

    // Feed operations.
    bool deleteFeed(const QString& feed_id);
    bool createFeed(const QString& url, int parent_id);
    bool renameFeed(const QString& new_name, const QString& custom_feed_id);

    // Get messages for given feed.
    OwnCloudGetMessagesResponse getMessages(int feed_id);

    // Misc methods.
    QNetworkReply::NetworkError triggerFeedUpdate(int feed_id);
    void markMessagesRead(RootItem::ReadStatus status, const QStringList& custom_ids, bool async = true);
    void markMessagesStarred(RootItem::Importance importance, const QStringList& feed_ids,
                             const QStringList& guid_hashes, bool async = true);

    // Gets/sets the amount of messages to obtain during single feed update.
    int batchSize() const;
    void setBatchSize(int batch_size);

    bool downloadOnlyUnreadMessages() const;
    void setDownloadOnlyUnreadMessages(bool dowload_only_unread_messages);

  private:
    QString m_url;
    QString m_fixedUrl;
    bool m_downloadOnlyUnreadMessages;
    bool m_forceServerSideUpdate;
    QString m_authUsername;
    QString m_authPassword;

    QNetworkReply::NetworkError m_lastError;
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
};

#endif // OWNCLOUDNETWORKFACTORY_H
