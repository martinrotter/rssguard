// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef REDDITSUBSCRIPTION_H
#define REDDITSUBSCRIPTION_H

#include "services/abstract/feed.h"

class RedditServiceRoot;

class RedditSubscription : public Feed {
    Q_OBJECT

  public:
    explicit RedditSubscription(RootItem* parent = nullptr);

    RedditServiceRoot* serviceRoot() const;

    QString prefixedName() const;
    void setPrefixedName(const QString& prefixed_name);

    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);

  private:
    QString m_prefixedName;
};

#endif // REDDITSUBSCRIPTION_H
