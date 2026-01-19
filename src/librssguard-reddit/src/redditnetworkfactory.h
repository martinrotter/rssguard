// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef REDDITNETWORKFACTORY_H
#define REDDITNETWORKFACTORY_H

#include "core/message.h"
#include "services/abstract/feed.h"
#include "services/abstract/rootitem.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
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

    static QJsonArray toJson(const QList<RedditComment>& c) {
      QJsonArray arr;

      for (const auto& comment : c) {
        arr.append(toJson(comment));
      }

      return arr;
    }

    static QList<RedditComment> fromJson(const QJsonArray& arr) {
      QList<RedditComment> comments;
      comments.reserve(arr.size());

      for (const auto& c : arr) {
        if (!c.isObject()) {
          continue;
        }

        comments.append(fromJson(c.toObject()));
      }

      return comments;
    }

    static QJsonObject toJson(const RedditComment& c) {
      QJsonObject o;

      o["id"] = c.id;
      o["parentId"] = c.parentId;
      o["author"] = c.author;
      o["bodyHtml"] = c.bodyHtml;
      o["bodyText"] = c.bodyText;
      o["created"] = c.created.toUTC().toString(Qt::ISODateWithMs);
      o["score"] = c.score;

      QJsonArray arr;

      for (const auto& reply : c.replies) {
        arr.append(toJson(reply));
      }

      o["replies"] = arr;

      return o;
    }

    static RedditComment fromJson(const QJsonObject& o) {
      RedditComment c;

      c.id = o.value("id").toString();
      c.parentId = o.value("parentId").toString();
      c.author = o.value("author").toString();
      c.bodyHtml = o.value("bodyHtml").toString();
      c.bodyText = o.value("bodyText").toString();
      c.score = o.value("score").toInt();

      // parse datetime safely
      const QString createdStr = o.value("created").toString();

      c.created = QDateTime::fromString(createdStr, Qt::ISODateWithMs);

      if (!c.created.isValid()) {
        c.created = QDateTime::fromString(createdStr, Qt::ISODate);
      }

      // replies recursion
      const QJsonArray arr = o.value("replies").toArray();
      c.replies.reserve(arr.size());

      for (const auto& v : arr) {
        if (!v.isObject()) {
          continue;
        }

        c.replies.append(fromJson(v.toObject()));
      }

      return c;
    }
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

    QString prefixedSubredditToBare(const QString& subr) const;
    QList<RedditComment> commentsTree(const QString& subreddit,
                                      const QString& post_id,
                                      const QNetworkProxy& proxy) const;
    QString commentsTreeToHtml(const QList<RedditComment>& comments,
                               const QString& post_title,
                               const QString& post_url) const;

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
                                 const QNetworkProxy& proxy) const;
    QList<RedditComment> parseCommentTree(const QJsonArray& children,
                                          const QString& link_fullname,
                                          const QNetworkProxy& proxy) const;
    QString cleanScOnOff(const QString& html) const;
    void renderCommentHtml(const RedditComment& c, QString& html, int depth) const;
    RedditComment commentFromJson(const QJsonObject& data) const;

  private:
    RedditServiceRoot* m_service;
    QString m_username;
    int m_batchSize;
    bool m_downloadOnlyUnreadMessages;
    OAuth2Service* m_oauth2;
};

#endif // REDDITNETWORKFACTORY_H
