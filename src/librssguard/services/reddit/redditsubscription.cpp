// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/reddit/redditsubscription.h"

#include "services/reddit/redditserviceroot.h"

RedditSubscription::RedditSubscription(RootItem* parent) : Feed(parent) {}

RedditServiceRoot* RedditSubscription::serviceRoot() const {
  return qobject_cast<RedditServiceRoot*>(getParentServiceRoot());
}
