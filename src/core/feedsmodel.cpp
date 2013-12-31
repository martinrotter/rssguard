#include "core/feedsmodel.h"

#include "core/defs.h"
#include "core/databasefactory.h"
#include "core/feedsmodelstandardcategory.h"
#include "core/feedsmodelstandardfeed.h"
#include "gui/iconthemefactory.h"
#include "gui/iconfactory.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QPair>
#include <QQueue>

#include <algorithm>


FeedsModel::FeedsModel(QObject *parent) : QAbstractItemModel(parent) {
  setObjectName("FeedsModel");

  m_rootItem = new FeedsModelRootItem();
  m_rootItem->setId(NO_PARENT_CATEGORY);
  m_rootItem->setTitle(tr("root"));
  m_rootItem->setIcon(IconThemeFactory::getInstance()->fromTheme("folder-red"));

  m_countsIcon = IconThemeFactory::getInstance()->fromTheme("mail-mark-unread");

  m_headerData << tr("Title");
  m_tooltipData << tr("Titles of feeds/categories.") <<
                   tr("Counts of unread/all meesages.");

  loadFromDatabase();
}

FeedsModel::~FeedsModel() {
  qDebug("Destroying FeedsModel instance.");

  delete m_rootItem;
  DatabaseFactory::getInstance()->removeConnection(objectName());
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const {
  return itemForIndex(index)->data(index.column(), role);
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

FeedsModelRootItem *FeedsModel::itemForIndex(const QModelIndex &index) const {
  if (index.isValid() && index.model() == this) {
    return static_cast<FeedsModelRootItem*>(index.internalPointer());
  }
  else {
    return m_rootItem;
  }
}

FeedsModelCategory *FeedsModel::categoryForIndex(const QModelIndex &index) const {
  FeedsModelRootItem *item = itemForIndex(index);

  if (item->kind() == FeedsModelRootItem::Category) {
    return static_cast<FeedsModelCategory*>(item);
  }
  else {
    return NULL;
  }
}


QModelIndex FeedsModel::indexForItem(FeedsModelRootItem *item) const {
  if (item == NULL || item->kind() == FeedsModelRootItem::RootItem) {
    // Root item lies on invalid index.
    return QModelIndex();
  }

  QList<QModelIndex> parents;

  // Start with root item.
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

/*
QModelIndex FeedsModel::indexForItem(FeedsModelRootItem *item) const {
  if (item->kind() == FeedsModelRootItem::RootItem) {
    // Root item lies on invalid index.
    return QModelIndex();
  }

  // TODO: Rewrite for better performance.

  QModelIndexList parents;

  // Start with invalid index (so that we start from the root
  // item).
  parents << QModelIndex();

  while (!parents.isEmpty()) {
    QModelIndex active_index = parents.takeFirst();
    int row_count = rowCount(active_index);

    // Iterate all childs of this parent.
    for (int i = 0; i < row_count; i++) {
      QModelIndex candidate_index = index(i, 0, active_index);

      // This index could be our target item.
      FeedsModelRootItem *target_item = itemForIndex(candidate_index);

      if (target_item != NULL) {
        if (FeedsModelRootItem::isEqual(target_item, item)) {
          // We found our target index, it's good.
          return candidate_index;
        }
        else if (hasChildren(candidate_index)) {
          // This is not our target index but it has children,
          // scan them too.
          parents << candidate_index;
        }
      }
    }

  }

  return QModelIndex();
}*/

void FeedsModel::reloadChangedLayout(QModelIndexList list) {
  while (!list.isEmpty()) {
    QModelIndex ix = list.takeLast();

    // Underlying data are changed.
    emit dataChanged(index(ix.row(), 0, ix.parent()),
                     index(ix.row(), FDS_MODEL_COUNTS_INDEX, ix.parent()));

    if (ix.parent().isValid()) {
      // Make sure that data of parent are changed too.
      list.append(ix.parent());
    }
  }
}

QStringList FeedsModel::textualFeedIds(const QList<FeedsModelFeed*> &feeds) {
  QStringList stringy_ids;
  stringy_ids.reserve(feeds.size());

  foreach (FeedsModelFeed *feed, feeds) {
    stringy_ids.append(QString::number(feed->id()));
  }

  return stringy_ids;
}

void FeedsModel::reloadWholeLayout() {
  // NOTE: Take a look at docs about this.
  // I have tested that this is LITTLE slower than code above,
  // but it is really SIMPLER, so if code above will be buggy, then
  // we can use this.
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

void FeedsModel::loadFromDatabase() {
  // Delete all childs of the root node and clear them from the memory.
  qDeleteAll(m_rootItem->childItems());
  m_rootItem->clearChilds();

  QSqlDatabase database = DatabaseFactory::getInstance()->addConnection(objectName());
  CategoryAssignment categories;
  FeedAssignment feeds;

  // Obtain data for categories from the database.
  QSqlQuery query_categories("SELECT * FROM Categories;", database);

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
  QSqlQuery query_feeds("SELECT * FROM Feeds;", database);

  if (query_feeds.lastError().isValid()) {
    qFatal("Query for obtaining feeds failed.");
  }

  while (query_feeds.next()) {
    // Process this feed.
    FeedsModelFeed::Type type = static_cast<FeedsModelFeed::Type>(query_feeds.value(FDS_DB_TYPE_INDEX).toInt());

    switch (type) {
      case FeedsModelFeed::StandardAtom10:
      case FeedsModelFeed::StandardRdf:
      case FeedsModelFeed::StandardRss0X:
      case FeedsModelFeed::StandardRss2X: {
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

QList<FeedsModelFeed*> FeedsModel::feedsForIndex(const QModelIndex &index) {
  FeedsModelRootItem *item = itemForIndex(index);
  return getFeeds(item);
}

FeedsModelFeed *FeedsModel::feedForIndex(const QModelIndex &index) {
  FeedsModelRootItem *item = itemForIndex(index);

  if (item->kind() == FeedsModelRootItem::Feed) {
    return static_cast<FeedsModelFeed*>(item);
  }
  else {
    return NULL;
  }
}

QList<FeedsModelFeed*> FeedsModel::feedsForIndexes(const QModelIndexList &indexes) {
  QList<FeedsModelFeed*> feeds;

  // Get selected feeds for each index.
  foreach (const QModelIndex &index, indexes) {
    feeds.append(feedsForIndex(index));
  }

  // Now we obtained all feeds from corresponding indexes.
  if (indexes.size() != feeds.size()) {
    // Selection contains duplicate feeds (for
    // example situation where feed and its parent category are both
    // selected). So, remove duplicates from the list.
    qSort(feeds.begin(), feeds.end(), FeedsModelRootItem::lessThan);
    feeds.erase(std::unique(feeds.begin(), feeds.end(), FeedsModelRootItem::isEqual),
                feeds.end());
  }

  return feeds;
}

bool FeedsModel::markFeedsRead(const QList<FeedsModelFeed*> &feeds,
                               int read) {
  QSqlDatabase db_handle = DatabaseFactory::getInstance()->addConnection(objectName());

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for feeds read change.");
    return false;
  }

  QSqlQuery query_read_msg(db_handle);
  if (!query_read_msg.prepare(QString("UPDATE messages SET read = :read "
                                      "WHERE feed IN (%1) AND deleted = 0").arg(textualFeedIds(feeds).join(", ")))) {
    qWarning("Query preparation failed for feeds read change.");

    db_handle.rollback();
    return false;
  }

  query_read_msg.bindValue(":read", read);

  if (!query_read_msg.exec()) {
    qDebug("Query execution for feeds read change failed.");
    db_handle.rollback();
  }

  // Commit changes.
  if (db_handle.commit()) {
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

bool FeedsModel::markFeedsDeleted(const QList<FeedsModelFeed *> &feeds,
                                  int deleted) {
  QSqlDatabase db_handle = DatabaseFactory::getInstance()->addConnection(objectName());

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for feeds clearing.");
    return false;
  }

  QSqlQuery query_delete_msg(db_handle);
  if (!query_delete_msg.prepare(QString("UPDATE messages SET deleted = :deleted "
                                        "WHERE feed IN (%1) AND deleted = 0").arg(textualFeedIds(feeds).join(", ")))) {
    qWarning("Query preparation failed for feeds clearing.");

    db_handle.rollback();
    return false;
  }

  query_delete_msg.bindValue(":deleted", deleted);

  if (!query_delete_msg.exec()) {
    qDebug("Query execution for feeds clearing failed.");
    db_handle.rollback();
  }

  // Commit changes.
  if (db_handle.commit()) {
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

QHash<int, FeedsModelCategory*> FeedsModel::getAllCategories() {
  return getCategories(m_rootItem);
}

// TODO: Rewrite this iterativelly (instead of
// current recursive implementation).
QHash<int, FeedsModelCategory*> FeedsModel::getCategories(FeedsModelRootItem *root) {
  QHash<int, FeedsModelCategory*> categories;

  foreach (FeedsModelRootItem *child, root->childItems()) {
    if (child->kind() == FeedsModelRootItem::Category) {
      FeedsModelCategory *converted = static_cast<FeedsModelCategory*>(child);

      // This child is some kind of category.
      categories.insert(converted->id(), converted);

      // Moreover, add all child categories of this category.
      categories.unite(getCategories(converted));
    }
  }

  return categories;
}

QList<FeedsModelFeed*> FeedsModel::getAllFeeds() {
  return getFeeds(m_rootItem);
}

QList<FeedsModelFeed*> FeedsModel::getFeeds(FeedsModelRootItem *root) {
  QList<FeedsModelFeed*> feeds;

  if (root->kind() == FeedsModelRootItem::Feed) {
    // Root itself is a FEED.
    feeds.append(static_cast<FeedsModelFeed*>(root));
  }
  else {
    // Root itself is a CATEGORY or ROOT item.
    QQueue<FeedsModelRootItem*> traversable_items;

    traversable_items.enqueue(root);

    // Iterate all nested categories.
    while (!traversable_items.isEmpty()) {
      FeedsModelRootItem *active_category = traversable_items.dequeue();

      foreach (FeedsModelRootItem *child, active_category->childItems()) {
        if (child->kind() == FeedsModelRootItem::Feed) {
          // This child is feed.
          feeds.append(static_cast<FeedsModelFeed*>(child));
        }
        else if (child->kind() == FeedsModelRootItem::Category) {
          // This child is category, add its child feeds too.
          traversable_items.enqueue(static_cast<FeedsModelCategory*>(child));
        }
      }
    }
  }

  return feeds;
}

void FeedsModel::assembleFeeds(FeedAssignment feeds) {
  QHash<int, FeedsModelCategory*> categories = getAllCategories();

  foreach (const FeedAssignmentItem &feed, feeds) {
    if (feed.first == NO_PARENT_CATEGORY) {
      // This is top-level feed, add it to the root item.
      m_rootItem->appendChild(feed.second);
    }
    else if (categories.contains(feed.first)) {
      // This feed belongs to this category.
      categories.value(feed.first)->appendChild(feed.second);
    }
    else {
      qWarning("Feed '%s' is loose, skipping it.",
               qPrintable(feed.second->title()));
    }
  }
}

FeedsModelRootItem *FeedsModel::rootItem() const {
  return m_rootItem;
}

void FeedsModel::assembleCategories(CategoryAssignment categories) {
  QHash<int, FeedsModelRootItem*> assignments;
  assignments.insert(NO_PARENT_CATEGORY, m_rootItem);

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
