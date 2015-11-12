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
class ServiceRoot;
class QAction;

namespace RootItemKind {
  // Describes the kind of the item.
  enum Kind {
    Root        = 1,
    Bin         = 2,
    Feed        = 4,
    Category    = 8,
    ServiceRoot = 16
  };

  inline Kind operator|(Kind a, Kind b) {
    return static_cast<Kind>(static_cast<int>(a) | static_cast<int>(b));
  }
}

// Represents ROOT item of FeedsModel.
// NOTE: This class is derived to add functionality for
// all other non-root items of FeedsModel.
class RootItem : public QObject {
    Q_OBJECT

  public:
    enum ReadStatus {
      Read,
      Unread
    };

    enum CleanStatus {
      Clean,
      Unclean
    };

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

    // Returns list of specific actions which can be done with the item.
    // Do not include general actions here like actions:
    //  Mark as read, Update, ...
    // NOTE: Ownership of returned actions is not switched to caller, free them when needed.
    virtual QList<QAction*> contextMenuActions();

    virtual bool canBeEdited();
    virtual bool editViaGui();
    virtual bool canBeDeleted();
    virtual bool deleteViaGui();

    virtual bool canBeMarkedAsReadUnread(ReadStatus status);
    virtual bool markAsReadUnread(ReadStatus status);

    // This method should "clean" all messages it contains.
    // What "clean" means? It means delete message -> move them to recycle bin
    // or eventually remove them completely if there is no recycle bin functionality.
    // If this method is called on "recycle bin" instance of your
    // service account, it should not do anything.
    virtual bool cleanMessages(bool clear_only_read);

    virtual int row() const;
    virtual QVariant data(int column, int role) const;

    // Each item offers "counts" of messages.
    // Returns counts of messages of all child items summed up.
    virtual int countOfUnreadMessages() const;
    virtual int countOfAllMessages() const;

    // Access to children.
    inline QList<RootItem*> childItems() const {
      return m_childItems;
    }

    inline void setChildItems(QList<RootItem*> child_items) {
      m_childItems = child_items;
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
    QList<RootItem*> getSubTree(RootItemKind::Kind kind_of_item);
    QList<Category*> getSubTreeCategories();
    QList<Feed*> getSubTreeFeeds();

    // Returns the service root node which is direct or indirect parent of current item.
    ServiceRoot *getParentServiceRoot();

    // Removes particular child at given index.
    // NOTE: Child is NOT freed from the memory.
    bool removeChild(int index);
    bool removeChild(RootItem *child);

    inline RootItemKind::Kind kind() const {
      return m_kind;
    }

    inline void setKind(RootItemKind::Kind kind) {
      m_kind = kind;
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

    QFont normalFont() const;
    void setNormalFont(const QFont &normal_font);

    QFont boldFont() const;
    void setBoldFont(const QFont &bold_font);

    // Converters
    Category *toCategory();
    Feed *toFeed();
    ServiceRoot *toServiceRoot();

  private:
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
