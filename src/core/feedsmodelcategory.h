#ifndef FEEDSMODELCLASSICCATEGORY_H
#define FEEDSMODELCLASSICCATEGORY_H

#include "core/feedsmodelrootitem.h"


// Base class for all categories contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR category types.
class FeedsModelCategory : public FeedsModelRootItem {
  public:
    // Constructors and destructors
    explicit FeedsModelCategory(FeedsModelRootItem *parent_item);
    virtual ~FeedsModelCategory();

};

#endif // FEEDSMODELCLASSICCATEGORY_H
