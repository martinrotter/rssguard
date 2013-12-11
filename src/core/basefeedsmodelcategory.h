#ifndef FEEDSMODELCLASSICCATEGORY_H
#define FEEDSMODELCLASSICCATEGORY_H

#include "core/feedsmodelnonrootitem.h"


// Base class for all categories contained in FeedsModel.
// NOTE: This class is derived to create PARTICULAR category types.
class BaseFeedsModelCategory : public FeedsModelNonRootItem
{
  public:
    // Constructors and destructors
    explicit BaseFeedsModelCategory(BaseFeedsModelItem *parent_item);
    virtual ~BaseFeedsModelCategory();

};

#endif // FEEDSMODELCLASSICCATEGORY_H
