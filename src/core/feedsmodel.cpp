#include "core/feedsmodel.h"

#include "core/defs.h"
#include "core/databasefactory.h"
#include "core/feedsmodelstandardcategory.h"
#include "core/feedsmodelstandardfeed.h"
#include "core/textfactory.h"
#include "gui/iconthemefactory.h"
#include "gui/iconfactory.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QPair>

#include <algorithm>


FeedsModel::FeedsModel(QObject *parent) : QAbstractItemModel(parent) {
  setObjectName("FeedsModel");

  m_rootItem = new FeedsModelRootItem();
  m_rootItem->setId(NO_PARENT_CATEGORY);
  m_rootItem->setTitle(tr("Root"));
  m_rootItem->setIcon(IconThemeFactory::instance()->fromTheme("folder-root"));
  m_countsIcon = IconThemeFactory::instance()->fromTheme("mail-mark-unread");
  m_headerData << tr("Title");
  m_tooltipData << tr("Titles of feeds/categories.") <<
                   tr("Counts of unread/all meesages.");

  loadFromDatabase();
}

FeedsModel::~FeedsModel() {
  qDebug("Destroying FeedsModel instance.");

  // Delete all model items.
  delete m_rootItem;
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

bool FeedsModel::removeItem(const QModelIndex &index) { 
  if (index.isValid()) {
    QModelIndex parent_index = index.parent();
    FeedsModelRootItem *deleting_item = itemForIndex(index);
    FeedsModelRootItem *parent_item = deleting_item->parent();

    // Try to persistently remove the item.
    if (deleting_item->removeItself()) {
      // Item was persistently removed.
      // Remove it from the model.
      beginRemoveRows(parent_index, index.row(), index.row());
      parent_item->removeChild(deleting_item);
      endRemoveRows();

      delete deleting_item;

      return true;
    }
  }

  // Item was not removed successfully.
  return false;
}

bool FeedsModel::addStandardCategory(FeedsModelStandardCategory *category,
                                     FeedsModelRootItem *parent) {
  // Get index of parent item (parent standard category).
  QModelIndex parent_index = indexForItem(parent);

  // Now, add category to persistent storage.
  // Children are removed, remove this standard category too.
  QSqlDatabase database = DatabaseFactory::instance()->connection(objectName(),
                                                                  DatabaseFactory::FromSettings);
  QSqlQuery query_add(database);

  query_add.setForwardOnly(true);
  query_add.prepare("INSERT INTO Categories "
                    "(parent_id, title, description, date_created, icon, type) "
                    "VALUES (:parent_id, :title, :description, :date_created, :icon, :type);");
  query_add.bindValue(":parent_id", parent->id());
  query_add.bindValue(":title", category->title());
  query_add.bindValue(":description", category->description());
  query_add.bindValue(":date_created", category->creationDate().toMSecsSinceEpoch());
  query_add.bindValue(":icon", IconFactory::toByteArray(category->icon()));
  query_add.bindValue(":type", (int) FeedsModelCategory::Standard);

  if (!query_add.exec()) {
    // Query failed.
    return false;
  }

  query_add.prepare("SELECT id FROM Categories WHERE date_created = :date_created;");
  query_add.bindValue(":date_created", category->creationDate().toMSecsSinceEpoch());
  if (query_add.exec() && query_add.next()) {
    // New category was added, fetch is primary id
    // from the database.
    category->setId(query_add.value(0).toInt());
  }
  else {
    // Something failed.
    return false;
  }

  // Category was added to the persistent storage,
  // so add it to the model.
  beginInsertRows(parent_index, parent->childCount(), parent->childCount());
  parent->appendChild(category);
  endInsertRows();

  return true;
}

bool FeedsModel::editStandardCategory(FeedsModelStandardCategory *original_category,
                                      FeedsModelStandardCategory *new_category) {
  QSqlDatabase database = DatabaseFactory::instance()->connection(objectName(),
                                                                  DatabaseFactory::FromSettings);
  QSqlQuery query_update_category(database);
  FeedsModelRootItem *original_parent = original_category->parent();
  FeedsModelRootItem *new_parent = new_category->parent();

  query_update_category.setForwardOnly(true);
  query_update_category.prepare("UPDATE Categories "
                                "SET title = :title, description = :description, icon = :icon, parent_id = :parent_id "
                                "WHERE id = :id;");
  query_update_category.bindValue(":title", new_category->title());
  query_update_category.bindValue(":description", new_category->description());
  query_update_category.bindValue(":icon", IconFactory::toByteArray(new_category->icon()));
  query_update_category.bindValue(":parent_id", new_parent->id());
  query_update_category.bindValue(":id", original_category->id());

  if (!query_update_category.exec()) {
    // Persistent storage update failed, no way to continue now.
    return false;
  }

  // Setup new model data for the original item.
  original_category->setDescription(new_category->description());
  original_category->setIcon(new_category->icon());
  original_category->setTitle(new_category->title());

  if (original_parent != new_parent) {
    // User edited category and set it new parent item,
    // se we need to move the item in the model too.
    int original_index_of_category = original_parent->childItems().indexOf(original_category);
    int new_index_of_category = new_parent->childCount();

    // Remove the original item from the model...
    beginRemoveRows(indexForItem(original_parent),
                    original_index_of_category,
                    original_index_of_category);
    original_parent->removeChild(original_category);
    endRemoveRows();

    // ... and insert it under the new parent.
    beginInsertRows(indexForItem(new_parent),
                    new_index_of_category,
                    new_index_of_category);
    new_parent->appendChild(original_category);
    endInsertRows();
  }

  // Free temporary category from memory.
  delete new_category;

  // Editing is done.
  return true;
}

bool FeedsModel::addStandardFeed(FeedsModelStandardFeed *feed,
                                 FeedsModelRootItem *parent) {
  // Get index of parent item (parent standard category).
  QModelIndex parent_index = indexForItem(parent);

  // Now, add category to persistent storage.
  // Children are removed, remove this standard category too.
  QSqlDatabase database = DatabaseFactory::instance()->connection(objectName(),
                                                                  DatabaseFactory::FromSettings);
  QSqlQuery query_add_feed(database);

  query_add_feed.setForwardOnly(true);
  query_add_feed.prepare("INSERT INTO Feeds "
                         "(title, description, date_created, icon, category, encoding, url, protected, username, password, update_type, update_interval, type) "
                         "VALUES (:title, :description, :date_created, :icon, :category, :encoding, :url, :protected, :username, :password, :update_type, :update_interval, :type);");
  query_add_feed.bindValue(":title", feed->title());
  query_add_feed.bindValue(":description", feed->description());
  query_add_feed.bindValue(":date_created", feed->creationDate().toMSecsSinceEpoch());
  query_add_feed.bindValue(":icon", IconFactory::toByteArray(feed->icon()));
  query_add_feed.bindValue(":category", parent->id());
  query_add_feed.bindValue(":encoding", feed->encoding());
  query_add_feed.bindValue(":url", feed->url());
  query_add_feed.bindValue(":protected", (int) feed->passwordProtected());
  query_add_feed.bindValue(":username", feed->username());
  query_add_feed.bindValue(":password", feed->password());
  query_add_feed.bindValue(":update_type", (int) feed->autoUpdateType());
  query_add_feed.bindValue(":update_interval", feed->autoUpdateInitialInterval());
  query_add_feed.bindValue(":type", (int) FeedsModelCategory::Standard);

  if (!query_add_feed.exec()) {
    // Query failed.
    return false;
  }

  query_add_feed.prepare("SELECT id FROM Feeds WHERE date_created = :date_created;");
  query_add_feed.bindValue(":date_created", feed->creationDate().toMSecsSinceEpoch());
  if (query_add_feed.exec() && query_add_feed.next()) {
    // New category was added, fetch is primary id
    // from the database.
    feed->setId(query_add_feed.value(0).toInt());
  }
  else {
    // Something failed.
    return false;
  }

  // Category was added to the persistent storage,
  // so add it to the model.
  beginInsertRows(parent_index, parent->childCount(), parent->childCount());
  parent->appendChild(feed);
  endInsertRows();

  return true;
}

bool FeedsModel::editStandardFeed(FeedsModelStandardFeed *original_feed,
                                  FeedsModelStandardFeed *new_feed) {
  QSqlDatabase database = DatabaseFactory::instance()->connection(objectName(),
                                                                  DatabaseFactory::FromSettings);
  QSqlQuery query_update_feed(database);
  FeedsModelRootItem *original_parent = original_feed->parent();
  FeedsModelRootItem *new_parent = new_feed->parent();

  query_update_feed.setForwardOnly(true);
  query_update_feed.prepare("UPDATE Feeds "
                            "SET title = :title, description = :description, icon = :icon, category = :category, encoding = :encoding, url = :url, protected = :protected, username = :username, password = :password, update_type = :update_type, update_interval = :update_interval, type = :type "
                            "WHERE id = :id;");
  query_update_feed.bindValue(":title", new_feed->title());
  query_update_feed.bindValue(":description", new_feed->description());
  query_update_feed.bindValue(":icon", IconFactory::toByteArray(new_feed->icon()));
  query_update_feed.bindValue(":category", new_parent->id());
  query_update_feed.bindValue(":encoding", new_feed->encoding());
  query_update_feed.bindValue(":url", new_feed->url());
  query_update_feed.bindValue(":protected", (int) new_feed->passwordProtected());
  query_update_feed.bindValue(":username", new_feed->username());
  query_update_feed.bindValue(":password", new_feed->password());
  query_update_feed.bindValue(":update_type", (int) new_feed->autoUpdateType());
  query_update_feed.bindValue(":update_interval", new_feed->autoUpdateInitialInterval());
  query_update_feed.bindValue(":type", new_feed->type());
  query_update_feed.bindValue(":id", original_feed->id());

  if (!query_update_feed.exec()) {
    // Persistent storage update failed, no way to continue now.
    return false;
  }

  // Setup new model data for the original item.
  original_feed->setTitle(new_feed->title());
  original_feed->setDescription(new_feed->description());
  original_feed->setIcon(new_feed->icon());
  original_feed->setEncoding(new_feed->encoding());
  original_feed->setDescription(new_feed->description());
  original_feed->setUrl(new_feed->url());
  original_feed->setPasswordProtected(new_feed->passwordProtected());
  original_feed->setUsername(new_feed->username());
  original_feed->setPassword(new_feed->password());
  original_feed->setAutoUpdateType(new_feed->autoUpdateType());
  original_feed->setAutoUpdateInitialInterval(new_feed->autoUpdateInitialInterval());
  original_feed->setType(new_feed->type());

  if (original_parent != new_parent) {
    // User edited category and set it new parent item,
    // se we need to move the item in the model too.
    int original_index_of_feed = original_parent->childItems().indexOf(original_feed);
    int new_index_of_feed = new_parent->childCount();

    // Remove the original item from the model...
    beginRemoveRows(indexForItem(original_parent),
                    original_index_of_feed,
                    original_index_of_feed);
    original_parent->removeChild(original_feed);
    endRemoveRows();

    // ... and insert it under the new parent.
    beginInsertRows(indexForItem(new_parent),
                    new_index_of_feed,
                    new_index_of_feed);
    new_parent->appendChild(original_feed);
    endInsertRows();
  }

  // Free temporary category from memory.
  delete new_feed;

  // Editing is done.
  return true;
}

QList<FeedsModelFeed*> FeedsModel::feedsForScheduledUpdate(bool auto_update_now) {
  QList<FeedsModelFeed*> feeds_for_update;

  foreach (FeedsModelFeed *feed, allFeeds()) {
    FeedsModelStandardFeed *std_feed = static_cast<FeedsModelStandardFeed*>(feed);

    switch (std_feed->autoUpdateType()) {
      case FeedsModelStandardFeed::DontAutoUpdate:
        // Do not auto-update this feed ever.
        continue;

      case FeedsModelStandardFeed::DefaultAutoUpdate:
        if (auto_update_now) {
          feeds_for_update.append(feed);
        }

        break;

      case FeedsModelStandardFeed::SpecificAutoUpdate:
      default:
        int remaining_interval = std_feed->autoUpdateRemainingInterval();

        if (--remaining_interval <= 0) {
          // Interval of this feed passed, include this feed in the output list
          // and reset the interval.
          feeds_for_update.append(feed);
          std_feed->setAutoUpdateRemainingInterval(std_feed->autoUpdateInitialInterval());
        }
        else {
          // Interval did not pass, set new decremented interval and do NOT
          // include this feed in the output list.
          std_feed->setAutoUpdateRemainingInterval(remaining_interval);
        }

        break;
    }
  }

  return feeds_for_update;
}

QList<Message> FeedsModel::messagesForFeeds(const QList<FeedsModelFeed*> &feeds) {
  QList<Message> messages;

  QSqlDatabase database = DatabaseFactory::instance()->connection(objectName(),
                                                                  DatabaseFactory::FromSettings);
  QSqlQuery query_read_msg(database);
  query_read_msg.setForwardOnly(true);
  query_read_msg.prepare("SELECT title, url, author, date_created, contents "
                         "FROM Messages "
                         "WHERE is_deleted = 0 AND feed = :feed;");

  foreach (FeedsModelFeed *feed, feeds) {
    query_read_msg.bindValue(":feed", feed->id());

    if (query_read_msg.exec()) {
      while (query_read_msg.next()) {
        Message message;

        message.m_title = query_read_msg.value(0).toString();
        message.m_url = query_read_msg.value(1).toString();
        message.m_author = query_read_msg.value(2).toString();
        message.m_created = TextFactory::parseDateTime(query_read_msg.value(3).value<qint64>());
        message.m_contents = query_read_msg.value(4).toString();

        messages.append(message);
      }
    }
  }

  return messages;
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

void FeedsModel::reloadChangedLayout(QModelIndexList list) {
  while (!list.isEmpty()) {
    QModelIndex indx = list.takeFirst();
    QModelIndex indx_parent = indx.parent();

    // Underlying data are changed.
    emit dataChanged(index(indx.row(), 0, indx_parent),
                     index(indx.row(), FDS_MODEL_COUNTS_INDEX, indx_parent));

    if (indx_parent.isValid()) {
      // Make sure that data of parent are changed too.
      list.prepend(indx_parent);
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
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

void FeedsModel::loadFromDatabase() {
  // Delete all childs of the root node and clear them from the memory.
  qDeleteAll(m_rootItem->childItems());
  m_rootItem->clearChildren();

  QSqlDatabase database = DatabaseFactory::instance()->connection(objectName(),
                                                                  DatabaseFactory::FromSettings);
  CategoryAssignment categories;
  FeedAssignment feeds;

  // Obtain data for categories from the database.
  QSqlQuery query_categories(database);
  query_categories.setForwardOnly(true);

  if (!query_categories.exec("SELECT * FROM Categories;") ||
      query_categories.lastError().isValid()) {
    qFatal("Query for obtaining categories failed. Error message: '%s'.",
           qPrintable(query_categories.lastError().text()));
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
      default:
        break;
    }
  }

  // All categories are now loaded.
  QSqlQuery query_feeds(database);
  query_feeds.setForwardOnly(true);

  if (!query_feeds.exec("SELECT * FROM Feeds;") ||
      query_feeds.lastError().isValid()) {
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
  return feedsForItem(item);
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
    feeds.erase(std::unique(feeds.begin(),
                            feeds.end(), FeedsModelRootItem::isEqual),
                feeds.end());
  }

  return feeds;
}

bool FeedsModel::markFeedsRead(const QList<FeedsModelFeed*> &feeds,
                               int read) {
  QSqlDatabase db_handle = DatabaseFactory::instance()->connection(objectName(),
                                                                   DatabaseFactory::FromSettings);

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for feeds read change.");
    return false;
  }

  QSqlQuery query_read_msg(db_handle);
  query_read_msg.setForwardOnly(true);

  if (!query_read_msg.prepare(QString("UPDATE Messages SET is_read = :read "
                                      "WHERE feed IN (%1) AND is_deleted = 0;").arg(textualFeedIds(feeds).join(", ")))) {
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

bool FeedsModel::markFeedsDeleted(const QList<FeedsModelFeed*> &feeds,
                                  int deleted,
                                  bool read_only) {
  QSqlDatabase db_handle = DatabaseFactory::instance()->connection(objectName(),
                                                                   DatabaseFactory::FromSettings);

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for feeds clearing.");
    return false;
  }

  QSqlQuery query_delete_msg(db_handle);
  query_delete_msg.setForwardOnly(true);

  if (read_only) {
    if (!query_delete_msg.prepare(QString("UPDATE Messages SET is_deleted = :deleted "
                                          "WHERE feed IN (%1) AND is_deleted = 0 AND is_read = 1;").arg(textualFeedIds(feeds).join(", ")))) {
      qWarning("Query preparation failed for feeds clearing.");

      db_handle.rollback();
      return false;
    }
  }
  else {
    if (!query_delete_msg.prepare(QString("UPDATE Messages SET is_deleted = :deleted "
                                          "WHERE feed IN (%1) AND is_deleted = 0;").arg(textualFeedIds(feeds).join(", ")))) {
      qWarning("Query preparation failed for feeds clearing.");

      db_handle.rollback();
      return false;
    }
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

QHash<int, FeedsModelCategory*> FeedsModel::allCategories() {
  return categoriesForItem(m_rootItem);
}

QHash<int, FeedsModelCategory*> FeedsModel::categoriesForItem(FeedsModelRootItem *root) {
  QHash<int, FeedsModelCategory*> categories;
  QList<FeedsModelRootItem*> parents;

  parents.append(root->childItems());

  while (!parents.isEmpty()) {
    FeedsModelRootItem *item = parents.takeFirst();

    if (item->kind() == FeedsModelRootItem::Category) {
      // This item is category, add it to the output list and
      // scan its children.
      int category_id = item->id();
      FeedsModelCategory *category = static_cast<FeedsModelCategory*>(item);

      if (!categories.contains(category_id)) {
        categories.insert(category_id, category);
      }

      parents.append(category->childItems());
    }
  }

  return categories;
}

QList<FeedsModelFeed*> FeedsModel::allFeeds() {
  return feedsForItem(m_rootItem);
}

QList<FeedsModelFeed*> FeedsModel::feedsForItem(FeedsModelRootItem *root) {
  QList<FeedsModelFeed*> feeds;

  if (root->kind() == FeedsModelRootItem::Feed) {
    // Root itself is a FEED.
    feeds.append(static_cast<FeedsModelFeed*>(root));
  }
  else {
    // Root itself is a CATEGORY or ROOT item.
    QList<FeedsModelRootItem*> traversable_items;

    traversable_items.append(root);

    // Iterate all nested categories.
    while (!traversable_items.isEmpty()) {
      FeedsModelRootItem *active_category = traversable_items.takeFirst();

      foreach (FeedsModelRootItem *child, active_category->childItems()) {
        if (child->kind() == FeedsModelRootItem::Feed) {
          // This child is feed.
          feeds.append(static_cast<FeedsModelFeed*>(child));
        }
        else if (child->kind() == FeedsModelRootItem::Category) {
          // This child is category, add its child feeds too.
          traversable_items.append(static_cast<FeedsModelCategory*>(child));
        }
      }
    }
  }

  return feeds;
}

void FeedsModel::assembleFeeds(FeedAssignment feeds) {
  QHash<int, FeedsModelCategory*> categories = allCategories();

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
