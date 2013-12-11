#ifndef FEEDMODELROOTITEM_H
#define FEEDMODELROOTITEM_H

#include "core/feedsmodelitem.h"


// Represents ROOT item of FeedsModel.
// NOTE: This class is derived to add functionality for
// all non-root items of FeedsModel.
class FeedsModelRootItem : public FeedsModelItem {
    friend class FeedsModelNonRootItem;
    friend class FeedsModel;

  public:
    // Constructors and destructors.
    explicit FeedsModelRootItem();
    virtual ~FeedsModelRootItem();

    FeedsModelItem *parent();
    FeedsModelItem *child(int row);
    int childCount() const;
    int columnCount() const;
    int row() const;
    QVariant data(int column, int role) const;

  protected:
    QList<FeedsModelItem*> m_childItems;

};

#endif // FEEDMODELROOTITEM_H
