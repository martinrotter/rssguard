// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TTRSSFEED_H
#define TTRSSFEED_H

#include "services/abstract/feed.h"

class TtRssServiceRoot;

class TtRssFeed : public Feed {
  Q_OBJECT

  public:
    explicit TtRssFeed(RootItem* parent = nullptr);

    TtRssServiceRoot* serviceRoot() const;

    virtual bool canBeDeleted() const;
    virtual bool deleteViaGui();
    virtual QList<Message> obtainNewMessages(bool* error_during_obtaining);

  private:
    bool removeItself();
};

#endif // TTRSSFEED_H
