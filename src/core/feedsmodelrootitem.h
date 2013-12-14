#ifndef FEEDMODELROOTITEM_H
#define FEEDMODELROOTITEM_H

#include <QIcon>


// Represents ROOT item of FeedsModel.
// NOTE: This class is derived to add functionality for
// all other non-root items of FeedsModel.
class FeedsModelRootItem {
    friend class FeedsModel;

  public:
    // Constructors and destructors.
    explicit FeedsModelRootItem(FeedsModelRootItem *parent_item = NULL);
    virtual ~FeedsModelRootItem();

    virtual void setParent(FeedsModelRootItem *parent_item);
    virtual FeedsModelRootItem *parent();
    virtual FeedsModelRootItem *child(int row);
    virtual void appendChild(FeedsModelRootItem *child);
    virtual int childCount() const;
    virtual int columnCount() const;
    virtual int row() const;
    virtual QVariant data(int column, int role) const;

    virtual int countOfUnreadMessages() const;
    virtual int countOfAllMessages() const;

    void setIcon(const QIcon &icon);

    int id() const;
    void setId(int id);

  protected:
    int m_id;
    QIcon m_icon;
    QList<FeedsModelRootItem*> m_childItems;
    FeedsModelRootItem *m_parentItem;

};

#endif // FEEDMODELROOTITEM_H
