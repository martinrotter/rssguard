// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feedsmodel.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/feed.h"
#include "services/abstract/recyclebin.h"
#include "services/abstract/serviceentrypoint.h"
#include "services/abstract/serviceroot.h"

#include <QMimeData>
#include <QPair>
#include <QSqlError>
#include <QStack>
#include <QTimer>

FeedsModel::FeedsModel(QObject* parent) : QAbstractItemModel(parent), m_rootItem(new RootItem()) {
  setObjectName(QSL("FeedsModel"));

  // Create root item.

  // : Name of root item of feed list which can be seen in feed add/edit dialog.
  m_rootItem->setTitle(tr("Root"));
  m_rootItem->setIcon(qApp->icons()->fromTheme(QSL("folder")));

  // Setup icons.
  m_countsIcon = qApp->icons()->fromTheme(QSL("mail-mark-unread"));

  // : Title text in the feed list header.
  m_headerData = {tr("Title"), tr("Item ID"), tr("Article counts")};
  m_tooltipData = {tr("Titles of feeds/categories."),
                   tr("Database ID of each item."),
                   tr("Counts of unread/all mesages.")};

  setupFonts();
  setupBehaviorDuringFetching();
}

FeedsModel::~FeedsModel() {
  qDebugNN << LOGSEC_FEEDMODEL << "Destroying FeedsModel instance.";
  delete m_rootItem;
}

QMimeData* FeedsModel::mimeData(const QModelIndexList& indexes) const {
  auto* mime_data = new QMimeData();
  QByteArray encoded_data;
  QDataStream stream(&encoded_data, QIODevice::OpenModeFlag::WriteOnly);

  for (const QModelIndex& index : indexes) {
    if (index.column() != 0) {
      continue;
    }

    RootItem* item_for_index = itemForIndex(index);

    if (item_for_index->kind() != RootItem::Kind::Root) {
      stream << quintptr(item_for_index);
    }
  }

  mime_data->setData(QSL(MIME_TYPE_ITEM_POINTER), encoded_data);

  return mime_data;
}

QStringList FeedsModel::mimeTypes() const {
  return QStringList() << QSL(MIME_TYPE_ITEM_POINTER);
}

Qt::DropActions FeedsModel::supportedDropActions() const {
  return Qt::DropAction::MoveAction;
}

Qt::ItemFlags FeedsModel::flags(const QModelIndex& index) const {
  const RootItem* item_for_index = itemForIndex(index);

  Qt::ItemFlags base_flags = QAbstractItemModel::flags(index);
  Qt::ItemFlags additional_flags = item_for_index->additionalFlags();

  return base_flags | additional_flags;
}

QVariant FeedsModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation != Qt::Orientation::Horizontal) {
    return QVariant();
  }

  switch (role) {
    case Qt::ItemDataRole::DisplayRole: {
      if (section == FDS_MODEL_COUNTS_INDEX) {
        return QVariant();
      }
      else {
        return m_headerData.at(section);
      }
    }

    case Qt::ItemDataRole::EditRole:
      return m_headerData.at(section);

    case Qt::ItemDataRole::ToolTipRole:
      return m_tooltipData.at(section);

    case Qt::ItemDataRole::DecorationRole:
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

QModelIndex FeedsModel::index(int row, int column, const QModelIndex& parent) const {
  if (!hasIndex(row, column, parent)) {
    return QModelIndex();
  }

  RootItem* parent_item = itemForIndex(parent);
  RootItem* child_item = parent_item->child(row);

  if (child_item != nullptr) {
    return createIndex(row, column, child_item);
  }
  else {
    return QModelIndex();
  }
}

QModelIndex FeedsModel::parent(const QModelIndex& child) const {
  if (!child.isValid()) {
    return QModelIndex();
  }

  RootItem* child_item = itemForIndex(child);
  RootItem* parent_item = child_item->parent();

  if (parent_item == m_rootItem) {
    return QModelIndex();
  }
  else {
    return createIndex(parent_item->row(), 0, parent_item);
  }
}

int FeedsModel::rowCount(const QModelIndex& parent) const {
  if (parent.column() > 0) {
    return 0;
  }
  else {
    return itemForIndex(parent)->childCount();
  }
}

int FeedsModel::countOfAllMessages() const {
  return m_rootItem->countOfAllMessages();
}

int FeedsModel::countOfUnreadMessages() const {
  return m_rootItem->countOfUnreadMessages();
}

void FeedsModel::reloadCountsOfWholeModel() {
  m_rootItem->updateCounts();
  reloadWholeLayout();
  notifyWithCounts();
}

void FeedsModel::removeItem(RootItem* deleting_item, bool reload_counts) {
  if (deleting_item != nullptr) {
    RootItem* parent_item = deleting_item->parent();

    if (parent_item == nullptr) {
      return;
    }

    QModelIndex index = indexForItem(deleting_item);
    QModelIndex parent_index = index.parent();

    deleting_item->setDeleting(true);
    beginRemoveRows(parent_index, index.row(), index.row());
    parent_item->removeChild(deleting_item);
    endRemoveRows();

    if (reload_counts && deleting_item->kind() != RootItem::Kind::ServiceRoot) {
      deleting_item->account()->updateCounts();
    }

    delete deleting_item;

    notifyWithCounts();
  }
}

void FeedsModel::reassignNodeToNewParent(RootItem* original_node, RootItem* new_parent) {
  RootItem* original_parent = original_node->parent();

  if (original_parent != new_parent) {
    if (original_parent != nullptr) {
      int original_index_of_item = original_parent->childItems().indexOf(original_node);

      if (original_index_of_item >= 0) {
        // Remove the original item from the model...
        beginRemoveRows(indexForItem(original_parent), original_index_of_item, original_index_of_item);
        original_parent->removeChild(original_node);
        endRemoveRows();
      }
    }

    int new_index_of_item = new_parent->childCount();

    // ... and insert it under the new parent.
    beginInsertRows(indexForItem(new_parent), new_index_of_item, new_index_of_item);
    new_parent->appendChild(original_node);
    endInsertRows();
  }
}

QList<ServiceRoot*> FeedsModel::serviceRoots() const {
  QList<ServiceRoot*> roots;
  auto ch = m_rootItem->childItems();

  for (RootItem* root : std::as_const(ch)) {
    if (root->kind() == RootItem::Kind::ServiceRoot) {
      roots.append(root->toServiceRoot());
    }
  }

  return roots;
}

QList<Feed*> FeedsModel::feedsForScheduledUpdate(bool auto_update_now) {
  QList<Feed*> feeds_for_update;
  auto stf = m_rootItem->getSubTreeFeeds();
  auto cur_date = QDateTime::currentDateTimeUtc();

  for (Feed* feed : std::as_const(stf)) {
    switch (feed->autoUpdateType()) {
      case Feed::AutoUpdateType::DontAutoUpdate:
        // Do not auto-update this feed ever.
        continue;

      case Feed::AutoUpdateType::DefaultAutoUpdate:
        if (auto_update_now) {
          feeds_for_update.append(feed);
        }

        break;

      case Feed::AutoUpdateType::SpecificAutoUpdate:
      default:
        if (feed->lastUpdated().addSecs(feed->autoUpdateInterval()) < cur_date) {
          feeds_for_update.append(feed);
        }

        break;
    }
  }

  return feeds_for_update;
}

int FeedsModel::columnCount(const QModelIndex& parent) const {
  Q_UNUSED(parent)
  return FEEDS_VIEW_COLUMN_COUNT;
}

RootItem* FeedsModel::itemForIndex(const QModelIndex& index) const {
  if (index.isValid() && index.model() == this) {
    return static_cast<RootItem*>(index.internalPointer());
  }
  else {
    return m_rootItem;
  }
}

QModelIndex FeedsModel::indexForItem(const RootItem* item) const {
  if (item == nullptr || item->kind() == RootItem::Kind::Root) {
    // Root item lies on invalid index.
    return QModelIndex();
  }

  QStack<const RootItem*> chain;

  while (item->kind() != RootItem::Kind::Root) {
    chain.push(item);
    item = item->parent();
  }

  // Now, we have complete chain list: parent --- ..... --- parent --- leaf (item).
  QModelIndex target_index = indexForItem(m_rootItem);

  // We go through the stack and create our target index.
  while (!chain.isEmpty()) {
    const RootItem* parent_item = chain.pop();

    target_index =
      index(parent_item->parent()->childItems().indexOf(const_cast<RootItem* const>(parent_item)), 0, target_index);
  }

  return target_index;
}

bool FeedsModel::hasAnyFeedNewMessages() const {
  return qlinq::from(m_rootItem->getSubTreeFeeds()).any([](const Feed* feed) {
    return feed->status() == Feed::Status::NewMessages;
  });
}

RootItem* FeedsModel::rootItem() const {
  return m_rootItem;
}

void FeedsModel::setupBehaviorDuringFetching() {
  m_updateDuringFetching = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateFeedListDuringFetching)).toBool();

  if (m_updateDuringFetching) {
    m_updateItemIcon = qApp->icons()->fromTheme(QSL("view-refresh"));
  }
}

void FeedsModel::reloadChangedLayout(QModelIndexList list) {
  while (!list.isEmpty()) {
    QModelIndex indx = list.takeFirst();

    if (indx.isValid()) {
      QModelIndex indx_parent = indx.parent();

      // Underlying data are changed.
      emit dataChanged(index(indx.row(), 0, indx_parent), index(indx.row(), FDS_MODEL_COUNTS_INDEX, indx_parent));

      list.append(indx_parent);
    }
  }
}

void FeedsModel::reloadChangedItem(RootItem* item) {
  reloadChangedLayout({indexForItem(item)});
}

void FeedsModel::notifyWithCounts() {
  emit messageCountsChanged(countOfUnreadMessages(), hasAnyFeedNewMessages());
}

void FeedsModel::onItemDataChanged(const QList<RootItem*>& items) {
  if (items.size() > RELOAD_MODEL_BORDER_NUM) {
    qDebugNN << LOGSEC_FEEDMODEL << "There is request to reload feed model for more than " << RELOAD_MODEL_BORDER_NUM
             << " items, reloading model fully.";
    reloadWholeLayout();
  }
  else {
    qDebugNN << LOGSEC_FEEDMODEL << "There is request to reload feed model, reloading the " << items.size()
             << " items individually.";

    for (RootItem* item : items) {
      reloadChangedItem(item);
    }
  }

  notifyWithCounts();
}

void FeedsModel::setupFonts() {
  QFont fon;

  if (qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::CustomizeListFont)).toBool()) {
    fon.fromString(qApp->settings()
                     ->value(GROUP(Feeds), Feeds::ListFont, Application::font("FeedsView").toString())
                     .toString());
  }
  else {
    fon = Application::font("FeedsView");
  }

  m_normalFont = fon;

  m_boldFont = m_normalFont;
  m_boldFont.setBold(true);

  m_normalStrikedFont = m_normalFont;
  m_normalStrikedFont.setStrikeOut(true);

  m_boldStrikedFont = m_boldFont;
  m_boldStrikedFont.setStrikeOut(true);
}

void FeedsModel::informAboutDatabaseCleanup() {
  for (ServiceRoot* acc : serviceRoots()) {
    acc->onDatabaseCleanup();
  }
}

void FeedsModel::reloadWholeLayout() {
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

bool FeedsModel::addServiceAccount(ServiceRoot* root, bool freshly_activated) {
  int new_row_index = m_rootItem->childCount();

  beginInsertRows(indexForItem(m_rootItem), new_row_index, new_row_index);
  m_rootItem->appendChild(root);
  endInsertRows();

  // Connect.
  connect(root, &ServiceRoot::itemRemovalRequested, this, &FeedsModel::removeItem);
  connect(root, &ServiceRoot::itemReassignmentRequested, this, &FeedsModel::reassignNodeToNewParent);
  connect(root,
          &ServiceRoot::itemBlockingReassignmentRequested,
          this,
          &FeedsModel::reassignNodeToNewParent,
          Qt::ConnectionType::BlockingQueuedConnection);
  connect(root, &ServiceRoot::dataChanged, this, &FeedsModel::onItemDataChanged);
  connect(root, &ServiceRoot::dataChangeNotificationTriggered, this, &FeedsModel::dataChangeNotificationTriggered);
  connect(root, &ServiceRoot::itemExpandRequested, this, &FeedsModel::itemExpandRequested);
  connect(root, &ServiceRoot::itemExpandStateSaveRequested, this, &FeedsModel::itemExpandStateSaveRequested);

  root->start(freshly_activated);

  return true;
}

void FeedsModel::restoreAllBins() {
  auto srts = serviceRoots();

  for (ServiceRoot* root : std::as_const(srts)) {
    RecycleBin* bin_of_root = root->recycleBin();

    if (bin_of_root != nullptr) {
      bin_of_root->restore();
    }
  }
}

void FeedsModel::emptyAllBins() {
  auto srts = serviceRoots();

  for (ServiceRoot* root : std::as_const(srts)) {
    RecycleBin* bin_of_root = root->recycleBin();

    if (bin_of_root != nullptr) {
      bin_of_root->empty();
    }
  }
}

void FeedsModel::changeSortOrder(RootItem* item, bool move_top, bool move_bottom, int new_sort_order) {
  if (item == nullptr) {
    return;
  }

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::moveItem(item, move_top, move_bottom, new_sort_order, db);
  });
}

void FeedsModel::sortDirectDescendants(RootItem* item, RootItem::Kind kind_to_sort) {
  auto childs = item->childItems(kind_to_sort);

  std::sort(childs.begin(), childs.end(), [](RootItem* lhs, RootItem* rhs) {
    return lhs->title().compare(rhs->title(), Qt::CaseSensitivity::CaseInsensitive) < 0;
  });

  for (RootItem* it : childs) {
    changeSortOrder(it, false, true);
  }
}

void FeedsModel::loadActivatedServiceAccounts() {
  auto serv = qApp->feedReader()->feedServices();

  // Iterate all globally available feed "service plugins".
  for (const ServiceEntryPoint* entry_point : std::as_const(serv)) {
    // Load all stored root nodes from the entry point and add those to the model.
    QList<ServiceRoot*> roots = entry_point->initializeSubtree();

    for (ServiceRoot* root : roots) {
      addServiceAccount(root, false);
    }
  }

  if (serviceRoots().isEmpty()) {
    QTimer::singleShot(2000, qApp->mainForm(), []() {
      qApp->mainForm()->showAddAccountDialog();
    });
  }
}

void FeedsModel::stopServiceAccounts() {
  auto serv = serviceRoots();

  for (ServiceRoot* account : std::as_const(serv)) {
    account->stop();
  }
}

QList<Feed*> FeedsModel::feedsForIndex(const QModelIndex& index) const {
  return itemForIndex(index)->getSubTreeFeeds();
}

void FeedsModel::markItemRead(RootItem* item, RootItem::ReadStatus read) {
  if (item != nullptr) {
    item->markAsReadUnread(read);
  }
}

void FeedsModel::markItemCleared(RootItem* item, bool clean_read_only) {
  if (item != nullptr) {
    item->cleanMessages(clean_read_only);
  }
}

void FeedsModel::purgeArticles(const QList<Feed*>& feeds) {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::purgeFeedArticles(db, feeds);
  });

  QMultiHash<ServiceRoot*, Feed*> feeds_per_root;

  for (auto* fd : feeds) {
    feeds_per_root.insert(fd->account(), fd);
  }

  for (auto* acc : feeds_per_root.uniqueKeys()) {
    acc->onAfterFeedsPurged(feeds_per_root.values(acc));
  }

  notifyWithCounts();
  emit dataChangeNotificationTriggered(nullptr, ExternalDataChange::DatabaseCleaned);
}

QVariant FeedsModel::data(const QModelIndex& index, int role) const {
  switch (role) {
    case Qt::ItemDataRole::FontRole: {
      RootItem* it = itemForIndex(index);
      bool is_bold = it->countOfUnreadMessages() > 0;
      bool is_striked = it->kind() == RootItem::Kind::Feed && it->toFeed()->isSwitchedOff() &&
                        qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::StrikethroughDisabledFeeds)).toBool();

      return is_bold ? (is_striked ? m_boldStrikedFont : m_boldFont)
                     : (is_striked ? m_normalStrikedFont : m_normalFont);
    }

    case Qt::ItemDataRole::DecorationRole: {
      if (index.column() == FDS_MODEL_TITLE_INDEX && m_updateDuringFetching) {
        RootItem* it = itemForIndex(index);

        if (it->isFetching()) {
          return m_updateItemIcon;
        }
      }

      return itemForIndex(index)->data(index.column(), role);
    }

    case Qt::ItemDataRole::ToolTipRole:
      // NOTE: Fall-down to "default" if condition not met.
      if (!qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::EnableTooltipsFeedsMessages)).toBool()) {
        return QVariant();
      }

    default:
      return itemForIndex(index)->data(index.column(), role);
  }
}
