#ifndef FEEDSMODELFEED_H
#define FEEDSMODELFEED_H

#include "core/feedsmodelnonrootitem.h"


// Represents BASE class for feeds contained in FeedsModel.
// NOTE: This class is derived to create PARTICULAR feed types.
class FeedsModelFeed : public FeedsModelNonRootItem
{
  public:
    // Constructors and destructors.
    explicit FeedsModelFeed(BaseFeedsModelItem *parent_item);
    virtual ~FeedsModelFeed();
};

#endif // FEEDSMODELFEED_H
