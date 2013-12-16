#ifndef FEEDMODELROOTITEM_H
#define FEEDMODELROOTITEM_H

#include <QIcon>


// Represents ROOT item of FeedsModel.
// NOTE: This class is derived to add functionality for
// all other non-root items of FeedsModel.
class FeedsModelRootItem {
  public:
    // Describes the kind of the item.
    enum Kind {
      RootItem  = 1001,
      Feed      = 1002,
      Category  = 1003
    };

    // Constructors and destructors.
    explicit FeedsModelRootItem(FeedsModelRootItem *parent_item = NULL);
    virtual ~FeedsModelRootItem();

    // Basic operations.
    virtual FeedsModelRootItem *parent();
    virtual void setParent(FeedsModelRootItem *parent_item);
    virtual FeedsModelRootItem *child(int row);
    virtual void appendChild(FeedsModelRootItem *child);
    virtual int childCount() const;
    virtual int columnCount() const;
    virtual int row() const;
    virtual QVariant data(int column, int role) const;

    // Each item offers "counts" of messages.
    virtual int countOfUnreadMessages() const;
    virtual int countOfAllMessages() const;

    // Each item can be "updated".
    // NOTE: This method is used in the "update worker".
    // For example, it can fetch new messages from a remote destination
    // and store them in a local database and so on.
    virtual void update();

    virtual Kind kind() const;

    // Each item has icon.
    void setIcon(const QIcon &icon);

    // Each item has some kind of id.
    int id() const;
    void setId(int id);

    // Each item has its title.
    // NOTE: This is note entirely true for the root item.
    QString title() const;
    void setTitle(const QString &title);

    // Acess to children.
    QList<FeedsModelRootItem *> childItems() const;

  protected:
    Kind m_kind;
    QString m_title;
    int m_id;
    QIcon m_icon;
    QList<FeedsModelRootItem*> m_childItems;
    FeedsModelRootItem *m_parentItem;
};

#endif // FEEDMODELROOTITEM_H
