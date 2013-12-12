#ifndef FEEDMODELROOTITEM_H
#define FEEDMODELROOTITEM_H

#include <QIcon>


// Represents ROOT item of FeedsModel.
// NOTE: This class is derived to add functionality for
// all other non-root items of FeedsModel.
class FeedsModelRootItem {
    friend class FeedsModelNonRootItem;
    friend class FeedsModel;

  public:
    // Constructors and destructors.
    explicit FeedsModelRootItem(FeedsModelRootItem *parent_item);
    virtual ~FeedsModelRootItem();

    virtual FeedsModelRootItem *parent();
    virtual FeedsModelRootItem *child(int row);
    virtual void appendChild(FeedsModelRootItem *child);
    virtual int childCount() const;
    virtual int columnCount() const;
    virtual int row() const;
    virtual QVariant data(int column, int role) const;

  protected:
    QIcon m_icon;
    QList<FeedsModelRootItem*> m_childItems;
    FeedsModelRootItem *m_parentItem;

};

#endif // FEEDMODELROOTITEM_H
