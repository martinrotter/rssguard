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

#include "services/standard/standardserviceroot.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/iconfactory.h"
#include "core/feedsmodel.h"
#include "services/standard/standardserviceentrypoint.h"
#include "services/standard/standardrecyclebin.h"
#include "services/standard/standardfeed.h"
#include "services/standard/standardcategory.h"
#include "services/standard/standardfeedsimportexportmodel.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QStack>
#include <QCoreApplication>
#include <QMenu>


StandardServiceRoot::StandardServiceRoot(bool load_from_db, FeedsModel *feeds_model, RootItem *parent)
  : ServiceRoot(feeds_model, parent), m_recycleBin(new StandardRecycleBin(this)), m_addItemMenu(NULL) {
  m_title = qApp->system()->getUsername() + QL1S("@") + QL1S(APP_LOW_NAME);
  m_icon = StandardServiceEntryPoint().icon();
  m_description = tr("This is obligatory service account for standard RSS/RDF/ATOM feeds.");
  m_creationDate = QDateTime::currentDateTime();

  if (load_from_db) {
    loadFromDatabase();
  }
}

StandardServiceRoot::~StandardServiceRoot() {
  if (m_addItemMenu != NULL) {
    delete m_addItemMenu;
  }
}

bool StandardServiceRoot::canBeEdited() {
  return false;
}

bool StandardServiceRoot::canBeDeleted() {
  return false;
}

QVariant StandardServiceRoot::data(int column, int role) const {
  switch (role) {
    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::CountFormat)).toString()
            .replace(PLACEHOLDER_UNREAD_COUNTS, QString::number(countOfUnreadMessages()))
            .replace(PLACEHOLDER_ALL_COUNTS, QString::number(countOfAllMessages()));
      }
      else {
        return QVariant();
      }

    case Qt::EditRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return countOfUnreadMessages();
      }
      else {
        return QVariant();
      }

    case Qt::DecorationRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_icon;
      }
      else {
        return QVariant();
      }

    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return tr("This is service account for standard RSS/RDF/ATOM feeds.");
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        //: Tooltip for "unread" column of feed list.
        return tr("%n unread message(s).", 0, countOfUnreadMessages());
      }
      else {
        return QVariant();
      }

    case Qt::TextAlignmentRole:
      if (column == FDS_MODEL_COUNTS_INDEX) {
        return Qt::AlignCenter;
      }
      else {
        return QVariant();
      }

    case Qt::FontRole:
      return countOfUnreadMessages() > 0 ? m_boldFont : m_normalFont;

    default:
      return QVariant();
  }
}

void StandardServiceRoot::loadFromDatabase(){
  QSqlDatabase database = qApp->database()->connection("StandardServiceRoot", DatabaseFactory::FromSettings);
  CategoryAssignment categories;
  FeedAssignment feeds;

  // Obtain data for categories from the database.
  QSqlQuery query_categories(database);
  query_categories.setForwardOnly(true);

  if (!query_categories.exec(QSL("SELECT * FROM Categories;")) || query_categories.lastError().isValid()) {
    qFatal("Query for obtaining categories failed. Error message: '%s'.",
           qPrintable(query_categories.lastError().text()));
  }

  while (query_categories.next()) {
    CategoryAssignmentItem pair;
    pair.first = query_categories.value(CAT_DB_PARENT_ID_INDEX).toInt();
    pair.second = new StandardCategory(query_categories.record());

    categories << pair;
  }

  // All categories are now loaded.
  QSqlQuery query_feeds(database);
  query_feeds.setForwardOnly(true);

  if (!query_feeds.exec(QSL("SELECT * FROM Feeds;")) || query_feeds.lastError().isValid()) {
    qFatal("Query for obtaining feeds failed. Error message: '%s'.",
           qPrintable(query_feeds.lastError().text()));
  }

  while (query_feeds.next()) {
    // Process this feed.
    StandardFeed::Type type = static_cast<StandardFeed::Type>(query_feeds.value(FDS_DB_TYPE_INDEX).toInt());

    switch (type) {
      case StandardFeed::Atom10:
      case StandardFeed::Rdf:
      case StandardFeed::Rss0X:
      case StandardFeed::Rss2X: {
        FeedAssignmentItem pair;
        pair.first = query_feeds.value(FDS_DB_CATEGORY_INDEX).toInt();
        pair.second = new StandardFeed(query_feeds.record());
        pair.second->setType(type);

        feeds << pair;
        break;
      }

      default:
        break;
    }
  }

  // All data are now obtained, lets create the hierarchy.
  assembleCategories(categories);
  assembleFeeds(feeds);

  // As the last item, add recycle bin, which is needed.
  appendChild(m_recycleBin);
}

QHash<int,StandardCategory*> StandardServiceRoot::categoriesForItem(RootItem *root) {
  QHash<int,StandardCategory*> categories;
  QList<RootItem*> parents;

  parents.append(root->childItems());

  while (!parents.isEmpty()) {
    RootItem *item = parents.takeFirst();

    if (item->kind() == RootItemKind::Category) {
      // This item is category, add it to the output list and
      // scan its children.
      int category_id = item->id();
      StandardCategory *category = static_cast<StandardCategory*>(item);

      if (!categories.contains(category_id)) {
        categories.insert(category_id, category);
      }

      parents.append(category->childItems());
    }
  }

  return categories;
}

QHash<int,StandardCategory*> StandardServiceRoot::allCategories() {
  return categoriesForItem(this);
}

QList<QAction*> StandardServiceRoot::getContextMenuForFeed(StandardFeed *feed) {
  QList<QAction*> list;

  // Fetch feed metadata.
  QAction *action_fetch_metadata = new QAction(qApp->icons()->fromTheme(QSL("download-manager")), tr("Fetch metadata"), NULL);
  connect(action_fetch_metadata, SIGNAL(triggered()), feed, SLOT(fetchMetadataForItself()));

  list.append(action_fetch_metadata);
  return list;
}

void StandardServiceRoot::assembleFeeds(FeedAssignment feeds) {
  QHash<int,StandardCategory*> categories = categoriesForItem(this);

  foreach (const FeedAssignmentItem &feed, feeds) {
    if (feed.first == NO_PARENT_CATEGORY) {
      // This is top-level feed, add it to the root item.
      appendChild(feed.second);
    }
    else if (categories.contains(feed.first)) {
      // This feed belongs to this category.
      categories.value(feed.first)->appendChild(feed.second);
    }
    else {
      qWarning("Feed '%s' is loose, skipping it.", qPrintable(feed.second->title()));
    }
  }
}

StandardRecycleBin *StandardServiceRoot::recycleBin() const {
  return m_recycleBin;
}

bool StandardServiceRoot::mergeImportExportModel(FeedsImportExportModel *model, QString &output_message) {
  QStack<RootItem*> original_parents; original_parents.push(this);
  QStack<RootItem*> new_parents; new_parents.push(model->rootItem());
  bool some_feed_category_error = false;

  // Iterate all new items we would like to merge into current model.
  while (!new_parents.isEmpty()) {
    RootItem *target_parent = original_parents.pop();
    RootItem *source_parent = new_parents.pop();

    foreach (RootItem *source_item, source_parent->childItems()) {
      if (!model->isItemChecked(source_item)) {
        // We can skip this item, because it is not checked and should not be imported.
        // NOTE: All descendants are thus skipped too.
        continue;
      }

      if (source_item->kind() == RootItemKind::Category) {
        StandardCategory *source_category = static_cast<StandardCategory*>(source_item);
        StandardCategory *new_category = new StandardCategory(*source_category);
        QString new_category_title = new_category->title();

        // Add category to model.
        new_category->clearChildren();

        if (new_category->addItself(target_parent)) {
          m_feedsModel->reassignNodeToNewParent(new_category, target_parent);

          // Process all children of this category.
          original_parents.push(new_category);
          new_parents.push(source_category);
        }
        else {
          delete new_category;

          // Add category failed, but this can mean that the same category (with same title)
          // already exists. If such a category exists in current parent, then find it and
          // add descendants to it.
          RootItem *existing_category = NULL;
          foreach (RootItem *child, target_parent->childItems()) {
            if (child->kind() == RootItemKind::Category && child->title() == new_category_title) {
              existing_category = child;
            }
          }

          if (existing_category != NULL) {
            original_parents.push(existing_category);
            new_parents.push(source_category);
          }
          else {
            some_feed_category_error = true;
          }
        }
      }
      else if (source_item->kind() == RootItemKind::Feed) {
        StandardFeed *source_feed = static_cast<StandardFeed*>(source_item);
        StandardFeed *new_feed = new StandardFeed(*source_feed);

        // Append this feed and end this iteration.
        if (new_feed->addItself(target_parent)) {
          m_feedsModel->reassignNodeToNewParent(new_feed, target_parent);
        }
        else {
          delete new_feed;
          some_feed_category_error = true;
        }
      }
    }
  }

  if (some_feed_category_error) {
    output_message = tr("Import successfull, but some feeds/categories were not imported due to error.");
  }
  else {
    output_message = tr("Import was completely successfull.");
  }

  return !some_feed_category_error;
}

QMenu *StandardServiceRoot::addItemMenu() {
  if (m_addItemMenu == NULL) {
    m_addItemMenu = new QMenu(title(), NULL);
    m_addItemMenu->setIcon(icon());
    m_addItemMenu->setToolTip(description());

    // TODO: Add items.
    m_addItemMenu->addAction(new QAction("abc", m_addItemMenu));
  }

  return m_addItemMenu;
}

void StandardServiceRoot::assembleCategories(CategoryAssignment categories) {
  QHash<int, RootItem*> assignments;
  assignments.insert(NO_PARENT_CATEGORY, this);

  // Add top-level categories.
  while (!categories.isEmpty()) {
    for (int i = 0; i < categories.size(); i++) {
      if (assignments.contains(categories.at(i).first)) {
        // Parent category of this category is already added.
        assignments.value(categories.at(i).first)->appendChild(categories.at(i).second);

        // Now, added category can be parent for another categories, add it.
        assignments.insert(categories.at(i).second->id(), categories.at(i).second);

        // Remove the category from the list, because it was
        // added to the final collection.
        categories.removeAt(i);
        i--;
      }
    }
  }
}
