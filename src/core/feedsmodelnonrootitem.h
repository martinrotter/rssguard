#ifndef FEEDSMODELNONROOTITEM_H
#define FEEDSMODELNONROOTITEM_H

#include "core/feedsmodelrootitem.h"


// Base class for non-root items of FeedsModel.
// NOTE: This class add member for pointer to parent item (which is not needed
// for root item).
class FeedsModelNonRootItem : public FeedsModelRootItem {
  public:
    // Constructors and destructors.
    explicit FeedsModelNonRootItem(BaseFeedsModelItem *parent_item);
    virtual ~FeedsModelNonRootItem();

    BaseFeedsModelItem *parent();

  protected:
    BaseFeedsModelItem *m_parentItem;
};

#endif // FEEDSMODELNONROOTITEM_H
