// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/feedsview.h"

#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "gui/styleditemdelegatewithoutfocus.h"
#include "gui/systemtrayicon.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/systemfactory.h"
#include "services/abstract/feed.h"
#include "services/abstract/rootitem.h"
#include "services/abstract/serviceroot.h"
#include "services/standard/gui/formstandardcategorydetails.h"
#include "services/standard/standardcategory.h"
#include "services/standard/standardfeed.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QTimer>

FeedsView::FeedsView(QWidget* parent)
  : QTreeView(parent), m_contextMenuService(nullptr), m_contextMenuBin(nullptr), m_contextMenuCategories(nullptr),
  m_contextMenuFeeds(nullptr), m_contextMenuImportant(nullptr), m_contextMenuEmptySpace(nullptr), m_contextMenuOtherItems(nullptr),
  m_contextMenuLabel(nullptr) {
  setObjectName(QSL("FeedsView"));

  // Allocate models.
  m_sourceModel = qApp->feedReader()->feedsModel();
  m_proxyModel = qApp->feedReader()->feedsProxyModel();

  // Connections.
  connect(m_sourceModel, &FeedsModel::requireItemValidationAfterDragDrop, this, &FeedsView::validateItemAfterDragDrop);
  connect(m_sourceModel, &FeedsModel::itemExpandRequested, this, &FeedsView::onItemExpandRequested);
  connect(m_sourceModel, &FeedsModel::itemExpandStateSaveRequested, this, &FeedsView::onItemExpandStateSaveRequested);
  connect(header(), &QHeaderView::sortIndicatorChanged, this, &FeedsView::saveSortState);
  connect(m_proxyModel, &FeedsProxyModel::expandAfterFilterIn, this, &FeedsView::expandItemDelayed);

  setModel(m_proxyModel);
  setupAppearance();
}

FeedsView::~FeedsView() {
  qDebugNN << LOGSEC_GUI << "Destroying FeedsView instance.";
}

void FeedsView::reloadFontSettings() {
  m_sourceModel->setupFonts();
}

void FeedsView::setSortingEnabled(bool enable) {
  disconnect(header(), &QHeaderView::sortIndicatorChanged, this, &FeedsView::saveSortState);
  QTreeView::setSortingEnabled(enable);
  connect(header(), &QHeaderView::sortIndicatorChanged, this, &FeedsView::saveSortState);
}

QList<Feed*> FeedsView::selectedFeeds() const {
  const QModelIndex current_index = currentIndex();

  if (current_index.isValid()) {
    return m_sourceModel->feedsForIndex(m_proxyModel->mapToSource(current_index));
  }
  else {
    return QList<Feed*>();
  }
}

RootItem* FeedsView::selectedItem() const {
  const QModelIndexList selected_rows = selectionModel()->selectedRows();

  if (selected_rows.isEmpty()) {
    return nullptr;
  }
  else {
    RootItem* selected_item = m_sourceModel->itemForIndex(m_proxyModel->mapToSource(selected_rows.at(0)));

    return selected_item == m_sourceModel->rootItem() ? nullptr : selected_item;
  }
}

void FeedsView::onItemExpandStateSaveRequested(RootItem* item) {
  saveExpandStates(item);
}

void FeedsView::saveAllExpandStates() {
  saveExpandStates(sourceModel()->rootItem());
}

void FeedsView::saveExpandStates(RootItem* item) {
  Settings* settings = qApp->settings();
  QList<RootItem*> items = item->getSubTree(RootItem::Kind::Category | RootItem::Kind::ServiceRoot);

  // Iterate all categories and save their expand statuses.
  for (const RootItem* it : items) {
    const QString setting_name = it->hashCode();
    QModelIndex source_index = sourceModel()->indexForItem(it);
    QModelIndex visible_index = model()->mapFromSource(source_index);

    settings->setValue(GROUP(CategoriesExpandStates),
                       setting_name,
                       isExpanded(visible_index));
  }
}

void FeedsView::loadAllExpandStates() {
  const Settings* settings = qApp->settings();
  QList<RootItem*> expandable_items;

  expandable_items.append(sourceModel()->rootItem()->getSubTree(RootItem::Kind::Category | RootItem::Kind::ServiceRoot));

  // Iterate all categories and save their expand statuses.
  for (const RootItem* item : expandable_items) {
    const QString setting_name = item->hashCode();

    setExpanded(model()->mapFromSource(sourceModel()->indexForItem(item)),
                settings->value(GROUP(CategoriesExpandStates), setting_name, item->childCount() > 0).toBool());
  }

  sortByColumn(qApp->settings()->value(GROUP(GUI), SETTING(GUI::DefaultSortColumnFeeds)).toInt(),
               static_cast<Qt::SortOrder>(qApp->settings()->value(GROUP(GUI), SETTING(GUI::DefaultSortOrderFeeds)).toInt()));
}

void FeedsView::copyUrlOfSelectedFeeds() const {
  auto feeds = selectedFeeds();
  QStringList urls;

  for (const auto* feed : feeds) {
    if (!feed->url().isEmpty()) {
      urls << feed->url();
    }
  }

  if (qApp->clipboard() != nullptr && !urls.isEmpty()) {
    qApp->clipboard()->setText(urls.join(TextFactory::newline()));
  }
}

void FeedsView::sortByColumn(int column, Qt::SortOrder order) {
  const int old_column = header()->sortIndicatorSection();
  const Qt::SortOrder old_order = header()->sortIndicatorOrder();

  if (column == old_column && order == old_order) {
    m_proxyModel->sort(column, order);
  }
  else {
    QTreeView::sortByColumn(column, order);
  }
}

void FeedsView::addFeedIntoSelectedAccount() {
  RootItem* selected = selectedItem();

  if (selected != nullptr) {
    ServiceRoot* root = selected->getParentServiceRoot();

    if (root->supportsFeedAdding()) {
      root->addNewFeed(selected);
    }
    else {
      qApp->showGuiMessage(tr("Not supported"),
                           tr("Selected account does not support adding of new feeds."),
                           QSystemTrayIcon::Warning,
                           qApp->mainFormWidget(), true);
    }
  }
}

void FeedsView::addCategoryIntoSelectedAccount() {
  const RootItem* selected = selectedItem();

  if (selected != nullptr) {
    ServiceRoot* root = selected->getParentServiceRoot();

    if (root->supportsCategoryAdding()) {
      root->addNewCategory(selectedItem());
    }
    else {
      qApp->showGuiMessage(tr("Not supported"),
                           tr("Selected account does not support adding of new categories."),
                           QSystemTrayIcon::Warning,
                           qApp->mainFormWidget(), true);
    }
  }
}

void FeedsView::expandCollapseCurrentItem() {
  if (selectionModel()->selectedRows().size() == 1) {
    QModelIndex index = selectionModel()->selectedRows().at(0);

    if (!model()->index(0, 0, index).isValid() && index.parent().isValid()) {
      setCurrentIndex(index.parent());
      index = index.parent();
    }

    isExpanded(index) ? collapse(index) : expand(index);
  }
}

void FeedsView::updateSelectedItems() {
  qApp->feedReader()->updateFeeds(selectedFeeds());
}

void FeedsView::clearSelectedFeeds() {
  m_sourceModel->markItemCleared(selectedItem(), false);
}

void FeedsView::clearAllFeeds() {
  m_sourceModel->markItemCleared(m_sourceModel->rootItem(), false);
}

void FeedsView::editSelectedItem() {
  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(tr("Cannot edit item"),
                         tr("Selected item cannot be edited because another critical operation is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainFormWidget(), true);

    // Thus, cannot delete and quit the method.
    return;
  }

  if (selectedItem()->canBeEdited()) {
    selectedItem()->editViaGui();
  }
  else {
    qApp->showGuiMessage(tr("Cannot edit item"),
                         tr("Selected item cannot be edited, this is not (yet?) supported."),
                         QSystemTrayIcon::Warning,
                         qApp->mainFormWidget(),
                         true);
  }

  // Changes are done, unlock the update master lock.
  qApp->feedUpdateLock()->unlock();
}

void FeedsView::deleteSelectedItem() {
  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(tr("Cannot delete item"),
                         tr("Selected item cannot be deleted because another critical operation is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainFormWidget(), true);

    // Thus, cannot delete and quit the method.
    return;
  }

  if (!currentIndex().isValid()) {
    // Changes are done, unlock the update master lock and exit.
    qApp->feedUpdateLock()->unlock();
    return;
  }

  RootItem* selected_item = selectedItem();

  if (selected_item != nullptr) {
    if (selected_item->canBeDeleted()) {
      // Ask user first.
      if (MessageBox::show(qApp->mainFormWidget(),
                           QMessageBox::Question,
                           tr("Deleting \"%1\"").arg(selected_item->title()),
                           tr("You are about to completely delete item \"%1\".").arg(selected_item->title()),
                           tr("Are you sure?"),
                           QString(), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No) {
        // User refused.
        qApp->feedUpdateLock()->unlock();
        return;
      }

      // We have deleteable item selected, remove it via GUI.
      if (!selected_item->deleteViaGui()) {
        qApp->showGuiMessage(tr("Cannot delete \"%1\"").arg(selected_item->title()),
                             tr("This item cannot be deleted because something critically failed. Submit bug report."),
                             QSystemTrayIcon::Critical,
                             qApp->mainFormWidget(),
                             true);
      }
    }
    else {
      qApp->showGuiMessage(tr("Cannot delete \"%1\"").arg(selected_item->title()),
                           tr("This item cannot be deleted, because it does not support it\nor this functionality is not implemented yet."),
                           QSystemTrayIcon::Critical,
                           qApp->mainFormWidget(),
                           true);
    }
  }

  // Changes are done, unlock the update master lock.
  qApp->feedUpdateLock()->unlock();
}

void FeedsView::markSelectedItemReadStatus(RootItem::ReadStatus read) {
  m_sourceModel->markItemRead(selectedItem(), read);
}

void FeedsView::markSelectedItemRead() {
  markSelectedItemReadStatus(RootItem::ReadStatus::Read);
}

void FeedsView::markSelectedItemUnread() {
  markSelectedItemReadStatus(RootItem::ReadStatus::Unread);
}

void FeedsView::markAllItemsReadStatus(RootItem::ReadStatus read) {
  m_sourceModel->markItemRead(m_sourceModel->rootItem(), read);
}

void FeedsView::markAllItemsRead() {
  markAllItemsReadStatus(RootItem::ReadStatus::Read);
}

void FeedsView::openSelectedItemsInNewspaperMode() {
  RootItem* selected_item = selectedItem();
  const QList<Message> messages = m_sourceModel->messagesForItem(selected_item);

  if (!messages.isEmpty()) {
    emit openMessagesInNewspaperView(selected_item, messages);
  }
}

void FeedsView::selectNextItem() {
  QModelIndex index_previous = moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);

  while (m_proxyModel->hasChildren(index_previous) && !isExpanded(index_previous)) {
    expand(index_previous);
    index_previous = moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);
  }

  if (index_previous.isValid()) {
    setCurrentIndex(index_previous);
  }

  setFocus();
}

void FeedsView::selectPreviousItem() {
  QModelIndex index_previous = moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);

  while (m_proxyModel->hasChildren(index_previous) && !isExpanded(index_previous)) {
    expand(index_previous);
    index_previous = moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);
  }

  if (index_previous.isValid()) {
    setCurrentIndex(index_previous);
  }

  setFocus();
}

void FeedsView::selectNextUnreadItem() {
  QModelIndex next_unread_row;

  if (currentIndex().isValid()) {
    next_unread_row = nextPreviousUnreadItem(currentIndex());
  }
  else {
    next_unread_row = nextPreviousUnreadItem(m_proxyModel->index(0, MSG_DB_READ_INDEX));
  }

  if (next_unread_row.isValid()) {
    setCurrentIndex(next_unread_row);
    emit requestViewNextUnreadMessage();
  }
}

QModelIndex FeedsView::nextPreviousUnreadItem(QModelIndex default_row) {
  const bool started_from_zero = default_row.row() == 0 && !default_row.parent().isValid();
  QModelIndex next_index = nextUnreadItem(default_row);

  // There is no next message, check previous.
  if (!next_index.isValid() && !started_from_zero) {
    next_index = nextUnreadItem(m_proxyModel->index(0, 0));
  }

  return next_index;
}

QModelIndex FeedsView::nextUnreadItem(QModelIndex default_row) {
  default_row = m_proxyModel->index(default_row.row(), 0, default_row.parent());
  const QModelIndex starting_row = default_row;

  while (true) {
    bool has_unread = m_sourceModel->itemForIndex(m_proxyModel->mapToSource(default_row))->countOfUnreadMessages() > 0;

    if (has_unread) {
      if (m_proxyModel->hasChildren(default_row)) {
        // Current index has unread items, but is expandable, go to first child.
        expand(default_row);
        default_row = indexBelow(default_row);
        continue;
      }
      else {
        // We found unread feed, return it.
        return default_row;
      }
    }
    else {
      QModelIndex next_row = indexBelow(default_row);

      if (next_row == default_row || !next_row.isValid() || starting_row == next_row) {
        // We came to last row probably.
        break;
      }
      else {
        default_row = next_row;
      }
    }
  }

  return QModelIndex();
}

QMenu* FeedsView::initializeContextMenuBin(RootItem* clicked_item) {
  if (m_contextMenuBin == nullptr) {
    m_contextMenuBin = new QMenu(tr("Context menu for recycle bins"), this);
  }
  else {
    m_contextMenuBin->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuFeedsList();

  m_contextMenuBin->addActions(QList<QAction*>() <<
                               qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode <<
                               qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead <<
                               qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread);

  if (!specific_actions.isEmpty()) {
    m_contextMenuBin->addSeparator();
    m_contextMenuBin->addActions(specific_actions);
  }

  return m_contextMenuBin;
}

QMenu* FeedsView::initializeContextMenuService(RootItem* clicked_item) {
  if (m_contextMenuService == nullptr) {
    m_contextMenuService = new QMenu(tr("Context menu for accounts"), this);
  }
  else {
    m_contextMenuService->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuFeedsList();

  m_contextMenuService->addActions(QList<QAction*>() <<
                                   qApp->mainForm()->m_ui->m_actionUpdateSelectedItems <<
                                   qApp->mainForm()->m_ui->m_actionEditSelectedItem <<
                                   qApp->mainForm()->m_ui->m_actionCopyUrlSelectedFeed <<
                                   qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode <<
                                   qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead <<
                                   qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread <<
                                   qApp->mainForm()->m_ui->m_actionDeleteSelectedItem);

  auto cat_add = clicked_item->getParentServiceRoot()->supportsCategoryAdding();
  auto feed_add = clicked_item->getParentServiceRoot()->supportsFeedAdding();

  if (cat_add || feed_add) {
    m_contextMenuService->addSeparator();
  }

  if (cat_add) {
    m_contextMenuService->addAction(qApp->mainForm()->m_ui->m_actionAddCategoryIntoSelectedItem);
  }

  if (feed_add) {
    m_contextMenuService->addAction(qApp->mainForm()->m_ui->m_actionAddFeedIntoSelectedItem);
  }

  if (!specific_actions.isEmpty()) {
    m_contextMenuService->addSeparator();
    m_contextMenuService->addActions(specific_actions);
  }

  return m_contextMenuService;
}

void FeedsView::switchVisibility() {
  setVisible(!isVisible());
}

void FeedsView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const {
  if (!rootIsDecorated()) {
    painter->save();
    painter->setOpacity(0.0);
  }

  QTreeView::drawBranches(painter, rect, index);

  if (!rootIsDecorated()) {
    painter->restore();
  }
}

void FeedsView::focusInEvent(QFocusEvent* event) {
  QTreeView::focusInEvent(event);

  if (currentIndex().isValid()) {
    selectionModel()->select(currentIndex(), QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);
  }
}

void FeedsView::expandItemDelayed(const QModelIndex& idx) {
  QTimer::singleShot(100, this, [=] {
    setExpanded(m_proxyModel->mapFromSource(idx), true);
  });
}

QMenu* FeedsView::initializeContextMenuCategories(RootItem* clicked_item) {
  if (m_contextMenuCategories == nullptr) {
    m_contextMenuCategories = new QMenu(tr("Context menu for categories"), this);
  }
  else {
    m_contextMenuCategories->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuFeedsList();

  m_contextMenuCategories->addActions(QList<QAction*>() <<
                                      qApp->mainForm()->m_ui->m_actionUpdateSelectedItems <<
                                      qApp->mainForm()->m_ui->m_actionEditSelectedItem <<
                                      qApp->mainForm()->m_ui->m_actionCopyUrlSelectedFeed <<
                                      qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode <<
                                      qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead <<
                                      qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread <<
                                      qApp->mainForm()->m_ui->m_actionDeleteSelectedItem);

  auto cat_add = clicked_item->getParentServiceRoot()->supportsCategoryAdding();
  auto feed_add = clicked_item->getParentServiceRoot()->supportsFeedAdding();

  if (cat_add || feed_add) {
    m_contextMenuCategories->addSeparator();
  }

  if (cat_add) {
    m_contextMenuCategories->addAction(qApp->mainForm()->m_ui->m_actionAddCategoryIntoSelectedItem);
  }

  if (feed_add) {
    m_contextMenuCategories->addAction(qApp->mainForm()->m_ui->m_actionAddFeedIntoSelectedItem);
  }

  if (!specific_actions.isEmpty()) {
    m_contextMenuCategories->addSeparator();
    m_contextMenuCategories->addActions(specific_actions);
  }

  return m_contextMenuCategories;
}

QMenu* FeedsView::initializeContextMenuFeeds(RootItem* clicked_item) {
  if (m_contextMenuFeeds == nullptr) {
    m_contextMenuFeeds = new QMenu(tr("Context menu for categories"), this);
  }
  else {
    m_contextMenuFeeds->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuFeedsList();

  m_contextMenuFeeds->addActions(QList<QAction*>() <<
                                 qApp->mainForm()->m_ui->m_actionUpdateSelectedItems <<
                                 qApp->mainForm()->m_ui->m_actionEditSelectedItem <<
                                 qApp->mainForm()->m_ui->m_actionCopyUrlSelectedFeed <<
                                 qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode <<
                                 qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead <<
                                 qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread <<
                                 qApp->mainForm()->m_ui->m_actionDeleteSelectedItem);

  auto cat_add = clicked_item->getParentServiceRoot()->supportsCategoryAdding();
  auto feed_add = clicked_item->getParentServiceRoot()->supportsFeedAdding();

  if (cat_add || feed_add) {
    m_contextMenuFeeds->addSeparator();
  }

  if (cat_add) {
    m_contextMenuFeeds->addAction(qApp->mainForm()->m_ui->m_actionAddCategoryIntoSelectedItem);
  }

  if (feed_add) {
    m_contextMenuFeeds->addAction(qApp->mainForm()->m_ui->m_actionAddFeedIntoSelectedItem);
  }

  if (!specific_actions.isEmpty()) {
    m_contextMenuFeeds->addSeparator();
    m_contextMenuFeeds->addActions(specific_actions);
  }

  return m_contextMenuFeeds;
}

QMenu* FeedsView::initializeContextMenuImportant(RootItem* clicked_item) {
  if (m_contextMenuImportant == nullptr) {
    m_contextMenuImportant = new QMenu(tr("Context menu for important messages"), this);
  }
  else {
    m_contextMenuImportant->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuFeedsList();

  m_contextMenuImportant->addActions(QList<QAction*>() <<
                                     qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode <<
                                     qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead <<
                                     qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread);

  if (!specific_actions.isEmpty()) {
    m_contextMenuImportant->addSeparator();
    m_contextMenuImportant->addActions(specific_actions);
  }

  return m_contextMenuImportant;
}

QMenu* FeedsView::initializeContextMenuEmptySpace() {
  if (m_contextMenuEmptySpace == nullptr) {
    m_contextMenuEmptySpace = new QMenu(tr("Context menu for empty space"), this);
    m_contextMenuEmptySpace->addMenu(qApp->mainForm()->m_ui->m_menuAddItem);
    m_contextMenuEmptySpace->addSeparator();
  }

  return m_contextMenuEmptySpace;
}

QMenu* FeedsView::initializeContextMenuOtherItem(RootItem* clicked_item) {
  if (m_contextMenuOtherItems == nullptr) {
    m_contextMenuOtherItems = new QMenu(tr("Context menu for other items"), this);
  }
  else {
    m_contextMenuOtherItems->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuFeedsList();

  if (!specific_actions.isEmpty()) {
    m_contextMenuOtherItems->addSeparator();
    m_contextMenuOtherItems->addActions(specific_actions);
  }
  else {
    m_contextMenuOtherItems->addAction(qApp->mainForm()->m_ui->m_actionNoActions);
  }

  return m_contextMenuOtherItems;
}

QMenu* FeedsView::initializeContextMenuLabel(RootItem* clicked_item) {
  if (m_contextMenuLabel == nullptr) {
    m_contextMenuLabel = new QMenu(tr("Context menu for label"), this);
  }
  else {
    m_contextMenuLabel->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuFeedsList();

  if (!specific_actions.isEmpty()) {
    m_contextMenuLabel->addSeparator();
    m_contextMenuLabel->addActions(specific_actions);
  }
  else {
    m_contextMenuLabel->addAction(qApp->mainForm()->m_ui->m_actionEditSelectedItem);
    m_contextMenuLabel->addAction(qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead);
    m_contextMenuLabel->addAction(qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread);
    m_contextMenuLabel->addAction(qApp->mainForm()->m_ui->m_actionDeleteSelectedItem);
  }

  return m_contextMenuLabel;
}

void FeedsView::setupAppearance() {
  // Setup column resize strategies.
  header()->setSectionResizeMode(FDS_MODEL_TITLE_INDEX, QHeaderView::Stretch);
  header()->setSectionResizeMode(FDS_MODEL_COUNTS_INDEX, QHeaderView::ResizeToContents);
  header()->setStretchLastSection(false);

  setUniformRowHeights(true);
  setAnimated(true);
  setSortingEnabled(true);
  setItemsExpandable(true);
  setAutoExpandDelay(0);
  setExpandsOnDoubleClick(true);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setIndentation(FEEDS_VIEW_INDENTATION);
  setAcceptDrops(false);
  setDragEnabled(true);
  setDropIndicatorShown(true);
  setDragDropMode(QAbstractItemView::InternalMove);
  setAllColumnsShowFocus(false);
  setRootIsDecorated(false);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setItemDelegate(new StyledItemDelegateWithoutFocus(this));
}

void FeedsView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
  RootItem* selected_item = selectedItem();

  m_proxyModel->setSelectedItem(selected_item);
  QTreeView::selectionChanged(selected, deselected);
  emit itemSelected(selected_item);

  m_proxyModel->invalidateReadFeedsFilter();

  if (!selectedIndexes().isEmpty() &&
      qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoExpandOnSelection)).toBool()) {
    expand(selectedIndexes().first());
  }
}

void FeedsView::keyPressEvent(QKeyEvent* event) {
  QTreeView::keyPressEvent(event);

  if (event->key() == Qt::Key_Delete) {
    deleteSelectedItem();
  }
}

void FeedsView::contextMenuEvent(QContextMenuEvent* event) {
  const QModelIndex clicked_index = indexAt(event->pos());

  if (clicked_index.isValid()) {
    const QModelIndex mapped_index = model()->mapToSource(clicked_index);
    RootItem* clicked_item = sourceModel()->itemForIndex(mapped_index);

    if (clicked_item->kind() == RootItem::Kind::Category) {
      // Display context menu for categories.
      initializeContextMenuCategories(clicked_item)->exec(event->globalPos());
    }
    else if (clicked_item->kind() == RootItem::Kind::Feed) {
      // Display context menu for feeds.
      initializeContextMenuFeeds(clicked_item)->exec(event->globalPos());
    }
    else if (clicked_item->kind() == RootItem::Kind::Important) {
      initializeContextMenuImportant(clicked_item)->exec(event->globalPos());
    }
    else if (clicked_item->kind() == RootItem::Kind::Bin) {
      initializeContextMenuBin(clicked_item)->exec(event->globalPos());
    }
    else if (clicked_item->kind() == RootItem::Kind::ServiceRoot) {
      initializeContextMenuService(clicked_item)->exec(event->globalPos());
    }
    else if (clicked_item->kind() == RootItem::Kind::Label) {
      initializeContextMenuLabel(clicked_item)->exec(event->globalPos());
    }
    else {
      initializeContextMenuOtherItem(clicked_item)->exec(event->globalPos());
    }
  }
  else {
    // Display menu for empty space.
    initializeContextMenuEmptySpace()->exec(event->globalPos());
  }
}

void FeedsView::mouseDoubleClickEvent(QMouseEvent* event) {
  QModelIndex idx = indexAt(event->pos());

  if (idx.isValid()) {
    RootItem* item = m_sourceModel->itemForIndex(m_proxyModel->mapToSource(idx));

    if (item->kind() == RootItem::Kind::Feed || item->kind() == RootItem::Kind::Bin) {
      const QList<Message> messages = m_sourceModel->messagesForItem(item);

      if (!messages.isEmpty()) {
        emit openMessagesInNewspaperView(item, messages);
      }
    }
  }

  QTreeView::mouseDoubleClickEvent(event);
}

void FeedsView::saveSortState(int column, Qt::SortOrder order) {
  qApp->settings()->setValue(GROUP(GUI), GUI::DefaultSortColumnFeeds, column);
  qApp->settings()->setValue(GROUP(GUI), GUI::DefaultSortOrderFeeds, order);
}

void FeedsView::validateItemAfterDragDrop(const QModelIndex& source_index) {
  const QModelIndex mapped = m_proxyModel->mapFromSource(source_index);

  if (mapped.isValid()) {
    expand(mapped);
    setCurrentIndex(mapped);
  }
}

void FeedsView::onItemExpandRequested(const QList<RootItem*>& items, bool exp) {
  for (const RootItem* item : items) {
    QModelIndex source_index = m_sourceModel->indexForItem(item);
    QModelIndex proxy_index = m_proxyModel->mapFromSource(source_index);

    setExpanded(proxy_index, exp);
  }
}
