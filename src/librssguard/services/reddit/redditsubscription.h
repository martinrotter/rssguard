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
};

#endif // REDDITSUBSCRIPTION_H
