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

#include "services/standard/standardfeedsimportexportmodel.h"

#include "services/standard/standardfeed.h"
#include "services/standard/standardcategory.h"
#include "services/standard/standardserviceroot.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDomAttr>
#include <QStack>
#include <QLocale>


FeedsImportExportModel::FeedsImportExportModel(QObject *parent)
  : QAbstractItemModel(parent), m_checkStates(QHash<RootItem*, Qt::CheckState>()),
    m_rootItem(NULL), m_recursiveChange(false), m_mode(Import) {
}

FeedsImportExportModel::~FeedsImportExportModel() {
  if (m_rootItem != NULL && m_mode == Import) {
    // Delete all model items, but only if we are in import mode. Export mode shares
    // root item with main feed model, thus cannot be deleted from memory now.
    delete m_rootItem;
  }
}

RootItem *FeedsImportExportModel::itemForIndex(const QModelIndex &index) const {
  if (index.isValid() && index.model() == this) {
    return static_cast<RootItem*>(index.internalPointer());
  }
  else {
    return m_rootItem;
  }
}

RootItem *FeedsImportExportModel::rootItem() const {
  return m_rootItem;
}

void FeedsImportExportModel::setRootItem(RootItem *root_item) {
  m_rootItem = root_item;
}

bool FeedsImportExportModel::exportToOMPL20(QByteArray &result) {
  QDomDocument opml_document;
  QDomProcessingInstruction xml_declaration = opml_document.createProcessingInstruction(QSL("xml"),
                                                                                        QSL("version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\""));
  opml_document.appendChild(xml_declaration);

  // Added OPML 2.0 metadata.
  opml_document.appendChild(opml_document.createElement(QSL("opml")));
  opml_document.documentElement().setAttribute(QSL("version"), QSL("version"));

  QDomElement elem_opml_head = opml_document.createElement(QSL("head"));

  QDomElement elem_opml_title = opml_document.createElement(QSL("title"));
  QDomText text_opml_title = opml_document.createTextNode(QString(APP_NAME));
  elem_opml_title.appendChild(text_opml_title);
  elem_opml_head.appendChild(elem_opml_title);

  QDomElement elem_opml_created = opml_document.createElement(QSL("dateCreated"));
  QDomText text_opml_created = opml_document.createTextNode(QLocale::c().toString(QDateTime::currentDateTimeUtc(),
                                                                                  QSL("ddd, dd MMM yyyy hh:mm:ss")) + QL1S(" GMT"));
  elem_opml_created.appendChild(text_opml_created);
  elem_opml_head.appendChild(elem_opml_created);
  opml_document.documentElement().appendChild(elem_opml_head);

  QDomElement elem_opml_body = opml_document.createElement(QSL("body"));
  QStack<RootItem*> items_to_process; items_to_process.push(m_rootItem);
  QStack<QDomElement> elements_to_use; elements_to_use.push(elem_opml_body);

  // Process all unprocessed nodes.
  while (!items_to_process.isEmpty()) {
    QDomElement active_element = elements_to_use.pop();
    RootItem *active_item = items_to_process.pop();

    foreach (RootItem *child_item, active_item->childItems()) {
      if (!m_checkStates.contains(child_item) || m_checkStates[child_item] != Qt::Checked) {
        continue;
      }

      switch (child_item->kind()) {
        case RootItemKind::Category: {
          QDomElement outline_category = opml_document.createElement(QSL("outline"));
          outline_category.setAttribute(QSL("text"), child_item->title());
          outline_category.setAttribute(QSL("description"), child_item->description());
          outline_category.setAttribute(QSL("rssguard:icon"), QString(qApp->icons()->toByteArray(child_item->icon())));
          active_element.appendChild(outline_category);
          items_to_process.push(child_item);
          elements_to_use.push(outline_category);
          break;
        }

        case RootItemKind::Feed: {
          StandardFeed *child_feed = static_cast<StandardFeed*>(child_item);
          QDomElement outline_feed = opml_document.createElement("outline");
          outline_feed.setAttribute(QSL("text"), child_feed->title());
          outline_feed.setAttribute(QSL("xmlUrl"), child_feed->url());
          outline_feed.setAttribute(QSL("description"), child_feed->description());
          outline_feed.setAttribute(QSL("encoding"), child_feed->encoding());
          outline_feed.setAttribute(QSL("title"), child_feed->title());
          outline_feed.setAttribute(QSL("rssguard:icon"), QString(qApp->icons()->toByteArray(child_feed->icon())));

          switch (child_feed->type()) {
            case StandardFeed::Rss0X:
            case StandardFeed::Rss2X:
              outline_feed.setAttribute(QSL("version"), QSL("RSS"));
              break;

            case StandardFeed::Rdf:
              outline_feed.setAttribute(QSL("version"), QSL("RSS1"));
              break;

            case StandardFeed::Atom10:
              outline_feed.setAttribute(QSL("version"), QSL("ATOM"));
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

void FeedsImportExportModel::importAsOPML20(const QByteArray &data) {
  emit parsingStarted();

  QDomDocument opml_document;

  if (!opml_document.setContent(data)) {
    emit parsingFinished(0, 0, true);
  }

  if (opml_document.documentElement().isNull() || opml_document.documentElement().tagName() != QSL("opml") ||
      opml_document.documentElement().elementsByTagName(QSL("body")).size() != 1) {
    // This really is not an OPML file.
    emit parsingFinished(0, 0, true);
  }

  int completed = 0, total = 0;
  StandardServiceRoot *root_item = new StandardServiceRoot();
  QStack<RootItem*> model_items; model_items.push(root_item);
  QStack<QDomElement> elements_to_process; elements_to_process.push(opml_document.documentElement().elementsByTagName(QSL("body")).at(0).toElement());

  while (!elements_to_process.isEmpty()) {
    RootItem *active_model_item = model_items.pop();
    QDomElement active_element = elements_to_process.pop();

    int current_count = active_element.childNodes().size();
    total += current_count;

    for (int i = 0; i < current_count; i++) {
      QDomNode child = active_element.childNodes().at(i);

      if (child.isElement()) {
        QDomElement child_element = child.toElement();

        // Now analyze if this element is category or feed.
        // NOTE: All feeds must include xmlUrl attribute and text attribute.
        if (child_element.attributes().contains(QSL("xmlUrl")) && child.attributes().contains(QSL("text"))) {
          // This is FEED.
          // Add feed and end this iteration.
          QString feed_title = child_element.attribute(QSL("text"));
          QString feed_url = child_element.attribute(QSL("xmlUrl"));
          QString feed_encoding = child_element.attribute(QSL("encoding"), DEFAULT_FEED_ENCODING);
          QString feed_type = child_element.attribute(QSL("version"), DEFAULT_FEED_TYPE).toUpper();
          QString feed_description = child_element.attribute(QSL("description"));
          QIcon feed_icon = qApp->icons()->fromByteArray(child_element.attribute(QSL("rssguard:icon")).toLocal8Bit());

          StandardFeed *new_feed = new StandardFeed(active_model_item);
          new_feed->setTitle(feed_title);
          new_feed->setDescription(feed_description);
          new_feed->setEncoding(feed_encoding);
          new_feed->setUrl(feed_url);
          new_feed->setCreationDate(QDateTime::currentDateTime());
          new_feed->setIcon(feed_icon.isNull() ? qApp->icons()->fromTheme(QSL("folder-feed")) : feed_icon);

          if (feed_type == QL1S("RSS1")) {
            new_feed->setType(StandardFeed::Rdf);
          }
          else if (feed_type == QL1S("ATOM")) {
            new_feed->setType(StandardFeed::Atom10);
          }
          else {
            new_feed->setType(StandardFeed::Rss2X);
          }

          active_model_item->appendChild(new_feed);
        }
        else {
          // This must be CATEGORY.
          // Add category and continue.
          QString category_title = child_element.attribute(QSL("text"));
          QString category_description = child_element.attribute(QSL("description"));
          QIcon category_icon = qApp->icons()->fromByteArray(child_element.attribute(QSL("rssguard:icon")).toLocal8Bit());

          if (category_title.isEmpty()) {
            qWarning("Given OMPL file provided category without valid text attribute. Using fallback name.");

            category_title = child_element.attribute(QSL("title"));

            if (category_title.isEmpty()) {
              category_title = tr("Category ") + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
            }
          }

          StandardCategory *new_category = new StandardCategory(active_model_item);
          new_category->setTitle(category_title);
          new_category->setIcon(category_icon.isNull() ? qApp->icons()->fromTheme(QSL("folder-category")) : category_icon);
          new_category->setCreationDate(QDateTime::currentDateTime());
          new_category->setDescription(category_description);

          active_model_item->appendChild(new_category);

          // Children of this node must be processed later.
          elements_to_process.push(child_element);
          model_items.push(new_category);
        }

        emit parsingProgress(++completed, total);
      }
    }
  }

  // Now, XML is processed and we have result in form of pointer item structure.
  emit layoutAboutToBeChanged();
  setRootItem(root_item);
  emit layoutChanged();
  emit parsingFinished(0, completed, false);
}

bool FeedsImportExportModel::exportToTxtURLPerLine(QByteArray &result) {
  foreach (const Feed * const feed, m_rootItem->getSubTreeFeeds()) {
    result += feed->url() + QL1S("\n");
  }

  return true;
}

void FeedsImportExportModel::importAsTxtURLPerLine(const QByteArray &data) {
  emit parsingStarted();

  int completed = 0, succeded = 0, failed = 0;
  StandardServiceRoot *root_item = new StandardServiceRoot();
  QList<QByteArray> urls = data.split('\n');

  foreach (const QByteArray &url, urls) {
    if (!url.isEmpty()) {
      QPair<StandardFeed*,QNetworkReply::NetworkError> guessed = StandardFeed::guessFeed(url);

      if (guessed.second == QNetworkReply::NoError) {
        guessed.first->setUrl(url);
        root_item->appendChild(guessed.first);
        succeded++;
      }
      else {
        StandardFeed *feed = new StandardFeed();

        feed->setUrl(url);
        feed->setTitle(url);
        feed->setCreationDate(QDateTime::currentDateTime());
        feed->setIcon(qApp->icons()->fromTheme(QSL("folder-feed")));
        feed->setEncoding(DEFAULT_FEED_ENCODING);
        root_item->appendChild(feed);
        failed++;
      }

      qApp->processEvents();
    }
    else {
      qWarning("Detected empty URL when parsing input TXT (one URL per line) data.");
      failed++;
    }

    emit parsingProgress(++completed, urls.size());
  }

  // Now, XML is processed and we have result in form of pointer item structure.
  emit layoutAboutToBeChanged();
  setRootItem(root_item);
  emit layoutChanged();
  emit parsingFinished(failed, succeded, false);
}

FeedsImportExportModel::Mode FeedsImportExportModel::mode() const {
  return m_mode;
}

void FeedsImportExportModel::setMode(const FeedsImportExportModel::Mode &mode) {
  m_mode = mode;
}

void FeedsImportExportModel::checkAllItems() {
  if (m_rootItem != NULL) {
    foreach (RootItem *root_child, m_rootItem->childItems()) {
      if (root_child->kind() != RootItemKind::Bin) {
        setData(indexForItem(root_child), Qt::Checked, Qt::CheckStateRole);
      }
    }
  }
}

void FeedsImportExportModel::uncheckAllItems() {
  if (m_rootItem != NULL) {
    foreach (RootItem *root_child, m_rootItem->childItems()) {
      if (root_child->kind() != RootItemKind::Bin) {
        setData(indexForItem(root_child), Qt::Unchecked, Qt::CheckStateRole);
      }
    }
  }
}

QModelIndex FeedsImportExportModel::index(int row, int column, const QModelIndex &parent) const {
  if (!hasIndex(row, column, parent)) {
    return QModelIndex();
  }

  RootItem *parent_item = itemForIndex(parent);
  RootItem *child_item = parent_item->child(row);

  if (child_item) {
    return createIndex(row, column, child_item);
  }
  else {
    return QModelIndex();
  }
}

QModelIndex FeedsImportExportModel::indexForItem(RootItem *item) const {
  if (item == NULL || item->kind() == RootItemKind::ServiceRoot || item->kind() == RootItemKind::Root) {
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
      RootItem *active_item = itemForIndex(active_index);
      int candidate_index = active_item->childItems().indexOf(item);

      if (candidate_index >= 0) {
        // We found our item.
        return index(candidate_index, 0, active_index);
      }
      else {
        // Item is not found, add all "categories" from active_item.
        for (int i = 0; i < row_count; i++) {
          RootItem *possible_category = active_item->child(i);

          if (possible_category->kind() == RootItemKind::Category) {
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

  RootItem *child_item = itemForIndex(child);
  RootItem *parent_item = child_item->parent();

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

  RootItem *item = itemForIndex(index);

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
      case RootItemKind::Category:
      case RootItemKind::Bin:
      case RootItemKind::Feed:
        return item->icon();

      default:
        return QVariant();
    }
  }
  else if (role == Qt::DisplayRole) {
    switch (item->kind()) {
      case RootItemKind::Category:
        return QVariant(item->data(index.column(), role).toString() + tr(" (category)"));

      case RootItemKind::Feed:
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
    RootItem *item = itemForIndex(index);

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
    foreach(RootItem *child, item->childItems()) {
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
      foreach (RootItem *child_of_parent, item->childItems()) {
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
  if (!index.isValid() || itemForIndex(index)->kind() == RootItemKind::Bin) {
    return Qt::NoItemFlags;
  }

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if ( index.column() == 0 ) {
    flags |= Qt::ItemIsUserCheckable;
  }

  return flags;
}

bool FeedsImportExportModel::isItemChecked(RootItem *item) {
  return m_checkStates.contains(item) && m_checkStates.value(item, Qt::Unchecked);
}
