#ifndef FEEDSMODELFEED_H
#define FEEDSMODELFEED_H

#include "core/feedsmodelnonrootitem.h"


// Represents BASE class for feeds contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR feed types.
class FeedsModelFeed : public FeedsModelNonRootItem
{
  public:
    // Constructors and destructors.
    explicit FeedsModelFeed(FeedsModelRootItem *parent_item);
    virtual ~FeedsModelFeed();

    int childCount() const;

};

#endif // FEEDSMODELFEED_H
