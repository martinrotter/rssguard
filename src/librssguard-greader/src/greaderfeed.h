// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GREADERFEED_H
#define GREADERFEED_H

#include <librssguard/services/abstract/feed.h>

class GreaderServiceRoot;

class GreaderFeed : public Feed {
    Q_OBJECT

    friend class FormGreaderFeedDetails;

  public:
    explicit GreaderFeed(RootItem* parent = nullptr);

    virtual bool canBeDeleted() const;
    virtual void deleteItem();

  private:
    GreaderServiceRoot* serviceRoot() const;
    bool removeItself();
};

#endif // TTRSSFEED_H
