// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef OWNCLOUDFEED_H
#define OWNCLOUDFEED_H

#include "services/abstract/feed.h"

class OwnCloudServiceRoot;

class OwnCloudFeed : public Feed {
    Q_OBJECT

  public:
    explicit OwnCloudFeed(RootItem* parent = nullptr);

    virtual bool canBeDeleted() const;
    virtual bool deleteItem();

  private:
    bool removeItself();
    OwnCloudServiceRoot* serviceRoot() const;
};

#endif // OWNCLOUDFEED_H
