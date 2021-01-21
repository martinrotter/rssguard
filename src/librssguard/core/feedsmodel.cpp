// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feedsmodel.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/feed.h"
#include "services/abstract/recyclebin.h"
#include "services/abstract/serviceentrypoint.h"
#include "services/abstract/serviceroot.h"
#include "services/standard/standardserviceentrypoint.h"
#include "services/standard/standardserviceroot.h"

#include <QMimeData>
#include <QPair>
#include <QSqlError>
#include <QSqlRecord>
#include <QStack>
#include <QTimer>

#include <algorithm>

using RootItemPtr = RootItem*;

FeedsModel::FeedsModel(QObject* parent) : QAbstractItemModel(parent), m_itemHeight(-1) {
  setObjectName(QSL("FeedsModel"));

  // Create root item.
  m_rootItem = new RootItem();

  // : Name of root item of feed list which can be seen in feed add/edit dialog.
  m_rootItem->setTitle(tr("Root"));
  m_rootItem->setIcon(qApp->icons()->fromTheme(QSL("folder")));

  // Setup icons.
  m_countsIcon = qApp->icons()->fromTheme(QSL("mail-mark-unread"));

  // : Title text in the feed list header.
  m_headerData << tr("Title");
  m_tooltipData
    << /*: Feed list header "titles" column tooltip.*/ tr("Titles of feeds/categories.")
    << /*: Feed list header "counts" column tooltip.*/ tr("Counts of unread/all mesages.");

  setupFonts();
}

FeedsModel::~FeedsModel() {
  qDebugNN << LOGSEC_FEEDMODEL << "Destroying FeedsModel instance.";
  delete m_rootItem;
}

QMimeData* FeedsModel::mimeData(const QModelIndexList& indexes) const {
  auto* mime_data = new QMimeData();
  QByteArray encoded_data;
  QDataStream stream(&encoded_data, QIODevice::WriteOnly);

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

bool FeedsModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row,
                              int column, const QModelIndex& parent) {
  Q_UNUSED(row)
  Q_UNUSED(column)

  if (action == Qt::DropAction::IgnoreAction) {
    return true;
  }
  else if (action != Qt::DropAction::MoveAction) {
    return false;
  }

  QByteArray dragged_items_data = data->data(QSL(MIME_TYPE_ITEM_POINTER));

  if (dragged_items_data.isEmpty()) {
    return false;
  }
  else {
    QDataStream stream(&dragged_items_data, QIODevice::ReadOnly);

    while (!stream.atEnd()) {
      quintptr pointer_to_item; stream >> pointer_to_item;

      // We have item we want to drag, we also determine the target item.
      auto* dragged_item = RootItemPtr(pointer_to_item);
      RootItem* target_item = itemForIndex(parent);
      ServiceRoot* dragged_item_root = dragged_item->getParentServiceRoot();
      ServiceRoot* target_item_root = target_item->getParentServiceRoot();

      if (dragged_item == target_item || dragged_item->parent() == target_item) {
        qDebug("Dragged item is equal to target item or its parent is equal to target item. Cancelling drag-drop action.");
        return false;
      }

      if (dragged_item_root != target_item_root) {
        // Transferring of items between different accounts is not possible.
        qApp->showGuiMessage(tr("Cannot perform drag & drop operation"),
                             tr("You can't transfer dragged item into different account, this is not supported."),
                             QSystemTrayIcon::MessageIcon::Warning,
                             qApp->mainFormWidget(),
                             true);
        qDebugNN << LOGSEC_FEEDMODEL
                 << "Dragged item cannot be dragged into different account. Cancelling drag-drop action.";
        return false;
      }

      if (dragged_item->performDragDropChange(target_item)) {
        // Drag & drop is supported by the dragged item and was
        // completed on data level and in item hierarchy.
        emit requireItemValidationAfterDragDrop(indexForItem(dragged_item));
      }
    }

    return true;
  }

  return false;
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
  m_rootItem->updateCounts(true);
  reloadWholeLayout();
  notifyWithCounts();
}

void FeedsModel::removeItem(const QModelIndex& index) {
  if (index.isValid()) {
    RootItem* deleting_item = itemForIndex(index);
    QModelIndex parent_index = index.parent();
    RootItem* parent_item = deleting_item->parent();

    beginRemoveRows(parent_index, index.row(), index.row());
    parent_item->removeChild(deleting_item);
    endRemoveRows();
    deleting_item->deleteLater();
    notifyWithCounts();
  }
}

void FeedsModel::removeItem(RootItem* deleting_item) {
  if (deleting_item != nullptr) {
    QModelIndex index = indexForItem(deleting_item);
    QModelIndex parent_index = index.parent();
    RootItem* parent_item = deleting_item->parent();

    beginRemoveRows(parent_index, index.row(), index.row());
    parent_item->removeChild(deleting_item);
    endRemoveRows();
    deleting_item->deleteLater();
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

QList<ServiceRoot*>FeedsModel::serviceRoots() const {
  QList<ServiceRoot*> roots;

  for (RootItem* root : m_rootItem->childItems()) {
    if (root->kind() == RootItem::Kind::ServiceRoot) {
      roots.append(root->toServiceRoot());
    }
  }

  return roots;
}

QList<Feed*>FeedsModel::feedsForScheduledUpdate(bool auto_update_now) {
  QList<Feed*>feeds_for_update;

  for (Feed* feed : m_rootItem->getSubTreeFeeds()) {
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
        int remaining_interval = feed->autoUpdateRemainingInterval();

        if (--remaining_interval <= 0) {
          // Interval of this feed passed, include this feed in the output list
          // and reset the interval.
          feeds_for_update.append(feed);
          feed->setAutoUpdateRemainingInterval(feed->autoUpdateInitialInterval());
        }
        else {
          // Interval did not pass, set new decremented interval and do NOT
          // include this feed in the output list.
          feed->setAutoUpdateRemainingInterval(remaining_interval);
        }

        break;
    }
  }

  return feeds_for_update;
}

QList<Message> FeedsModel::messagesForItem(RootItem* item) const {
  return item->undeletedMessages();
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

    target_index = index(parent_item->parent()->childItems().indexOf(const_cast<RootItem* const>(parent_item)),
                         0,
                         target_index);
  }

  return target_index;
}

bool FeedsModel::hasAnyFeedNewMessages() const {
  return boolinq::from(m_rootItem->getSubTreeFeeds()).any([](const Feed* feed) {
    return feed->status() == Feed::Status::NewMessages;
  });
}

RootItem* FeedsModel::rootItem() const {
  return m_rootItem;
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
  reloadChangedLayout(QModelIndexList() << indexForItem(item));
}

void FeedsModel::notifyWithCounts() {
  emit messageCountsChanged(countOfUnreadMessages(), hasAnyFeedNewMessages());
}

void FeedsModel::onItemDataChanged(const QList<RootItem*>& items) {
  if (items.size() > RELOAD_MODEL_BORDER_NUM) {
    qDebugNN << LOGSEC_FEEDMODEL
             << "There is request to reload feed model for more than "
             << RELOAD_MODEL_BORDER_NUM
             << " items, reloading model fully.";
    reloadWholeLayout();
  }
  else {
    qDebugNN << LOGSEC_FEEDMODEL
             << "There is request to reload feed model, reloading the "
             << items.size()
             << " items individually.";

    for (RootItem* item : items) {
      reloadChangedItem(item);
    }
  }

  notifyWithCounts();
}

void FeedsModel::setupFonts() {
  QFont fon;

  fon.fromString(qApp->settings()->value(GROUP(Feeds), Feeds::ListFont, Application::font("FeedsView").toString()).toString());

  m_normalFont = fon;
  m_boldFont = m_normalFont;
  m_boldFont.setBold(true);

  m_itemHeight = qApp->settings()->value(GROUP(GUI), SETTING(GUI::HeightRowFeeds)).toInt();

  if (m_itemHeight > 0) {
    m_boldFont.setPixelSize(int(m_itemHeight * 0.6));
    m_normalFont.setPixelSize(int(m_itemHeight * 0.6));
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
  connect(root, &ServiceRoot::itemRemovalRequested, this, static_cast<void (FeedsModel::*)(RootItem*)>(&FeedsModel::removeItem));
  connect(root, &ServiceRoot::itemReassignmentRequested, this, &FeedsModel::reassignNodeToNewParent);
  connect(root, &ServiceRoot::dataChanged, this, &FeedsModel::onItemDataChanged);
  connect(root, &ServiceRoot::reloadMessageListRequested, this, &FeedsModel::reloadMessageListRequested);
  connect(root, &ServiceRoot::itemExpandRequested, this, &FeedsModel::itemExpandRequested);
  connect(root, &ServiceRoot::itemExpandStateSaveRequested, this, &FeedsModel::itemExpandStateSaveRequested);
  root->start(freshly_activated);

  return true;
}

bool FeedsModel::restoreAllBins() {
  bool result = true;

  for (ServiceRoot* root : serviceRoots()) {
    RecycleBin* bin_of_root = root->recycleBin();

    if (bin_of_root != nullptr) {
      result &= bin_of_root->restore();
    }
  }

  return result;
}

bool FeedsModel::emptyAllBins() {
  bool result = true;

  for (ServiceRoot* root : serviceRoots()) {
    RecycleBin* bin_of_root = root->recycleBin();

    if (bin_of_root != nullptr) {
      result &= bin_of_root->empty();
    }
  }

  return result;
}

void FeedsModel::loadActivatedServiceAccounts() {
  // Iterate all globally available feed "service plugins".
  for (const ServiceEntryPoint* entry_point : qApp->feedReader()->feedServices()) {
    // Load all stored root nodes from the entry point and add those to the model.
    QList<ServiceRoot*> roots = entry_point->initializeSubtree();

    for (ServiceRoot* root : roots) {
      addServiceAccount(root, false);
    }
  }

  if (serviceRoots().isEmpty()) {
    QTimer::singleShot(3000, qApp->mainForm(), []() {
      qApp->mainForm()->showAddAccountDialog();
    });
  }
}

void FeedsModel::stopServiceAccounts() {
  for (ServiceRoot* account : serviceRoots()) {
    account->stop();
  }
}

QList<Feed*>FeedsModel::feedsForIndex(const QModelIndex& index) const {
  return itemForIndex(index)->getSubTreeFeeds();
}

bool FeedsModel::markItemRead(RootItem* item, RootItem::ReadStatus read) {
  return item->markAsReadUnread(read);
}

bool FeedsModel::markItemCleared(RootItem* item, bool clean_read_only) {
  return item->cleanMessages(clean_read_only);
}
