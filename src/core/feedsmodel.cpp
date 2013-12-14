#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QPair>

#include "core/feedsmodel.h"
#include "core/feedsmodelstandardcategory.h"
#include "core/feedsmodelstandardfeed.h"
#include "core/defs.h"
#include "core/databasefactory.h"
#include "gui/iconthemefactory.h"
#include "gui/iconfactory.h"


FeedsModel::FeedsModel(QObject *parent) : QAbstractItemModel(parent) {
  setObjectName("FeedsModel");

  m_rootItem = new FeedsModelRootItem();
  m_countsIcon = IconThemeFactory::getInstance()->fromTheme("mail-mark-unread");

  m_headerData << tr("Title");
  m_tooltipData << tr("Titles of feeds/categories.") <<
                   tr("Counts of unread/all meesages.");

  loadFromDatabase();
/*
  FeedsModelStandardCategory *cat1 = new FeedsModelStandardCategory();
  FeedsModelStandardCategory *cat2 = new FeedsModelStandardCategory();
  FeedsModelStandardFeed *feed1 = new FeedsModelStandardFeed();
  FeedsModelStandardFeed *feed2 = new FeedsModelStandardFeed();
  FeedsModelStandardFeed *feed3 = new FeedsModelStandardFeed();
  FeedsModelStandardFeed *feed4 = new FeedsModelStandardFeed();
  FeedsModelStandardFeed *feed5 = new FeedsModelStandardFeed();

  feed1->setTitle("aaa");
  feed2->setTitle("aaa");
  feed3->setTitle("aaa");
  feed4->setTitle("aaa");
  feed5->setTitle("aaa");

  cat1->appendChild(feed1);
  cat1->appendChild(feed2);
  cat1->appendChild(cat2);

  cat2->appendChild(feed4);
  cat2->appendChild(feed5);

  m_rootItem->appendChild(cat1);
  m_rootItem->appendChild(feed3);
  */
}

FeedsModel::~FeedsModel() {
  qDebug("Destroying FeedsModel instance.");
  delete m_rootItem;
  DatabaseFactory::getInstance()->removeConnection(objectName());
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  FeedsModelRootItem *item = static_cast<FeedsModelRootItem*>(index.internalPointer());
  return item->data(index.column(), role);
}

QVariant FeedsModel::headerData(int section,
                                Qt::Orientation orientation,
                                int role) const {
  if (orientation != Qt::Horizontal) {
    return QVariant();
  }

  switch (role) {
    case Qt::DisplayRole:
      if (section == FDS_MODEL_TITLE_INDEX) {
        return m_headerData.at(FDS_MODEL_TITLE_INDEX);
      }
      else {
        return QVariant();
      }

    case Qt::ToolTipRole:
      return m_tooltipData.at(section);

    case Qt::DecorationRole:
      if (section == FDS_MODEL_COUNTS_INDEX) {
        return m_countsIcon;
      }
      else {
        return QVariant();
      }

    default:
      return QVariant();
  }
}

QModelIndex FeedsModel::index(int row, int column, const QModelIndex &parent) const {
  if (!hasIndex(row, column, parent)) {
    return QModelIndex();
  }

  FeedsModelRootItem *parent_item;

  if (!parent.isValid()) {
    parent_item = m_rootItem;
  }
  else {
    parent_item = static_cast<FeedsModelRootItem*>(parent.internalPointer());
  }

  FeedsModelRootItem *child_item = parent_item->child(row);

  if (child_item) {
    return createIndex(row, column, child_item);
  }
  else {
    return QModelIndex();
  }
}

QModelIndex FeedsModel::parent(const QModelIndex &child) const {
  if (!child.isValid()) {
    return QModelIndex();
  }

  FeedsModelRootItem *child_item = static_cast<FeedsModelRootItem*>(child.internalPointer());
  FeedsModelRootItem *parent_item = child_item->parent();

  if (parent_item == m_rootItem) {
    return QModelIndex();
  }
  else {
    return createIndex(parent_item->row(), 0, parent_item);
  }
}

int FeedsModel::rowCount(const QModelIndex &parent) const {
  FeedsModelRootItem *parent_item;

  if (parent.column() > 0) {
    return 0;
  }

  if (!parent.isValid()) {
    parent_item = m_rootItem;
  }
  else {
    parent_item = static_cast<FeedsModelRootItem*>(parent.internalPointer());
  }

  return parent_item->childCount();
}

int FeedsModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return static_cast<FeedsModelRootItem*>(parent.internalPointer())->columnCount();
  }
  else {
    return m_rootItem->columnCount();
  }
}

void FeedsModel::loadFromDatabase() {
  qDeleteAll(m_rootItem->m_childItems);

  QSqlDatabase database = DatabaseFactory::getInstance()->addConnection(objectName());
  CategoryAssignment categories;
  FeedAssignment feeds;

  // Obtain data for categories from the database.
  QSqlQuery query_categories = database.exec("SELECT * FROM Categories;");

  if (query_categories.lastError().isValid()) {
    qFatal("Query for obtaining categories failed.");
  }

  while (query_categories.next()) {
    // Process this category.
    FeedsModelCategory::Type type = static_cast<FeedsModelCategory::Type>(query_categories.value(CAT_DB_TYPE_INDEX).toInt());

    switch (type) {
      case FeedsModelCategory::Standard: {
        CategoryAssignmentItem pair;
        pair.first = query_categories.value(CAT_DB_PARENT_ID_INDEX).toInt();
        pair.second = FeedsModelStandardCategory::loadFromRecord(query_categories.record());

        categories << pair;
        break;
      }

      case FeedsModelCategory::Feedly:
      case FeedsModelCategory::TinyTinyRss:
      default:
        // NOTE: Not yet implemented.
        break;
    }
  }

  // All categories are now loaded.
  QSqlQuery query_feeds = database.exec("SELECT * FROM Feeds;");

  if (query_feeds.lastError().isValid()) {
    qFatal("Query for obtaining feeds failed.");
  }

  while (query_feeds.next()) {
    // Process this feed.
    FeedsModelFeed::Type type = static_cast<FeedsModelFeed::Type>(query_feeds.value(FDS_DB_TYPE_INDEX).toInt());

    switch (type) {
      case FeedsModelFeed::StandardAtom:
      case FeedsModelFeed::StandardRdf:
      case FeedsModelFeed::StandardRss: {
        FeedAssignmentItem pair;
        pair.first = query_feeds.value(FDS_DB_CATEGORY_INDEX).toInt();
        pair.second = FeedsModelStandardFeed::loadFromRecord(query_feeds.record());
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
}

QHash<int, FeedsModelCategory *> FeedsModel::getCategories(FeedsModelRootItem *root) {
  QHash<int, FeedsModelCategory*> categories;

  foreach (FeedsModelRootItem *child, root->m_childItems) {
    FeedsModelCategory *converted = dynamic_cast<FeedsModelCategory*>(child);

    if (converted != NULL) {
      // This child is some kind of category.
      categories.insert(converted->id(), converted);

      // Moreover, add all child categories of this category.
      categories.unite(getCategories(converted));
    }
  }

  return categories;
}

QHash<int, FeedsModelCategory *> FeedsModel::getCategories() {
  return getCategories(m_rootItem);
}

void FeedsModel::assembleFeeds(FeedAssignment feeds) {
  QHash<int, FeedsModelCategory*> categories = getCategories();

  foreach (FeedAssignmentItem feed, feeds) {
    if (feed.first == NO_PARENT_CATEGORY) {
      // This is top-level feed, add it to the root item.
      m_rootItem->appendChild(feed.second);
    }
    else {
      // This feed belongs to some category.
      categories.value(feed.first)->appendChild(feed.second);
    }
  }
}

void FeedsModel::assembleCategories(CategoryAssignment categories) {
  QHash<int, FeedsModelRootItem*> assignments;
  assignments.insert(-1, m_rootItem);

  // Add top-level categories.
  while (!categories.isEmpty()) {
    for (int i = 0; i < categories.size(); i++) {
      if (assignments.contains(categories.at(i).first)) {
        // Parent category of this category is already added.
        assignments.value(categories.at(i).first)->appendChild(categories.at(i).second);

        // Now, added category can be parent for another categories, add it.
        assignments.insert(categories.at(i).second->id(),
                           categories.at(i).second);

        // Remove the category from the list, because it was
        // added to the final collection.
        categories.removeAt(i);
        i--;
      }
    }
  }
}
