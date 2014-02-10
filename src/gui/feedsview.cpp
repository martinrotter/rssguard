#include "gui/feedsview.h"

#include "core/defs.h"
#include "core/systemfactory.h"
#include "core/feedsmodelfeed.h"
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/feedsmodelrootitem.h"
#include "core/feedsmodelcategory.h"
#include "core/feedsmodelstandardfeed.h"
#include "core/feedsmodelstandardcategory.h"
#include "gui/formmain.h"
#include "gui/formstandardcategorydetails.h"
#include "gui/formstandardfeeddetails.h"
#include "gui/systemtrayicon.h"
#include "gui/messagebox.h"

#include <QMenu>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QPointer>
#include <QPainter>
#include <QTimer>


FeedsView::FeedsView(QWidget *parent)
  : QTreeView(parent),
    m_contextMenuCategoriesFeeds(NULL),
    m_contextMenuEmptySpace(NULL),
    m_autoUpdateTimer(new QTimer(this)) {
  // Allocate models.
  m_proxyModel = new FeedsProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  // Timed actions.
  connect(m_autoUpdateTimer, SIGNAL(timeout()),
          this, SLOT(executeNextAutoUpdate()));

  setModel(m_proxyModel);
  setupAppearance();

  // Setup the timer.
  updateAutoUpdateStatus();
}

FeedsView::~FeedsView() {
  qDebug("Destroying FeedsView instance.");
}

void FeedsView::quit() {
  if (m_autoUpdateTimer->isActive()) {
    m_autoUpdateTimer->stop();
  }
}

void FeedsView::updateAutoUpdateStatus() {
  // Restore global intervals.
  // NOTE: Specific per-feed interval are left intact.
  m_globalAutoUpdateInitialInterval = Settings::instance()->value(APP_CFG_FEEDS, "auto_update_interval", DEFAULT_AUTO_UPDATE_INTERVAL).toInt();
  m_globalAutoUpdateRemainingInterval = m_globalAutoUpdateInitialInterval;
  m_globalAutoUpdateEnabled = Settings::instance()->value(APP_CFG_FEEDS, "auto_update_enabled", false).toBool();

  // Start global auto-update timer if it is not running yet.
  // NOTE: The timer must run even if global auto-update
  // is not enabled because user can still enable auto-update
  // for individual feeds.
  if (!m_autoUpdateTimer->isActive()) {
    m_autoUpdateTimer->setInterval(AUTO_UPDATE_INTERVAL);
    m_autoUpdateTimer->start();

    qDebug("Auto-update timer started with interval %d.", m_autoUpdateTimer->interval());
  }
}

void FeedsView::setSortingEnabled(bool enable) {
  QTreeView::setSortingEnabled(enable);
  header()->setSortIndicatorShown(false);
}

QList<FeedsModelFeed*> FeedsView::selectedFeeds() const {
  QModelIndexList selection = selectionModel()->selectedRows();
  QModelIndexList mapped_selection = m_proxyModel->mapListToSource(selection);

  return m_sourceModel->feedsForIndexes(mapped_selection);
}

QList<FeedsModelFeed*> FeedsView::allFeeds() const {
  return m_sourceModel->allFeeds();
}

FeedsModelCategory *FeedsView::isCurrentIndexCategory() const {
  QModelIndex current_mapped = m_proxyModel->mapToSource(currentIndex());
  return m_sourceModel->categoryForIndex(current_mapped);
}

FeedsModelFeed *FeedsView::isCurrentIndexFeed() const {
  QModelIndex current_mapped = m_proxyModel->mapToSource(currentIndex());
  return m_sourceModel->feedForIndex(current_mapped);
}

void FeedsView::updateAllFeeds() {
  if (SystemFactory::instance()->applicationCloseLock()->tryLock()) {
    emit feedsUpdateRequested(allFeeds());
  }
  else {
    if (SystemTrayIcon::isSystemTrayActivated()) {
      SystemTrayIcon::instance()->showMessage(tr("Cannot update all items"),
                                              tr("You cannot update all items because another feed update is ongoing."),
                                              QSystemTrayIcon::Warning);
    }
    else {
      MessageBox::show(this,
                       QMessageBox::Warning,
                       tr("Cannot update all items"),
                       tr("You cannot update all items because another feed update is ongoing."));
    }
  }
}

void FeedsView::updateAllFeedsOnStartup() {
  if (Settings::instance()->value(APP_CFG_FEEDS, "feeds_update_on_startup", false).toBool()) {
    qDebug("Requesting update for all feeds on application startup.");
    QTimer::singleShot(STARTUP_UPDATE_DELAY, this, SLOT(updateAllFeeds()));
  }
}

void FeedsView::updateSelectedFeeds() {
  if (SystemFactory::instance()->applicationCloseLock()->tryLock()) {
    emit feedsUpdateRequested(selectedFeeds());
  }
  else {
    if (SystemTrayIcon::isSystemTrayActivated()) {
      SystemTrayIcon::instance()->showMessage(tr("Cannot update selected items"),
                                              tr("You cannot update selected items because another feed update is ongoing."),
                                              QSystemTrayIcon::Warning);
    }
    else {
      MessageBox::show(this,
                       QMessageBox::Warning,
                       tr("Cannot update selected items"),
                       tr("You cannot update selected items because another feed update is ongoing."));
    }
  }
}

void FeedsView::executeNextAutoUpdate() {
  if (!SystemFactory::instance()->applicationCloseLock()->tryLock()) {
    qDebug("Delaying scheduled feed auto-updates for one minute "
           "due to another running update.");

    // Cannot update, quit.
    return;
  }

  // If global auto-update is enabled
  // and its interval counter reached zero,
  // then we need to restore it.
  if (m_globalAutoUpdateEnabled &&
      --m_globalAutoUpdateRemainingInterval < 0) {
    // We should start next auto-update interval.
    m_globalAutoUpdateRemainingInterval = m_globalAutoUpdateInitialInterval;
  }

  qDebug("Starting auto-update event, pass %d/%d.",
         m_globalAutoUpdateRemainingInterval, m_globalAutoUpdateInitialInterval);

  // Pass needed interval data and lets the model decide which feeds
  // should be updated in this pass.
  QList<FeedsModelFeed*> feeds_for_update = m_sourceModel->feedsForScheduledUpdate(m_globalAutoUpdateRemainingInterval == 0);

  if (feeds_for_update.isEmpty()) {
    // No feeds are scheduled for update now, unlock the master lock.
    SystemFactory::instance()->applicationCloseLock()->unlock();
  }
  else {
    // Request update for given feeds.
    emit feedsUpdateRequested(feeds_for_update);

    if (SystemTrayIcon::isSystemTrayActivated()) {
      SystemTrayIcon::instance()->showMessage(tr("Scheduled update started"),
                                              tr("RSS Guard is performing scheduled update of some feeds."),
                                              QSystemTrayIcon::Information);
    }
  }
}

void FeedsView::setSelectedFeedsClearStatus(int clear) {
  m_sourceModel->markFeedsDeleted(selectedFeeds(), clear, 0);
  updateCountsOfSelectedFeeds();

  emit feedsNeedToBeReloaded(1);
}

void FeedsView::setAllFeedsClearStatus(int clear) {
  m_sourceModel->markFeedsDeleted(allFeeds(), clear, 0);
  updateCountsOfAllFeeds();

  emit feedsNeedToBeReloaded(1);
}

void FeedsView::clearSelectedFeeds() {
  setSelectedFeedsClearStatus(1);
}

void FeedsView::clearAllFeeds() {
  setAllFeedsClearStatus(1);
}

void FeedsView::addNewStandardCategory() {
  if (!SystemFactory::instance()->applicationCloseLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    if (SystemTrayIcon::isSystemTrayActivated()) {
      SystemTrayIcon::instance()->showMessage(tr("Cannot add standard category"),
                                              tr("You cannot add new standard category now because feed update is ongoing."),
                                              QSystemTrayIcon::Warning);
    }
    else {
      MessageBox::show(this,
                       QMessageBox::Warning,
                       tr("Cannot add standard category"),
                       tr("You cannot add new standard category now because feed update is ongoing."));
    }

    // Thus, cannot delete and quit the method.
    return;
  }

  QPointer<FormStandardCategoryDetails> form_pointer = new FormStandardCategoryDetails(m_sourceModel, this);

  form_pointer.data()->exec(NULL);

  delete form_pointer.data();

  // Changes are done, unlock the update master lock.
  SystemFactory::instance()->applicationCloseLock()->unlock();
}

void FeedsView::editStandardCategory(FeedsModelStandardCategory *category) {
  QPointer<FormStandardCategoryDetails> form_pointer = new FormStandardCategoryDetails(m_sourceModel, this);

  form_pointer.data()->exec(category);

  delete form_pointer.data();
}

void FeedsView::addNewStandardFeed() {
  if (!SystemFactory::instance()->applicationCloseLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    if (SystemTrayIcon::isSystemTrayActivated()) {
      SystemTrayIcon::instance()->showMessage(tr("Cannot add standard feed"),
                                              tr("You cannot add new standard feed now because feed update is ongoing."),
                                              QSystemTrayIcon::Warning);
    }
    else {
      MessageBox::show(this,
                       QMessageBox::Warning,
                       tr("Cannot add standard feed"),
                       tr("You cannot add new standard feed now because feed update is ongoing."));
    }

    // Thus, cannot delete and quit the method.
    return;
  }

  QPointer<FormStandardFeedDetails> form_pointer = new FormStandardFeedDetails(m_sourceModel, this);

  form_pointer.data()->exec(NULL);

  delete form_pointer.data();

  // Changes are done, unlock the update master lock.
  SystemFactory::instance()->applicationCloseLock()->unlock();
}

void FeedsView::editStandardFeed(FeedsModelStandardFeed *feed) {
  QPointer<FormStandardFeedDetails> form_pointer = new FormStandardFeedDetails(m_sourceModel, this);

  form_pointer.data()->exec(feed);

  delete form_pointer.data();
}

void FeedsView::editSelectedItem() {
  if (!SystemFactory::instance()->applicationCloseLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    if (SystemTrayIcon::isSystemTrayActivated()) {
      SystemTrayIcon::instance()->showMessage(tr("Cannot edit item"),
                                              tr("Selected item cannot be edited because feed update is ongoing."),
                                              QSystemTrayIcon::Warning);
    }
    else {
      MessageBox::show(this,
                       QMessageBox::Warning,
                       tr("Cannot edit item"),
                       tr("Selected item cannot be edited because feed update is ongoing."));
    }

    // Thus, cannot delete and quit the method.
    return;
  }

  FeedsModelCategory *category;
  FeedsModelFeed *feed;

  if ((category = isCurrentIndexCategory()) != NULL) {
    // Category is selected.
    switch (category->type()) {
      case FeedsModelCategory::Standard: {
        // User wants to edit standard category.
        editStandardCategory(static_cast<FeedsModelStandardCategory*>(category));
        break;
      }

      default:
        break;
    }
  }
  else if ((feed = isCurrentIndexFeed()) != NULL) {
    // Feed is selected.
    switch (feed->type()) {
      case FeedsModelFeed::StandardAtom10:
      case FeedsModelFeed::StandardRdf:
      case FeedsModelFeed::StandardRss0X:
      case FeedsModelFeed::StandardRss2X: {
        // User wants to edit standard feed.
        editStandardFeed(static_cast<FeedsModelStandardFeed*>(feed));
        break;
      }

      default:
        break;
    }
  }

  // Changes are done, unlock the update master lock.
  SystemFactory::instance()->applicationCloseLock()->unlock();
}

void FeedsView::deleteSelectedItem() {
  if (!SystemFactory::instance()->applicationCloseLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    if (SystemTrayIcon::isSystemTrayActivated()) {
      SystemTrayIcon::instance()->showMessage(tr("Cannot delete item"),
                                              tr("Selected item cannot be deleted because feed update is ongoing."),
                                              QSystemTrayIcon::Warning);
    }
    else {
      MessageBox::show(this,
                       QMessageBox::Warning,
                       tr("Cannot delete item"),
                       tr("Selected item cannot be deleted because feed update is ongoing."));
    }

    // Thus, cannot delete and quit the method.
    return;
  }

  QModelIndex current_index = currentIndex();
  QItemSelectionModel *selection_model = selectionModel();

  if (!current_index.isValid()) {
    // Changes are done, unlock the update master lock and exit.
    SystemFactory::instance()->applicationCloseLock()->unlock();
    return;
  }

  if (selection_model->selectedRows().size() > 1) {
    // pridat BalloonTip z qonverteru a tady
    // ho odpalit pokud todle nastane
    // s hlaskou "More than one item selected, removing
    // only current one."
    selection_model->clearSelection();
    selection_model->select(current_index, QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
  }

  if (m_sourceModel->removeItem(m_proxyModel->mapToSource(current_index))) {
    // Item WAS removed.
  }
  else {
    // Item WAS NOT removed, either database-related error occurred
    // or update is undergoing.
  }

  // Changes are done, unlock the update master lock.
  SystemFactory::instance()->applicationCloseLock()->unlock();
}

void FeedsView::markSelectedFeedsReadStatus(int read) {
  m_sourceModel->markFeedsRead(selectedFeeds(), read);
  updateCountsOfSelectedFeeds(false);

  emit feedsNeedToBeReloaded(read);
}

void FeedsView::markSelectedFeedsRead() {
  markSelectedFeedsReadStatus(1);
}

void FeedsView::markSelectedFeedsUnread() {
  markSelectedFeedsReadStatus(0);
}

void FeedsView::markAllFeedsReadStatus(int read) {
  m_sourceModel->markFeedsRead(allFeeds(), read);
  updateCountsOfAllFeeds(false);

  emit feedsNeedToBeReloaded(read);
}

void FeedsView::markAllFeedsRead() {
  markAllFeedsReadStatus(1);
}

void FeedsView::clearAllReadMessages() {
  m_sourceModel->markFeedsDeleted(allFeeds(), 1, 1);
}

void FeedsView::openSelectedFeedsInNewspaperMode() {
  QList<Message> messages = m_sourceModel->messagesForFeeds(selectedFeeds());

  if (!messages.isEmpty()) {
    emit openMessagesInNewspaperView(messages);
    markSelectedFeedsRead();
  }
}

void FeedsView::updateCountsOfSelectedFeeds(bool update_total_too) {
  QList<FeedsModelFeed*> selected_feeds = selectedFeeds();

  if (!selected_feeds.isEmpty()) {
    foreach (FeedsModelFeed *feed, selected_feeds) {
      feed->updateCounts(update_total_too);
    }

    // Make sure that selected view reloads changed indexes.
    m_sourceModel->reloadChangedLayout(m_proxyModel->mapListToSource(selectionModel()->selectedRows()));

    notifyWithCounts();
  }
}

void FeedsView::updateCountsOfAllFeeds(bool update_total_too) {
  foreach (FeedsModelFeed *feed, allFeeds()) {
    feed->updateCounts(update_total_too);
  }

  // Make sure that all views reloads its data.
  m_sourceModel->reloadWholeLayout();

  notifyWithCounts();
}

void FeedsView::updateCountsOfParticularFeed(FeedsModelFeed *feed,
                                             bool update_total_too) {
  QModelIndex index = m_sourceModel->indexForItem(feed);

  if (index.isValid()) {
    feed->updateCounts(update_total_too);
    m_sourceModel->reloadChangedLayout(QModelIndexList() << index);
  }

  notifyWithCounts();
}

void FeedsView::initializeContextMenuCategoriesFeeds() {
  m_contextMenuCategoriesFeeds = new QMenu(tr("Context menu for feeds"), this);
  m_contextMenuCategoriesFeeds->addActions(QList<QAction*>() <<
                                           FormMain::instance()->m_ui->m_actionUpdateSelectedFeedsCategories <<
                                           FormMain::instance()->m_ui->m_actionViewSelectedItemsNewspaperMode <<
                                           FormMain::instance()->m_ui->m_actionMarkSelectedFeedsAsRead <<
                                           FormMain::instance()->m_ui->m_actionMarkSelectedFeedsAsUnread);
}

void FeedsView::initializeContextMenuEmptySpace() {
  m_contextMenuEmptySpace = new QMenu(tr("Context menu for feeds"), this);
  m_contextMenuEmptySpace->addActions(QList<QAction*>() <<
                                      FormMain::instance()->m_ui->m_actionUpdateAllFeeds);

}

void FeedsView::setupAppearance() {
#if QT_VERSION >= 0x050000
  // Setup column resize strategies.
  header()->setSectionResizeMode(FDS_MODEL_TITLE_INDEX, QHeaderView::Stretch);
  header()->setSectionResizeMode(FDS_MODEL_COUNTS_INDEX, QHeaderView::ResizeToContents);
#else
  // Setup column resize strategies.
  header()->setResizeMode(FDS_MODEL_TITLE_INDEX, QHeaderView::Stretch);
  header()->setResizeMode(FDS_MODEL_COUNTS_INDEX, QHeaderView::ResizeToContents);
#endif

  header()->setStretchLastSection(false);
  setUniformRowHeights(true);
  setAnimated(true);
  setSortingEnabled(true);
  setItemsExpandable(true);
  setExpandsOnDoubleClick(true);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setIndentation(10);
  setAcceptDrops(false);
  setDragEnabled(false);
  setDropIndicatorShown(false);
  setDragDropMode(QAbstractItemView::NoDragDrop);
  setAllColumnsShowFocus(true);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setRootIsDecorated(false);

  // Sort in ascending order, that is categories are
  // "bigger" than feeds.
  sortByColumn(0, Qt::AscendingOrder);
}

void FeedsView::selectionChanged(const QItemSelection &selected,
                                 const QItemSelection &deselected) {
  QTreeView::selectionChanged(selected, deselected);

  m_selectedFeeds.clear();

  foreach (FeedsModelFeed *feed, selectedFeeds()) {
#if defined(DEBUG)
    QModelIndex index_for_feed = m_sourceModel->indexForItem(feed);

    qDebug("Selecting feed '%s' (source index [%d, %d]).",
           qPrintable(feed->title()), index_for_feed.row(), index_for_feed.column());
#endif

    m_selectedFeeds << feed->id();
  }

  emit feedsSelected(m_selectedFeeds);
}

void FeedsView::keyPressEvent(QKeyEvent *event) {
  QTreeView::keyPressEvent(event);

  if (event->key() == Qt::Key_Delete) {
    deleteSelectedItem();
  }
}

void FeedsView::contextMenuEvent(QContextMenuEvent *event) {
  if (indexAt(event->pos()).isValid()) {
    // Display context menu for categories.
    if (m_contextMenuCategoriesFeeds == NULL) {
      // Context menu is not initialized, initialize.
      initializeContextMenuCategoriesFeeds();
    }

    m_contextMenuCategoriesFeeds->exec(event->globalPos());
  }
  else {
    // Display menu for empty space.
    if (m_contextMenuEmptySpace == NULL) {
      // Context menu is not initialized, initialize.
      initializeContextMenuEmptySpace();
    }

    m_contextMenuEmptySpace->exec(event->globalPos());
  }
}
