#ifndef BASEFEEDSMODELITEM_H
#define BASEFEEDSMODELITEM_H

#include <QList>
#include <QIcon>


// Base class for all items contained in FeedsModel.
class FeedsModelItem {
  public:
    // Constructors and destructors.
    explicit FeedsModelItem();
    virtual ~FeedsModelItem();

    // Returns parent item of this item.
    // NOTE: Model ROOT item has NULL parent.
    virtual FeedsModelItem *parent() = 0;
    virtual int childCount() const = 0;
    virtual int columnCount() const = 0;
    virtual FeedsModelItem *child(int row) = 0;
    virtual int row() const = 0;

  protected:
    QIcon m_icon;

};

#endif // BASEFEEDSMODELITEM_H
