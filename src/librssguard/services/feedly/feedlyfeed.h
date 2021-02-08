// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDLYFEED_H
#define FEEDLYFEED_H

#include "services/abstract/feed.h"

class FeedlyServiceRoot;

class FeedlyFeed : public Feed {
  public:
    explicit FeedlyFeed(RootItem* parent = nullptr);
    explicit FeedlyFeed(const QSqlRecord& record);

    FeedlyServiceRoot* serviceRoot() const;
    QList<Message> obtainNewMessages(bool* error_during_obtaining);
};

#endif // FEEDLYFEED_H
