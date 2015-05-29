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

#include "core/feedsimportexportmodel.h"

#include "core/feedsmodelfeed.h"
#include "core/feedsmodelcategory.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDomAttr>
#include <QStack>
#include <QLocale>


FeedsImportExportModel::FeedsImportExportModel(QObject *parent)
  : QAbstractItemModel(parent), m_checkStates(QHash<FeedsModelRootItem*, Qt::CheckState>()),
    m_rootItem(NULL), m_recursiveChange(false), m_mode(Import) {
}

FeedsImportExportModel::~FeedsImportExportModel() {
  if (m_rootItem != NULL && m_mode == Import) {
    // Delete all model items, but only if we are in import mode. Export mode shares
    // root item with main feed model, thus cannot be deleted from memory now.
    delete m_rootItem;
  }
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
          outline_category.setAttribute("description", child_item->description());
          outline_category.setAttribute("rssguard:icon", QString(qApp->icons()->toByteArray(child_item->icon())));
          active_element.appendChild(outline_category);
          items_to_process.push(child_item);
          elements_to_use.push(outline_category);
          break;
        }

        case FeedsModelRootItem::Feed: {
          FeedsModelFeed *child_feed = child_item->toFeed();
          QDomElement outline_feed = opml_document.createElement("outline");
          outline_feed.setAttribute("text", child_feed->title());
          outline_feed.setAttribute("xmlUrl", child_feed->url());
          outline_feed.setAttribute("description", child_feed->description());
          outline_feed.setAttribute("encoding", child_feed->encoding());
          outline_feed.setAttribute("title", child_feed->title());
          outline_feed.setAttribute("rssguard:icon", QString(qApp->icons()->toByteArray(child_feed->icon())));

          switch (child_feed->type()) {
            case FeedsModelFeed::Rss0X:
            case FeedsModelFeed::Rss2X:
              outline_feed.setAttribute("version", "RSS");
              break;

            case FeedsModelFeed::Rdf:
              outline_feed.setAttribute("version", "RSS1");
              break;

            case FeedsModelFeed::Atom10:
              outline_feed.setAttribute("version", "ATOM");
              break;

            default:
              break;
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
  QDomDocument opml_document;

  if (!opml_document.setContent(data)) {
    return false;
  }

  if (opml_document.documentElement().isNull() || opml_document.documentElement().tagName() != "opml" ||
      opml_document.documentElement().elementsByTagName("body").size() != 1) {
    // This really is not an OPML file.
    return false;
  }

  FeedsModelRootItem *root_item = new FeedsModelRootItem();
  QStack<FeedsModelRootItem*> model_items; model_items.push(root_item);
  QStack<QDomElement> elements_to_process; elements_to_process.push(opml_document.documentElement().elementsByTagName("body").at(0).toElement());

  while (!elements_to_process.isEmpty()) {
    FeedsModelRootItem *active_model_item = model_items.pop();
    QDomElement active_element = elements_to_process.pop();

    for (int i = 0; i < active_element.childNodes().size(); i++) {
      QDomNode child = active_element.childNodes().at(i);

      if (child.isElement()) {
        QDomElement child_element = child.toElement();

        // Now analyze if this element is category or feed.
        // NOTE: All feeds must include xmlUrl attribute and text attribute.
        if (child_element.attributes().contains("xmlUrl") && child.attributes().contains("text")) {
          // This is FEED.
          // Add feed and end this iteration.
          QString feed_title = child_element.attribute("text");
          QString feed_url = child_element.attribute("xmlUrl");
          QString feed_encoding = child_element.attribute("encoding", DEFAULT_FEED_ENCODING);
          QString feed_type = child_element.attribute("version", DEFAULT_FEED_TYPE).toUpper();
          QString feed_description = child_element.attribute("description");
          QIcon feed_icon = qApp->icons()->fromByteArray(child_element.attribute("rssguard:icon").toLocal8Bit());

          FeedsModelFeed *new_feed = new FeedsModelFeed(active_model_item);
          new_feed->setTitle(feed_title);
          new_feed->setDescription(feed_description);
          new_feed->setEncoding(feed_encoding);
          new_feed->setUrl(feed_url);
          new_feed->setCreationDate(QDateTime::currentDateTime());
          new_feed->setIcon(feed_icon.isNull() ? qApp->icons()->fromTheme("folder-feed") : feed_icon);
          new_feed->setAutoUpdateType(FeedsModelFeed::DefaultAutoUpdate);

          if (feed_type == "RSS1") {
            new_feed->setType(FeedsModelFeed::Rdf);
          }
          else if (feed_type == "ATOM") {
            new_feed->setType(FeedsModelFeed::Atom10);
          }
          else {
            new_feed->setType(FeedsModelFeed::Rss2X);
          }

          active_model_item->appendChild(new_feed);
        }
        else {
          // This must be CATEGORY.
          // Add category and continue.
          QString category_title = child_element.attribute("text");
          QString category_description = child_element.attribute("description");
          QIcon category_icon = qApp->icons()->fromByteArray(child_element.attribute("rssguard:icon").toLocal8Bit());

          if (category_title.isEmpty()) {
            qWarning("Given OMPL file provided category without valid text attribute. Using fallback name.");

            category_title = child_element.attribute("title");

            if (category_title.isEmpty()) {
              category_title = tr("Category ") + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
            }
          }

          FeedsModelCategory *new_category = new FeedsModelCategory(active_model_item);
          new_category->setTitle(category_title);
          new_category->setIcon(category_icon.isNull() ? qApp->icons()->fromTheme("folder-category") : category_icon);
          new_category->setCreationDate(QDateTime::currentDateTime());
          new_category->setDescription(category_description);

          active_model_item->appendChild(new_category);

          // Children of this node must be processed later.
          elements_to_process.push(child_element);
          model_items.push(new_category);
        }
      }
    }
  }

  // Now, XML is processed and we have result in form of pointer item structure.
  emit layoutAboutToBeChanged();
  setRootItem(root_item);
  emit layoutChanged();

  return true;
}

FeedsImportExportModel::Mode FeedsImportExportModel::mode() const {
  return m_mode;
}

void FeedsImportExportModel::setMode(const FeedsImportExportModel::Mode &mode) {
  m_mode = mode;
}

void FeedsImportExportModel::checkAllItems() {
  foreach (FeedsModelRootItem *root_child, m_rootItem->childItems()) {
    if (root_child->kind() != FeedsModelRootItem::RecycleBin) {
      setData(indexForItem(root_child), Qt::Checked, Qt::CheckStateRole);
    }
  }
}

void FeedsImportExportModel::uncheckAllItems() {
  foreach (FeedsModelRootItem *root_child, m_rootItem->childItems()) {
    if (root_child->kind() != FeedsModelRootItem::RecycleBin) {
      setData(indexForItem(root_child), Qt::Unchecked, Qt::CheckStateRole);
    }
  }
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
  else if (role == Qt::DecorationRole) {
    switch (item->kind()) {
      case FeedsModelRootItem::Category:
      case FeedsModelRootItem::RecycleBin:
      case FeedsModelRootItem::Feed:
        return item->icon();

      default:
        return QVariant();
    }
  }
  else if (role == Qt::DisplayRole) {
    switch (item->kind()) {
      case FeedsModelRootItem::Category:
        return QVariant(item->data(index.column(), role).toString() + tr(" (category)"));

      case FeedsModelRootItem::Feed:
        return QVariant(item->data(index.column(), role).toString() + tr(" (feed)"));

      default:
        return item->title();
    }
  }
  else {
    return QVariant();
  }
}

bool FeedsImportExportModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (index.isValid() && index.column() == 0 && role == Qt::CheckStateRole) {
    FeedsModelRootItem *item = itemForIndex(index);

    if (item == m_rootItem) {
      // Cannot set data on root item.
      return false;
    }

    // Change data for the actual item.
    m_checkStates[item] = static_cast<Qt::CheckState>(value.toInt());
    emit dataChanged(index, index);

    if (m_recursiveChange) {
      return true;
    }

    // Set new data for all descendants of this actual item.
    foreach(FeedsModelRootItem *child, item->childItems()) {
      setData(indexForItem(child), value, Qt::CheckStateRole);
    }

    // Now we need to change new data to all parents.
    QModelIndex parent_index = index;
    m_recursiveChange = true;

    // Iterate all valid parents.
    while ((parent_index = parent_index.parent()).isValid()) {
      // We now have parent index. Get parent item too.
      item = item->parent();

      // Check children of this new parent item.
      Qt::CheckState parent_state = Qt::Unchecked;
      foreach (FeedsModelRootItem *child_of_parent, item->childItems()) {
        if (m_checkStates.contains(child_of_parent) && m_checkStates[child_of_parent] == Qt::Checked) {
          // We found out, that some child of this item is checked,
          // therefore this item must be checked too.
          parent_state = Qt::Checked;
          break;
        }
      }

      setData(parent_index, parent_state, Qt::CheckStateRole);
    }

    m_recursiveChange = false;
    return true;
  }

  return false;
}

Qt::ItemFlags FeedsImportExportModel::flags(const QModelIndex &index) const {
  if (!index.isValid() || itemForIndex(index)->kind() == FeedsModelRootItem::RecycleBin) {
    return Qt::NoItemFlags;
  }

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if ( index.column() == 0 ) {
    flags |= Qt::ItemIsUserCheckable;
  }

  return flags;
}

bool FeedsImportExportModel::isItemChecked(FeedsModelRootItem *item) {
  return m_checkStates.contains(item) && m_checkStates.value(item, Qt::Unchecked);
}
