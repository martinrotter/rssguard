#ifndef BASEFEEDSMODELITEM_H
#define BASEFEEDSMODELITEM_H

#include <QList>
#include <QIcon>


// Base class for all items contained in FeedsModel.
class BaseFeedsModelItem {
  public:
    // Constructors and destructors.
    explicit BaseFeedsModelItem();
    virtual ~BaseFeedsModelItem();

    // Returns parent item of this item.
    // NOTE: Model ROOT item has NULL parent.
    virtual BaseFeedsModelItem *parent() = 0;
    virtual int childCount() const = 0;
    virtual int columnCount() const = 0;

  protected:
    QIcon m_icon;

};

#endif // BASEFEEDSMODELITEM_H
