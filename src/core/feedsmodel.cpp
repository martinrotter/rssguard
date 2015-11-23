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

#include "core/feedsmodel.h"

#include "definitions/definitions.h"
#include "services/abstract/feed.h"
#include "services/abstract/category.h"
#include "services/abstract/serviceroot.h"
#include "services/standard/standardserviceroot.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/databasecleaner.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "gui/messagebox.h"
#include "gui/statusbar.h"
#include "gui/dialogs/formmain.h"
#include "core/feeddownloader.h"

#include <QThread>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QPair>
#include <QStack>
#include <QMimeData>
#include <QTimer>

#include <algorithm>


FeedsModel::FeedsModel(QObject *parent)
  : QAbstractItemModel(parent), m_autoUpdateTimer(new QTimer(this)),
    m_feedDownloaderThread(NULL), m_feedDownloader(NULL),
    m_dbCleanerThread(NULL), m_dbCleaner(NULL) {
  setObjectName(QSL("FeedsModel"));

  // Create root item.
  m_rootItem = new RootItem();
  m_rootItem->setId(NO_PARENT_CATEGORY);

  //: Name of root item of feed list which can be seen in feed add/edit dialog.
  m_rootItem->setTitle(tr("Root"));
  m_rootItem->setIcon(qApp->icons()->fromTheme(QSL("folder-root")));

  // Setup icons.
  m_countsIcon = qApp->icons()->fromTheme(QSL("mail-mark-unread"));

  //: Title text in the feed list header.
  m_headerData << tr("Title");

  m_tooltipData << /*: Feed list header "titles" column tooltip.*/ tr("Titles of feeds/categories.") <<
                   /*: Feed list header "counts" column tooltip.*/ tr("Counts of unread/all meesages.");

  connect(m_autoUpdateTimer, SIGNAL(timeout()), this, SLOT(executeNextAutoUpdate()));

  loadActivatedServiceAccounts();
  updateAutoUpdateStatus();

  if (qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateOnStartup)).toBool()) {
    qDebug("Requesting update for all feeds on application startup.");
    QTimer::singleShot(STARTUP_UPDATE_DELAY, this, SLOT(updateAllItems()));
  }
}

FeedsModel::~FeedsModel() {
  qDebug("Destroying FeedsModel instance.");

  // Delete all model items.
  delete m_rootItem;
}

void FeedsModel::quit() {
  if (m_autoUpdateTimer->isActive()) {
    m_autoUpdateTimer->stop();
  }

  // Close worker threads.
  if (m_feedDownloaderThread != NULL && m_feedDownloaderThread->isRunning()) {
    qDebug("Quitting feed downloader thread.");
    m_feedDownloaderThread->quit();

    if (!m_feedDownloaderThread->wait(CLOSE_LOCK_TIMEOUT)) {
      qCritical("Feed downloader thread is running despite it was told to quit. Terminating it.");
      m_feedDownloaderThread->terminate();
    }
  }

  if (m_dbCleanerThread != NULL && m_dbCleanerThread->isRunning()) {
    qDebug("Quitting database cleaner thread.");
    m_dbCleanerThread->quit();

    if (!m_dbCleanerThread->wait(CLOSE_LOCK_TIMEOUT)) {
      qCritical("Database cleaner thread is running despite it was told to quit. Terminating it.");
      m_dbCleanerThread->terminate();
    }
  }

  // Close workers.
  if (m_feedDownloader != NULL) {
    qDebug("Feed downloader exists. Deleting it from memory.");
    m_feedDownloader->deleteLater();
  }

  if (m_dbCleaner != NULL) {
    qDebug("Database cleaner exists. Deleting it from memory.");
    m_dbCleaner->deleteLater();
  }

  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::ClearReadOnExit)).toBool()) {
    markItemCleared(m_rootItem, true);
  }
}

void FeedsModel::updateFeeds(const QList<Feed *> &feeds) {
  if (!qApp->feedUpdateLock()->tryLock()) {
    qApp->showGuiMessage(tr("Cannot update all items"),
                         tr("You cannot update all items because another another critical operation is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainForm(), true);
    return;
  }

  if (m_feedDownloader == NULL) {
    m_feedDownloader = new FeedDownloader();
    m_feedDownloaderThread = new QThread();

    // Downloader setup.
    qRegisterMetaType<QList<Feed*> >("QList<Feed*>");
    m_feedDownloader->moveToThread(m_feedDownloaderThread);

    connect(this, SIGNAL(feedsUpdateRequested(QList<Feed*>)), m_feedDownloader, SLOT(updateFeeds(QList<Feed*>)));
    connect(m_feedDownloaderThread, SIGNAL(finished()), m_feedDownloaderThread, SLOT(deleteLater()));
    connect(m_feedDownloader, SIGNAL(finished(FeedDownloadResults)), this, SLOT(onFeedUpdatesFinished(FeedDownloadResults)));
    connect(m_feedDownloader, SIGNAL(started()), this, SLOT(onFeedUpdatesStarted()));
    connect(m_feedDownloader, SIGNAL(progress(Feed*,int,int)), this, SLOT(onFeedUpdatesProgress(Feed*,int,int)));

    // Connections are made, start the feed downloader thread.
    m_feedDownloaderThread->start();
  }

  emit feedsUpdateRequested(feeds);
}

void FeedsModel::onFeedUpdatesStarted() {
  //: Text display in status bar when feed update is started.
  qApp->mainForm()->statusBar()->showProgressFeeds(0, tr("Feed update started"));
}

void FeedsModel::onFeedUpdatesProgress(Feed *feed, int current, int total) {
  // Some feed got updated.
  qApp->mainForm()->statusBar()->showProgressFeeds((current * 100.0) / total,
                                                   //: Text display in status bar when particular feed is updated.
                                                   tr("Updated feed '%1'").arg(feed->title()));
}

void FeedsModel::onFeedUpdatesFinished(FeedDownloadResults results) {
  qApp->feedUpdateLock()->unlock();
  qApp->mainForm()->statusBar()->clearProgressFeeds();

  if (!results.m_updatedFeeds.isEmpty()) {
    // Now, inform about results via GUI message/notification.
    qApp->showGuiMessage(tr("New messages downloaded"), results.getOverview(10), QSystemTrayIcon::NoIcon,
                         0, false, qApp->icons()->fromTheme(QSL("item-update-all")));
  }

  emit feedsUpdateFinished();
}

void FeedsModel::updateAllFeeds() {
  updateFeeds(m_rootItem->getSubTreeFeeds());
}


DatabaseCleaner *FeedsModel::databaseCleaner() {
  if (m_dbCleaner == NULL) {
    m_dbCleaner = new DatabaseCleaner();
    m_dbCleanerThread = new QThread();

    // Downloader setup.
    qRegisterMetaType<CleanerOrders>("CleanerOrders");
    m_dbCleaner->moveToThread(m_dbCleanerThread);
    connect(m_dbCleanerThread, SIGNAL(finished()), m_dbCleanerThread, SLOT(deleteLater()));

    // Connections are made, start the feed downloader thread.
    m_dbCleanerThread->start();
  }

  return m_dbCleaner;
}

void FeedsModel::executeNextAutoUpdate() {
  if (!qApp->feedUpdateLock()->tryLock()) {
    qDebug("Delaying scheduled feed auto-updates for one minute due to another running update.");

    // Cannot update, quit.
    return;
  }

  // If global auto-update is enabled and its interval counter reached zero,
  // then we need to restore it.
  if (m_globalAutoUpdateEnabled && --m_globalAutoUpdateRemainingInterval < 0) {
    // We should start next auto-update interval.
    m_globalAutoUpdateRemainingInterval = m_globalAutoUpdateInitialInterval;
  }

  qDebug("Starting auto-update event, pass %d/%d.", m_globalAutoUpdateRemainingInterval, m_globalAutoUpdateInitialInterval);

  // Pass needed interval data and lets the model decide which feeds
  // should be updated in this pass.
  QList<Feed*> feeds_for_update = feedsForScheduledUpdate(m_globalAutoUpdateEnabled && m_globalAutoUpdateRemainingInterval == 0);

  qApp->feedUpdateLock()->unlock();

  if (!feeds_for_update.isEmpty()) {
    // Request update for given feeds.
    emit feedsUpdateRequested(feeds_for_update);

    // NOTE: OSD/bubble informing about performing
    // of scheduled update can be shown now.
    qApp->showGuiMessage(tr("Starting auto-update of some feeds"),
                         tr("I will auto-update %n feed(s).", 0, feeds_for_update.size()),
                         QSystemTrayIcon::Information);
  }
}

void FeedsModel::updateAutoUpdateStatus() {
  // Restore global intervals.
  // NOTE: Specific per-feed interval are left intact.
  m_globalAutoUpdateInitialInterval = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateInterval)).toInt();
  m_globalAutoUpdateRemainingInterval = m_globalAutoUpdateInitialInterval;
  m_globalAutoUpdateEnabled = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateEnabled)).toBool();

  // Start global auto-update timer if it is not running yet.
  // NOTE: The timer must run even if global auto-update
  // is not enabled because user can still enable auto-update
  // for individual feeds.
  if (!m_autoUpdateTimer->isActive()) {
    m_autoUpdateTimer->setInterval(AUTO_UPDATE_INTERVAL);
    m_autoUpdateTimer->start();
    qDebug("Auto-update timer started with interval %d.", m_autoUpdateTimer->interval());
  }
  else {
    qDebug("Auto-update timer is already running.");
  }
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

QModelIndex FeedsModel::index(int row, int column, const QModelIndex &parent) const {
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

QModelIndex FeedsModel::parent(const QModelIndex &child) const {
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

int FeedsModel::rowCount(const QModelIndex &parent) const {
  if (parent.column() > 0) {
    return 0;
  }
  else {
    return itemForIndex(parent)->childCount();
  }
}

void FeedsModel::reloadCountsOfWholeModel() {
  m_rootItem->updateCounts(true);
  reloadWholeLayout();
  notifyWithCounts();
}

void FeedsModel::removeItem(const QModelIndex &index) {
  if (index.isValid()) {
    QModelIndex parent_index = index.parent();
    RootItem *deleting_item = itemForIndex(index);
    RootItem *parent_item = deleting_item->parent();

    // Item was persistently removed.
    // Remove it from the model.
    beginRemoveRows(parent_index, index.row(), index.row());
    parent_item->removeChild(deleting_item);
    endRemoveRows();

    delete deleting_item;
  }
}

void FeedsModel::reassignNodeToNewParent(RootItem *original_node, RootItem *new_parent) {
  RootItem *original_parent = original_node->parent();

  if (original_parent != new_parent) {
    if (original_parent != NULL) {
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

QList<ServiceRoot*> FeedsModel::serviceRoots() {
  QList<ServiceRoot*> roots;

  foreach (RootItem *root, m_rootItem->childItems()) {
    if (root->kind() == RootItemKind::ServiceRoot) {
      roots.append(root->toServiceRoot());
    }
  }

  return roots;
}

StandardServiceRoot *FeedsModel::standardServiceRoot() {
  foreach (RootItem *root, m_rootItem->childItems()) {
    StandardServiceRoot *std_service_root;

    if ((std_service_root = dynamic_cast<StandardServiceRoot*>(root)) != NULL) {
      return std_service_root;
    }
  }

  return NULL;
}

QList<Feed*> FeedsModel::feedsForScheduledUpdate(bool auto_update_now) {
  QList<Feed*> feeds_for_update;

  foreach (Feed *feed, allFeeds()) {
    switch (feed->autoUpdateType()) {
      case Feed::DontAutoUpdate:
        // Do not auto-update this feed ever.
        continue;

      case Feed::DefaultAutoUpdate:
        if (auto_update_now) {
          feeds_for_update.append(feed);
        }

        break;

      case Feed::SpecificAutoUpdate:
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

QList<Message> FeedsModel::messagesForFeeds(const QList<Feed*> &feeds) {
  QList<Message> messages;

  foreach (Feed *feed, feeds) {
    messages.append(feed->undeletedMessages());
  }

  return messages;
}

int FeedsModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)

  return FEEDS_VIEW_COLUMN_COUNT;
}

RootItem *FeedsModel::itemForIndex(const QModelIndex &index) const {
  if (index.isValid() && index.model() == this) {
    return static_cast<RootItem*>(index.internalPointer());
  }
  else {
    return m_rootItem;
  }
}

Category *FeedsModel::categoryForIndex(const QModelIndex &index) const {
  RootItem *item = itemForIndex(index);

  if (item->kind() == RootItemKind::Category) {
    return item->toCategory();
  }
  else {
    return NULL;
  }
}

QModelIndex FeedsModel::indexForItem(RootItem *item) const {
  if (item == NULL || item->kind() == RootItemKind::Root) {
    // Root item lies on invalid index.
    return QModelIndex();
  }

  QStack<RootItem*> chain;

  while (item->kind() != RootItemKind::Root) {
    chain.push(item);
    item = item->parent();
  }

  // Now, we have complete chain list: parent --- ..... --- parent --- leaf (item).
  QModelIndex target_index = indexForItem(m_rootItem);

  // We go through the stack and create our target index.
  while (!chain.isEmpty()) {
    RootItem *parent_item = chain.pop();
    target_index = index(parent_item->parent()->childItems().indexOf(parent_item), 0, target_index);
  }

  return target_index;
}

bool FeedsModel::hasAnyFeedNewMessages() {
  foreach (const Feed *feed, allFeeds()) {
    if (feed->status() == Feed::NewMessages) {
      return true;
    }
  }

  return false;
}

void FeedsModel::reloadChangedLayout(QModelIndexList list) {
  while (!list.isEmpty()) {
    QModelIndex indx = list.takeFirst();
    QModelIndex indx_parent = indx.parent();

    // Underlying data are changed.
    emit dataChanged(index(indx.row(), 0, indx_parent), index(indx.row(), FDS_MODEL_COUNTS_INDEX, indx_parent));
  }
}

void FeedsModel::reloadChangedItem(RootItem *item) {
  QModelIndex index_item = indexForItem(item);
  reloadChangedLayout(QModelIndexList() << index_item);
}

void FeedsModel::notifyWithCounts() {
  if (SystemTrayIcon::isSystemTrayActivated()) {
    qApp->trayIcon()->setNumber(countOfUnreadMessages(), hasAnyFeedNewMessages());
  }
}

void FeedsModel::onItemDataChanged(QList<RootItem*> items) {
  if (items.size() > RELOAD_MODEL_BORDER_NUM) {
    qDebug("There is request to reload feed model for more than %d items, reloading model fully.", RELOAD_MODEL_BORDER_NUM);
  }
  else {
    qDebug("There is request to reload feed model, reloading the %d items individually.", items.size());

    foreach (RootItem *item, items) {
      reloadChangedItem(item);
    }
  }

  notifyWithCounts();
}

QStringList FeedsModel::textualFeedIds(const QList<Feed*> &feeds) {
  QStringList stringy_ids;
  stringy_ids.reserve(feeds.size());

  foreach (Feed *feed, feeds) {
    stringy_ids.append(QString::number(feed->id()));
  }

  return stringy_ids;
}

void FeedsModel::reloadWholeLayout() {
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

bool FeedsModel::addServiceAccount(ServiceRoot *root) {
  m_rootItem->appendChild(root);

  // Connect.
  connect(root, SIGNAL(readFeedsFilterInvalidationRequested()), this, SIGNAL(readFeedsFilterInvalidationRequested()));
  connect(root, SIGNAL(dataChanged(QList<RootItem*>)), this, SLOT(onItemDataChanged(QList<RootItem*>)));
  connect(root, SIGNAL(reloadMessageListRequested(bool)), this, SIGNAL(reloadMessageListRequested(bool)));

  root->start();
  return true;
}

void FeedsModel::loadActivatedServiceAccounts() {
  // Iterate all globally available feed "service plugins".
  foreach (ServiceEntryPoint *entry_point, qApp->feedServices()) {
    // Load all stored root nodes from the entry point and add those to the model.
    QList<ServiceRoot*> roots = entry_point->initializeSubtree(this);

    foreach (ServiceRoot *root, roots) {
      addServiceAccount(root);
    }
  }
}

QList<Feed*> FeedsModel::feedsForIndex(const QModelIndex &index) {
  return itemForIndex(index)->getSubTreeFeeds();
}

Feed *FeedsModel::feedForIndex(const QModelIndex &index) {
  RootItem *item = itemForIndex(index);

  if (item->kind() == RootItemKind::Feed) {
    return item->toFeed();
  }
  else {
    return NULL;
  }
}

bool FeedsModel::markItemRead(RootItem *item, RootItem::ReadStatus read) {
  return item->markAsReadUnread(read);
}

bool FeedsModel::markItemCleared(RootItem *item, bool clean_read_only) {
  return item->cleanMessages(clean_read_only);
}

QList<Feed*> FeedsModel::allFeeds() {
  return m_rootItem->getSubTreeFeeds();
}

QList<Category*> FeedsModel::allCategories() {
  return m_rootItem->getSubTreeCategories();
}
