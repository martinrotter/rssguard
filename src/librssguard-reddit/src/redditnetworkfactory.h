// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef REDDITNETWORKFACTORY_H
#define REDDITNETWORKFACTORY_H

#include "core/message.h"
#include "services/abstract/feed.h"
#include "services/abstract/rootitem.h"

#include <QNetworkReply>
#include <QObject>

class RootItem;
class RedditServiceRoot;
class OAuth2Service;

struct RedditComment {
    QString id;
    QString parentId;
    QString author;
    QString bodyHtml;
    QString bodyText;
    QDateTime created;
    int score = 0;

    QList<RedditComment> replies;
};

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

    QJsonArray fetchMoreChildren(const QString& link_fullname,
                                 const QStringList& children_ids,
                                 const QNetworkProxy& proxy);
    QList<RedditComment> parseCommentTree(const QJsonArray& children,
                                          const QString& link_fullname,
                                          const QNetworkProxy& proxy);
    QList<RedditComment> commentsTree(const QString& subreddit, const QString& post_id, const QNetworkProxy& proxy);
    QString commentsTreeToHtml(const QList<RedditComment>& comments,
                               const QString& post_title,
                               const QString& post_url);
    QString cleanScOnOff(const QString& html);
    void renderCommentHtml(const RedditComment& c, QString& html, int depth);
    RedditComment commentFromJson(const QJsonObject& data);

  private:
    RedditServiceRoot* m_service;
    QString m_username;
    int m_batchSize;
    bool m_downloadOnlyUnreadMessages;
    OAuth2Service* m_oauth2;
};

#endif // REDDITNETWORKFACTORY_H
