// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef INOREADERFEED_H
#define INOREADERFEED_H

#include "services/abstract/feed.h"

class InoreaderServiceRoot;

class InoreaderFeed : public Feed {
  public:
    explicit InoreaderFeed(RootItem* parent = nullptr);

    InoreaderServiceRoot* serviceRoot() const;

    virtual QList<Message> obtainNewMessages(bool* error_during_obtaining);
};

#endif // INOREADERFEED_H
