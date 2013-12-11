#ifndef FEEDMODELROOTITEM_H
#define FEEDMODELROOTITEM_H

#include "core/basefeedsmodelitem.h"


// Represents ROOT item of FeedsModel.
// NOTE: This class is derived to add functionality for
// all non-root items of FeedsModel.
class FeedsModelRootItem : public BaseFeedsModelItem {
  public:
    // Constructors and destructors.
    explicit FeedsModelRootItem();
    virtual ~FeedsModelRootItem();

    BaseFeedsModelItem *parent();
    int childCount() const;
    int columnCount() const;

  protected:
    QList<BaseFeedsModelItem*> m_childItems;

};

#endif // FEEDMODELROOTITEM_H
