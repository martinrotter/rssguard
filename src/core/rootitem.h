// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#ifndef ROOTITEM_H
#define ROOTITEM_H

#include <QIcon>
#include <QDateTime>
#include <QFont>

class Category;
class Feed;

namespace RootItemKind {
  // Describes the kind of the item.
  enum Kind {
    Root        = 1001,
    Bin         = 1002,
    Feed        = 1003,
    Category    = 1004,
    ServiceRoot = 1005
  };
}

// Represents ROOT item of FeedsModel.
// NOTE: This class is derived to add functionality for
// all other non-root items of FeedsModel.
class RootItem {
  public:
    // Constructors and destructors.
    explicit RootItem(RootItem *parent_item = NULL);
    virtual ~RootItem();

    // Basic operations.
    inline virtual RootItem *parent() const {
      return m_parentItem;
    }

    inline virtual void setParent(RootItem *parent_item) {
      m_parentItem = parent_item;
    }

    inline virtual RootItem *child(int row) {
      return m_childItems.value(row);
    }

    inline virtual int childCount() const {
      return m_childItems.size();
    }

    inline virtual void appendChild(RootItem *child) {
      m_childItems.append(child);
      child->setParent(this);
    }

    // TODO: pracovat s těmito věcmi
    virtual bool canBeEdited() {
      return false;
    }

    virtual bool editViaDialog() {
      return false;
    }

    virtual bool canBeDeleted() {
      return false;
    }

    virtual bool deleteViaGui() {
      return false;
    }

    virtual int row() const;
    virtual QVariant data(int column, int role) const;

    // Each item offers "counts" of messages.
    // Returns counts of messages of all child items summed up.
    virtual int countOfUnreadMessages() const;
    virtual int countOfAllMessages() const;

    // This method is used to permanently
    // "remove" (or "unregister") this item.
    // This typically removes item and its
    // "children" (for example messages or child feeds)
    // from the database.
    // Returns true if "I" was removed.
    virtual bool removeItself() {
      return false;
    }

    // Access to children.
    inline QList<RootItem*> childItems() const {
      return m_childItems;
    }

    // Checks whether THIS object is child (direct or indirect)
    // of the given root.
    bool isChildOf(RootItem *root) {
      if (root == NULL) {
        return false;
      }

      RootItem *this_item = this;

      while (this_item->kind() != RootItemKind::Root) {
        if (root->childItems().contains(this_item)) {
          return true;
        }
        else {
          this_item = this_item->parent();
        }
      }

      return false;
    }

    // Is "this" item parent if given child?
    bool isParentOf(RootItem *child) {
      if (child == NULL) {
        return false;
      }
      else {
        return child->isChildOf(this);
      }
    }

    // Removes all children from this item.
    // NOTE: Children are NOT freed from the memory.
    inline void clearChildren() {
      m_childItems.clear();
    }

    // Returns flat list of all items from subtree where this item is a root.
    // Returned list includes this item too.
    QList<RootItem*> getSubTree();
    QList<Category*> getSubTreeCategories();
    QList<Feed*> getSubTreeFeeds();

    // Removes particular child at given index.
    // NOTE: Child is NOT freed from the memory.
    bool removeChild(int index);
    bool removeChild(RootItem *child);

    inline RootItemKind::Kind kind() const {
      return m_kind;
    }

    // Each item can have icon.
    inline QIcon icon() const {
      return m_icon;
    }

    inline void setIcon(const QIcon &icon) {
      m_icon = icon;
    }

    // Each item has some kind of id. Usually taken from primary key attribute from DB.
    inline int id() const {
      return m_id;
    }

    inline void setId(int id) {
      m_id = id;
    }

    // Each item has its title.
    inline QString title() const {
      return m_title;
    }

    inline void setTitle(const QString &title) {
      m_title = title;
    }

    inline QDateTime creationDate() const {
      return m_creationDate;
    }

    inline void setCreationDate(const QDateTime &creation_date) {
      m_creationDate = creation_date;
    }

    inline QString description() const {
      return m_description;
    }

    inline void setDescription(const QString &description) {
      m_description = description;
    }

    // Converters
    Category *toCategory();
    Feed *toFeed();

    // Compares two model items.
    static bool isEqual(RootItem *lhs, RootItem *rhs);
    static bool lessThan(RootItem *lhs, RootItem *rhs);

  protected:
    void setupFonts();

    RootItemKind::Kind m_kind;
    int m_id;
    QString m_title;
    QString m_description;
    QIcon m_icon;
    QDateTime m_creationDate;

    QFont m_normalFont;
    QFont m_boldFont;

    QList<RootItem*> m_childItems;
    RootItem *m_parentItem;
};

#endif // ROOTITEM_H
