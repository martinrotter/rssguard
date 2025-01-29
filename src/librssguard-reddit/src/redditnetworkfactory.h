// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef REDDITNETWORKFACTORY_H
#define REDDITNETWORKFACTORY_H

#include "core/message.h"
#include "services/abstract/feed.h"
#include "services/abstract/rootitem.h"
#include "src/3rd-party/mimesis/mimesis.hpp"

#include <QNetworkReply>
#include <QObject>

class RootItem;
class RedditServiceRoot;
class OAuth2Service;

struct Subreddit {};

class RedditNetworkFactory : public QObject {
    Q_OBJECT

  public:
    explicit RedditNetworkFactory(QObject* parent = nullptr);

    void setService(RedditServiceRoot* service);

    OAuth2Service* oauth() const;
    void setOauth(OAuth2Service* oauth);

    QString username() const;
    void setUsername(const QString& username);

    int batchSize() const;
    void setBatchSize(int batch_size);

    bool downloadOnlyUnreadMessages() const;
    void setDownloadOnlyUnreadMessages(bool download_only_unread_messages);

    // API methods.
    QVariantHash me(const QNetworkProxy& custom_proxy);
    QList<Feed*> subreddits(const QNetworkProxy& custom_proxy);
    QList<Message> hot(const QString& sub_name, const QNetworkProxy& custom_proxy);

  private slots:
    void onTokensError(const QString& error, const QString& error_description);
    void onAuthFailed();

  private:
    void initializeOauth();

  private:
    RedditServiceRoot* m_service;
    QString m_username;
    int m_batchSize;
    bool m_downloadOnlyUnreadMessages;
    OAuth2Service* m_oauth2;
};

#endif // REDDITNETWORKFACTORY_H
