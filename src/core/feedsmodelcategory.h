#ifndef FEEDSMODELCLASSICCATEGORY_H
#define FEEDSMODELCLASSICCATEGORY_H

#include "core/feedsmodelnonrootitem.h"


// Base class for all categories contained in FeedsModel.
// NOTE: This class is derived to create PARTICULAR category types.
class FeedsModelCategory : public FeedsModelNonRootItem {
  public:
    // Constructors and destructors
    explicit FeedsModelCategory(FeedsModelItem *parent_item);
    virtual ~FeedsModelCategory();

};

#endif // FEEDSMODELCLASSICCATEGORY_H
