// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEXTCLOUDFEED_H
#define NEXTCLOUDFEED_H

#include <librssguard/services/abstract/feed.h>

class NextcloudServiceRoot;

class NextcloudFeed : public Feed {
    Q_OBJECT

  public:
    explicit NextcloudFeed(RootItem* parent = nullptr);

    virtual bool canBeDeleted() const;
    virtual bool deleteItem();

  private:
    bool removeItself();
    NextcloudServiceRoot* serviceRoot() const;
};

#endif // NEXTCLOUDFEED_H
