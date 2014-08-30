// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "core/feedsimportexportmodel.h"

#include "core/feedsmodelfeed.h"
#include "core/feedsmodelcategory.h"
#include "definitions/definitions.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDomAttr>
#include <QStack>
#include <QLocale>


FeedsImportExportModel::FeedsImportExportModel(QObject *parent)
  : QAbstractItemModel(parent), m_checkStates(QHash<FeedsModelRootItem*, Qt::CheckState>()), m_recursiveChange(false) {
}

FeedsImportExportModel::~FeedsImportExportModel() {
}

FeedsModelRootItem *FeedsImportExportModel::itemForIndex(const QModelIndex &index) const {
  if (index.isValid() && index.model() == this) {
    return static_cast<FeedsModelRootItem*>(index.internalPointer());
  }
  else {
    return m_rootItem;
  }
}

FeedsModelRootItem *FeedsImportExportModel::rootItem() const {
  return m_rootItem;
}

void FeedsImportExportModel::setRootItem(FeedsModelRootItem *rootItem) {
  m_rootItem = rootItem;
}

bool FeedsImportExportModel::exportToOMPL20(QByteArray &result) {
  QDomDocument opml_document;
  QDomProcessingInstruction xml_declaration = opml_document.createProcessingInstruction("xml",
                                                                                        "version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");
  opml_document.appendChild(xml_declaration);

  // Adde OPML 2.0 metadata.
  opml_document.appendChild(opml_document.createElement("opml"));
  opml_document.documentElement().setAttribute("version", "2.0");

  QDomElement elem_opml_head = opml_document.createElement("head");

  QDomElement elem_opml_title = opml_document.createElement("title");
  QDomText text_opml_title = opml_document.createTextNode(QString(APP_NAME));
  elem_opml_title.appendChild(text_opml_title);
  elem_opml_head.appendChild(elem_opml_title);

  QDomElement elem_opml_created = opml_document.createElement("dateCreated");
  QDomText text_opml_created = opml_document.createTextNode(QLocale::c().toString(QDateTime::currentDateTimeUtc(),
                                                                                  "ddd, dd MMM yyyy hh:mm:ss") + " GMT");
  elem_opml_created.appendChild(text_opml_created);
  elem_opml_head.appendChild(elem_opml_created);
  opml_document.documentElement().appendChild(elem_opml_head);

  QDomElement elem_opml_body = opml_document.createElement("body");

  QStack<FeedsModelRootItem*> items_to_process; items_to_process.push(m_rootItem);
  QStack<QDomElement> elements_to_use; elements_to_use.push(elem_opml_body);

  // Process all unprocessed nodes.
  while (!items_to_process.isEmpty()) {
    QDomElement active_element = elements_to_use.pop();
    FeedsModelRootItem *active_item = items_to_process.pop();

    foreach (FeedsModelRootItem *child_item, active_item->childItems()) {
      if (!m_checkStates.contains(child_item) || m_checkStates[child_item] != Qt::Checked) {
        continue;
      }

      switch (child_item->kind()) {
        case FeedsModelRootItem::Category: {
          QDomElement outline_category = opml_document.createElement("outline");
          outline_category.setAttribute("text", child_item->title());
          active_element.appendChild(outline_category);
          items_to_process.push(child_item);
          elements_to_use.push(outline_category);
          break;
        }

        case FeedsModelRootItem::Feed: {
          FeedsModelFeed *child_feed = static_cast<FeedsModelFeed*>(child_item);
          QDomElement outline_feed = opml_document.createElement("outline");
          outline_feed.setAttribute("text", child_feed->title());
          outline_feed.setAttribute("xmlUrl", child_feed->url());
          outline_feed.setAttribute("description", child_feed->description());
          outline_feed.setAttribute("encoding", child_feed->encoding());

          switch (child_feed->type()) {
            case FeedsModelFeed::Rss0X:
            case FeedsModelFeed::Rss2X:
              outline_feed.setAttribute("version", "RSS");
              break;

            case FeedsModelFeed::Rdf:
              outline_feed.setAttribute("version", "RSS");
              break;

            case FeedsModelFeed::Atom10:
              outline_feed.setAttribute("version", "ATOM");
              break;

            default:
              break;
          }

          if (child_feed->passwordProtected()) {
            outline_feed.setAttribute("username", child_feed->username());
            outline_feed.setAttribute("password", child_feed->password());
          }

          active_element.appendChild(outline_feed);
          break;
        }

        default:
          break;
      }
    }
  }

  opml_document.documentElement().appendChild(elem_opml_body);

  result = opml_document.toByteArray(2);
  return true;
}

bool FeedsImportExportModel::importAsOPML20(const QByteArray &data) {
  // TODO: vytvorit strukturu podle obsahu OPML souboru, pokud se to podaří,
  // pak vytvořít novej root item a ten nastavit jako root pro tento model
  return false;
}

QModelIndex FeedsImportExportModel::index(int row, int column, const QModelIndex &parent) const {
  if (!hasIndex(row, column, parent)) {
    return QModelIndex();
  }

  FeedsModelRootItem *parent_item = itemForIndex(parent);
  FeedsModelRootItem *child_item = parent_item->child(row);

  if (child_item) {
    return createIndex(row, column, child_item);
  }
  else {
    return QModelIndex();
  }
}

QModelIndex FeedsImportExportModel::indexForItem(FeedsModelRootItem *item) const {
  if (item == NULL || item->kind() == FeedsModelRootItem::RootItem) {
    // Root item lies on invalid index.
    return QModelIndex();
  }

  QList<QModelIndex> parents;

  // Start with root item (which obviously has invalid index).
  parents << indexForItem(m_rootItem);

  while (!parents.isEmpty()) {
    QModelIndex active_index = parents.takeFirst();
    int row_count = rowCount(active_index);

    if (row_count > 0) {
      // This index has children.
      // Lets take a look if our target item is among them.
      FeedsModelRootItem *active_item = itemForIndex(active_index);
      int candidate_index = active_item->childItems().indexOf(item);

      if (candidate_index >= 0) {
        // We found our item.
        return index(candidate_index, 0, active_index);
      }
      else {
        // Item is not found, add all "categories" from active_item.
        for (int i = 0; i < row_count; i++) {
          FeedsModelRootItem *possible_category = active_item->child(i);

          if (possible_category->kind() == FeedsModelRootItem::Category) {
            parents << index(i, 0, active_index);
          }
        }
      }
    }
  }

  return QModelIndex();
}

QModelIndex FeedsImportExportModel::parent(const QModelIndex &child) const {
  if (!child.isValid()) {
    return QModelIndex();
  }

  FeedsModelRootItem *child_item = itemForIndex(child);
  FeedsModelRootItem *parent_item = child_item->parent();

  if (parent_item == m_rootItem) {
    return QModelIndex();
  }
  else {
    return createIndex(parent_item->row(), 0, parent_item);
  }
}

int FeedsImportExportModel::rowCount(const QModelIndex &parent) const {
  if (parent.column() > 0) {
    return 0;
  }
  else {
    return itemForIndex(parent)->childCount();
  }
}

int FeedsImportExportModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)

  return 1;
}

QVariant FeedsImportExportModel::data(const QModelIndex &index, int role) const {
  if (index.column() != 0) {
    return QVariant();
  }

  FeedsModelRootItem *item = itemForIndex(index);

  if (role == Qt::CheckStateRole) {
    if (m_checkStates.contains(item)) {
      return m_checkStates.value(item);
    }
    else {
      return static_cast<int>(Qt::Unchecked);
    }
  }
  else {
    return item->data(index.column(), role);
  }
}

bool FeedsImportExportModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (index.isValid() && index.column() == 0 && role == Qt::CheckStateRole) {
    FeedsModelRootItem *item = itemForIndex(index);

    if (item != m_rootItem) {
      m_checkStates[item] = static_cast<Qt::CheckState>(value.toInt());
      emit dataChanged(index, index);

      if (m_recursiveChange) {
        return true;
      }

      foreach(FeedsModelRootItem *child, item->childItems()) {
        setData(indexForItem(child), value, Qt::CheckStateRole);
      }

      QModelIndex parent_index = index;
      m_recursiveChange = true;

      while ((parent_index = parent_index.parent()).isValid()) {
        // We now have parent index.
        item = item->parent();

        // Check children of this new parent item.
        Qt::CheckState parent_state = Qt::Unchecked;
        foreach (FeedsModelRootItem *child_of_parent, item->childItems()) {
          if (m_checkStates.contains(child_of_parent) && m_checkStates[child_of_parent] == Qt::Checked) {
            parent_state = Qt::Checked;
            break;
          }
        }

        setData(parent_index, parent_state, Qt::CheckStateRole);
      }

      m_recursiveChange = false;

      return true;
    }
  }

  return false;
}

Qt::ItemFlags FeedsImportExportModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) {
    return 0;
  }

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if ( index.column() == 0 ) {
    flags |= Qt::ItemIsUserCheckable;
  }

  return flags;
}
