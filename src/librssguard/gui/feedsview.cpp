// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/feedsview.h"

#include "3rd-party/boolinq/boolinq.h"
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "gui/reusable/styleditemdelegatewithoutfocus.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/feed.h"
#include "services/abstract/gui/formaccountdetails.h"
#include "services/abstract/rootitem.h"
#include "services/abstract/serviceroot.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QTimer>

#include <algorithm>

FeedsView::FeedsView(QWidget* parent)
  : BaseTreeView(parent), m_contextMenuService(nullptr), m_contextMenuBin(nullptr), m_contextMenuCategories(nullptr),
    m_contextMenuFeeds(nullptr), m_contextMenuImportant(nullptr), m_contextMenuEmptySpace(nullptr),
    m_contextMenuOtherItems(nullptr), m_contextMenuLabel(nullptr), m_contextMenuProbe(nullptr),
    m_dontSaveExpandState(false) {
  setObjectName(QSL("FeedsView"));

  // Allocate models.
  m_sourceModel = qApp->feedReader()->feedsModel();
  m_proxyModel = qApp->feedReader()->feedsProxyModel();

  m_proxyModel->setView(this);

  // Connections.
  connect(m_sourceModel, &FeedsModel::itemExpandRequested, this, &FeedsView::onItemExpandRequested);
  connect(m_sourceModel, &FeedsModel::itemExpandStateSaveRequested, this, &FeedsView::onItemExpandStateSaveRequested);
  connect(header(), &QHeaderView::sortIndicatorChanged, this, &FeedsView::saveSortState);
  connect(m_proxyModel,
          &FeedsProxyModel::requireItemValidationAfterDragDrop,
          this,
          &FeedsView::validateItemAfterDragDrop);
  connect(m_proxyModel, &FeedsProxyModel::expandAfterFilterIn, this, &FeedsView::expandItemDelayed);
  connect(this, &FeedsView::expanded, this, &FeedsView::onIndexExpanded);
  connect(this, &FeedsView::collapsed, this, &FeedsView::onIndexCollapsed);

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

QList<Feed*> FeedsView::selectedFeeds(bool recursive) const {
  auto its = selectedItems();
  QList<Feed*> feeds;

  for (RootItem* it : its) {
    feeds.append(it->getSubTreeFeeds(recursive));
  }

  auto std_feeds = boolinq::from(feeds).distinct().toStdList();

  return FROM_STD_LIST(QList<Feed*>, std_feeds);
}

RootItem* FeedsView::selectedItem() const {
  const QModelIndexList selected_rows = selectionModel()->selectedRows();
  const QModelIndex current_row = currentIndex();

  if (selected_rows.isEmpty()) {
    return nullptr;
  }
  else {
    RootItem* selected_item = m_sourceModel->itemForIndex(m_proxyModel->mapToSource(selected_rows.at(0)));

    if (selected_rows.size() == 1) {
      return selected_item;
    }

    auto selected_items = boolinq::from(selected_rows)
                            .select([this](const QModelIndex& idx) {
                              return m_sourceModel->itemForIndex(m_proxyModel->mapToSource(idx));
                            })
                            .toStdList();

    RootItem* current_item = m_sourceModel->itemForIndex(m_proxyModel->mapToSource(current_row));

    if (std::find(selected_items.begin(), selected_items.end(), current_item) != selected_items.end()) {
      return current_item;
    }
    else {
      return selected_items.front();
    }
  }
}

QList<RootItem*> FeedsView::selectedItems() const {
  const QModelIndexList selected_rows = selectionModel()->selectedRows();

  auto selected_items = boolinq::from(selected_rows)
                          .select([this](const QModelIndex& idx) {
                            return m_sourceModel->itemForIndex(m_proxyModel->mapToSource(idx));
                          })
                          .toStdList();

  return FROM_STD_LIST(QList<RootItem*>, selected_items);
}

void FeedsView::copyUrlOfSelectedFeeds() const {
  auto feeds = selectedFeeds(true);
  QStringList urls;

  for (const auto* feed : feeds) {
    if (!feed->source().isEmpty()) {
      urls << feed->source();
    }
  }

  if (QGuiApplication::clipboard() != nullptr && !urls.isEmpty()) {
    QGuiApplication::clipboard()->setText(urls.join(TextFactory::newline()), QClipboard::Mode::Clipboard);
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
      root->addNewFeed(selected, QGuiApplication::clipboard()->text(QClipboard::Mode::Clipboard));
    }
    else {
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           {tr("Not supported by account"),
                            tr("Selected account does not support adding of new feeds."),
                            QSystemTrayIcon::MessageIcon::Warning});
    }
  }
}

void FeedsView::addCategoryIntoSelectedAccount() {
  RootItem* selected = selectedItem();

  if (selected != nullptr) {
    ServiceRoot* root = selected->getParentServiceRoot();

    if (root->supportsCategoryAdding()) {
      root->addNewCategory(selected);
    }
    else {
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           {tr("Not supported by account"),
                            tr("Selected account does not support adding of new categories."),
                            QSystemTrayIcon::MessageIcon::Warning});
    }
  }
}

void FeedsView::expandCollapseCurrentItem(bool recursive) {
  if (selectionModel()->selectedRows().size() == 1) {
    QModelIndex index = selectionModel()->selectedRows().at(0);

    if (!model()->index(0, 0, index).isValid() && index.parent().isValid()) {
      setCurrentIndex(index.parent());
      index = index.parent();
    }

    if (recursive) {
      QList<QModelIndex> to_process = {index};
      bool expa = !isExpanded(index);

      while (!to_process.isEmpty()) {
        auto idx = to_process.takeFirst();

        if (idx.isValid()) {
          setExpanded(idx, expa);

          for (int i = 0; i < m_proxyModel->rowCount(idx); i++) {
            auto new_idx = m_proxyModel->index(i, 0, idx);

            if (new_idx.isValid()) {
              to_process << new_idx;
            }
          }
        }
        else {
          break;
        }
      }
    }
    else {
      isExpanded(index) ? collapse(index) : expand(index);
    }
  }
}

void FeedsView::updateSelectedItems() {
  qApp->feedReader()->updateFeeds(selectedFeeds(true));
}

void FeedsView::clearSelectedItems() {
  if (MsgBox::show(nullptr,
                   QMessageBox::Icon::Question,
                   tr("Are you sure?"),
                   tr("Do you really want to clean all articles from selected items?"),
                   {},
                   {},
                   QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                   QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes) {
  }

  for (auto* it : selectedItems()) {
    m_sourceModel->markItemCleared(it, false);
  }
}

void FeedsView::clearAllItems() {
  if (MsgBox::show(nullptr,
                   QMessageBox::Icon::Question,
                   tr("Are you sure?"),
                   tr("Do you really want to clean all articles from selected items?"),
                   {},
                   {},
                   QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                   QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes) {
  }

  m_sourceModel->markItemCleared(m_sourceModel->rootItem(), false);
}

void FeedsView::editItems(const QList<RootItem*>& items) {
  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot edit item"),
                          tr("Selected item cannot be edited because another critical operation is ongoing."),
                          QSystemTrayIcon::MessageIcon::Warning});

    // Thus, cannot delete and quit the method.
    return;
  }

  if (items.isEmpty()) {
    qApp->feedUpdateLock()->unlock();
    return;
  }

  auto std_editable_items = boolinq::from(items)
                              .where([](RootItem* it) {
                                return it->canBeEdited();
                              })
                              .distinct()
                              .toStdList();

  if (std_editable_items.empty()) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot edit items"),
                          tr("Selected items cannot be edited. This is not supported (yet)."),
                          QSystemTrayIcon::MessageIcon::Critical});

    qApp->feedUpdateLock()->unlock();
    return;
  }

  if (std_editable_items.front()->kind() == RootItem::Kind::ServiceRoot && std_editable_items.size() > 1) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot edit items"),
                          tr("%1 does not support batch editing of multiple accounts.").arg(QSL(APP_NAME)),
                          QSystemTrayIcon::MessageIcon::Critical});

    qApp->feedUpdateLock()->unlock();
    return;
  }

  // We also check if items are from single account, if not we end.
  std::list<ServiceRoot*> distinct_accounts = boolinq::from(std_editable_items)
                                                .select([](RootItem* it) {
                                                  return it->getParentServiceRoot();
                                                })
                                                .distinct()
                                                .toStdList();

  if (distinct_accounts.size() != 1) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot edit items"),
                          tr("%1 does not support batch editing of items from multiple accounts.").arg(QSL(APP_NAME)),
                          QSystemTrayIcon::MessageIcon::Critical});

    qApp->feedUpdateLock()->unlock();
    return;
  }

  std::list<RootItem::Kind> distinct_types = boolinq::from(std_editable_items)
                                               .select([](RootItem* it) {
                                                 return it->kind();
                                               })
                                               .distinct()
                                               .toStdList();

  if (distinct_types.size() != 1) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot edit items"),
                          tr("%1 does not support batch editing of items of varying types.").arg(QSL(APP_NAME)),
                          QSystemTrayIcon::MessageIcon::Critical});

    qApp->feedUpdateLock()->unlock();
    return;
  }

  if (qsizetype(std_editable_items.size()) < items.size()) {
    // Some items are not editable.
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot edit some items"),
                          tr("Some of selected items cannot be edited. Proceeding to edit the rest."),
                          QSystemTrayIcon::MessageIcon::Warning});
  }

  distinct_accounts.front()->editItems(FROM_STD_LIST(QList<RootItem*>, std_editable_items));

  // Changes are done, unlock the update master lock.
  qApp->feedUpdateLock()->unlock();
}

void FeedsView::editChildFeeds() {
  auto items = selectedFeeds(false);

  if (!items.isEmpty()) {
    auto root_items = boolinq::from(items)
                        .select([](Feed* fd) {
                          return fd;
                        })
                        .toStdList();

    editItems(FROM_STD_LIST(QList<RootItem*>, root_items));
  }
}

void FeedsView::editRecursiveFeeds() {
  auto items = selectedFeeds(true);

  if (!items.isEmpty()) {
    auto root_items = boolinq::from(items)
                        .select([](Feed* fd) {
                          return fd;
                        })
                        .toStdList();

    editItems(FROM_STD_LIST(QList<RootItem*>, root_items));
  }
}

void FeedsView::editSelectedItems() {
  editItems(selectedItems());
}

void FeedsView::deleteSelectedItem() {
  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot delete item"),
                          tr("Selected item cannot be deleted because another critical operation is ongoing."),
                          QSystemTrayIcon::MessageIcon::Warning});

    // Thus, cannot delete and quit the method.
    return;
  }

  /*
  if (!currentIndex().isValid()) {
    qApp->feedUpdateLock()->unlock();
    return;
  }
  */

  QList<RootItem*> selected_items = selectedItems();
  auto std_deletable_items = boolinq::from(selected_items)
                               .where([](RootItem* it) {
                                 return it->canBeDeleted();
                               })
                               .toStdList();

  if (std_deletable_items.empty()) {
    qApp->feedUpdateLock()->unlock();
    return;
  }

  if (qsizetype(std_deletable_items.size()) < selected_items.size()) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         GuiMessage(tr("Some items won't be deleted"),
                                    tr("Some selected items will not be deleted, because they cannot be deleted."),
                                    QSystemTrayIcon::MessageIcon::Warning));
  }

  // Ask user first.
  if (MsgBox::show(qApp->mainFormWidget(),
                   QMessageBox::Icon::Question,
                   tr("Deleting %n items", nullptr, int(std_deletable_items.size())),
                   tr("You are about to completely delete %n items.", nullptr, int(std_deletable_items.size())),
                   tr("Are you sure?"),
                   QString(),
                   QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                   QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::No) {
    // User refused.
    qApp->feedUpdateLock()->unlock();
    return;
  }

  auto std_pointed_items = boolinq::from(std_deletable_items)
                             .select([](RootItem* it) {
                               return QPointer<RootItem>(it);
                             })
                             .toStdList();

  for (const QPointer<RootItem>& pnt : std_pointed_items) {
    if (pnt.isNull()) {
      continue;
    }

    if (pnt->deleteItem()) {
      m_proxyModel->invalidate();
    }
  }

  // Changes are done, unlock the update master lock.
  qApp->feedUpdateLock()->unlock();
}

void FeedsView::moveSelectedItemUp() {
  auto its = selectedItems();
  auto std_its = boolinq::from(its)
                   .orderBy([](RootItem* it) {
                     return it->sortOrder();
                   })
                   .toStdList();

  for (RootItem* it : std_its) {
    m_sourceModel->changeSortOrder(it, false, false, it->sortOrder() - 1);
  }

  m_proxyModel->invalidate();
}

void FeedsView::moveSelectedItemDown() {
  auto its = selectedItems();
  auto std_its = boolinq::from(its)
                   .orderBy([](RootItem* it) {
                     return it->sortOrder();
                   })
                   .reverse()
                   .toStdList();

  for (RootItem* it : std_its) {
    m_sourceModel->changeSortOrder(it, false, false, it->sortOrder() + 1);
  }

  m_proxyModel->invalidate();
}

void FeedsView::moveSelectedItemTop() {
  for (RootItem* it : selectedItems()) {
    m_sourceModel->changeSortOrder(it, true, false);
  }

  m_proxyModel->invalidate();
}

void FeedsView::moveSelectedItemBottom() {
  for (RootItem* it : selectedItems()) {
    m_sourceModel->changeSortOrder(it, false, true);
  }

  m_proxyModel->invalidate();
}

void FeedsView::rearrangeCategoriesOfSelectedItem() {
  for (RootItem* it : selectedItems()) {
    m_sourceModel->sortDirectDescendants(it, RootItem::Kind::Category);
  }

  m_proxyModel->invalidate();
}

void FeedsView::rearrangeFeedsOfSelectedItem() {
  for (RootItem* it : selectedItems()) {
    m_sourceModel->sortDirectDescendants(it, RootItem::Kind::Feed);
  }

  m_proxyModel->invalidate();
}

void FeedsView::markSelectedItemReadStatus(RootItem::ReadStatus read) {
  for (RootItem* it : selectedItems()) {
    m_sourceModel->markItemRead(it, read);
  }
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
  QModelIndex index_next = moveCursor(QAbstractItemView::CursorAction::MoveDown, Qt::KeyboardModifier::NoModifier);

  if (index_next.isValid()) {
    setCurrentIndex(index_next);
    scrollTo(index_next, QAbstractItemView::ScrollHint::EnsureVisible);
  }

  setFocus();
}

void FeedsView::selectPreviousItem() {
  QModelIndex index_previous = moveCursor(QAbstractItemView::CursorAction::MoveUp, Qt::KeyboardModifier::NoModifier);

  if (index_previous.isValid()) {
    setCurrentIndex(index_previous);
    scrollTo(index_previous, QAbstractItemView::ScrollHint::EnsureVisible);
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
    scrollTo(next_unread_row, QAbstractItemView::ScrollHint::EnsureVisible);
    emit requestViewNextUnreadMessage();
  }
}

QModelIndex FeedsView::nextPreviousUnreadItem(const QModelIndex& default_row) {
  const bool started_from_zero = default_row.row() == 0 && !default_row.parent().isValid();
  QModelIndex next_index = nextUnreadItem(default_row);

  // There is no next message, check previous.
  if (!next_index.isValid() && !started_from_zero) {
    next_index = nextUnreadItem(m_proxyModel->index(0, 0));
  }

  return next_index;
}

QModelIndex FeedsView::nextUnreadItem(const QModelIndex& default_row) {
  QModelIndex nconst_default_row = m_proxyModel->index(default_row.row(), 0, default_row.parent());
  const QModelIndex starting_row = default_row;

  while (true) {
    bool has_unread =
      m_sourceModel->itemForIndex(m_proxyModel->mapToSource(nconst_default_row))->countOfUnreadMessages() > 0;

    if (has_unread) {
      if (m_proxyModel->hasChildren(nconst_default_row)) {
        // Current index has unread items, but is expandable, go to first child.
        expand(nconst_default_row);
        nconst_default_row = indexBelow(nconst_default_row);
        continue;
      }
      else {
        // We found unread feed, return it.
        return nconst_default_row;
      }
    }
    else {
      QModelIndex next_row = indexBelow(nconst_default_row);

      if (next_row == nconst_default_row || !next_row.isValid() || starting_row == next_row) {
        // We came to last row probably.
        break;
      }
      else {
        nconst_default_row = next_row;
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

  m_contextMenuBin->addActions(QList<QAction*>() << qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode
                                                 << qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead
                                                 << qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread);

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

  m_contextMenuService->addActions({qApp->mainForm()->m_ui->m_actionUpdateSelectedItems,
                                    qApp->mainForm()->m_ui->m_actionEditSelectedItem,
                                    qApp->mainForm()->m_ui->m_actionEditChildFeeds,
                                    qApp->mainForm()->m_ui->m_actionEditChildFeedsRecursive,
                                    qApp->mainForm()->m_ui->m_actionCopyUrlSelectedFeed,
                                    qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode,
                                    qApp->mainForm()->m_ui->m_actionExpandCollapseItem,
                                    qApp->mainForm()->m_ui->m_actionExpandCollapseItemRecursively,
                                    qApp->mainForm()->m_ui->m_actionRearrangeCategories,
                                    qApp->mainForm()->m_ui->m_actionRearrangeFeeds,
                                    qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead,
                                    qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread,
                                    qApp->mainForm()->m_ui->m_actionDeleteSelectedItem});

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

  if (!qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::SortAlphabetically)).toBool()) {
    m_contextMenuService->addSeparator();
    m_contextMenuService->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveUp);
    m_contextMenuService->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveDown);
    m_contextMenuService->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveTop);
    m_contextMenuService->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveBottom);
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
    selectionModel()->select(currentIndex(),
                             QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);
  }
}

void FeedsView::filterItems(SearchLineEdit::SearchMode mode,
                            Qt::CaseSensitivity sensitivity,
                            int custom_criteria,
                            const QString& phrase) {
  if (!phrase.isEmpty()) {
    m_dontSaveExpandState = true;
    expandAll();
    m_dontSaveExpandState = false;
  }

  qDebugNN << LOGSEC_GUI << "Running search of feeds with pattern" << QUOTE_W_SPACE_DOT(phrase);

  switch (mode) {
    case SearchLineEdit::SearchMode::Wildcard:
      m_proxyModel->setFilterWildcard(phrase);
      break;

    case SearchLineEdit::SearchMode::RegularExpression:
      m_proxyModel->setFilterRegularExpression(phrase);
      break;

    case SearchLineEdit::SearchMode::FixedString:
    default:
      m_proxyModel->setFilterFixedString(phrase);
      break;
  }

  m_proxyModel->setFilterCaseSensitivity(sensitivity);

  FeedsToolBar::SearchFields where_search = FeedsToolBar::SearchFields(custom_criteria);

  m_proxyModel->setFilterKeyColumn(where_search == FeedsToolBar::SearchFields::SearchTitleOnly ? FDS_MODEL_TITLE_INDEX
                                                                                               : -1);

  if (phrase.isEmpty()) {
    loadAllExpandStates();
  }
}

void FeedsView::toggleFeedSortingMode(bool sort_alphabetically) {
  setSortingEnabled(sort_alphabetically);
  m_proxyModel->setSortAlphabetically(sort_alphabetically);
}

void FeedsView::onIndexExpanded(const QModelIndex& idx) {
  qDebugNN << LOGSEC_GUI << "Feed list item expanded - " << m_proxyModel->data(idx).toString();

  if (m_dontSaveExpandState) {
    qWarningNN << LOGSEC_GUI << "Don't saving expand state - " << m_proxyModel->data(idx).toString();
    return;
  }

  const RootItem* it = m_sourceModel->itemForIndex(m_proxyModel->mapToSource(idx));

  if (it != nullptr && (int(it->kind()) & int(RootItem::Kind::Category | RootItem::Kind::ServiceRoot |
                                              RootItem::Kind::Labels | RootItem::Kind::Probes)) > 0) {
    const QString setting_name = it->hashCode();

    qApp->settings()->setValue(GROUP(CategoriesExpandStates), setting_name, true);
  }
}

void FeedsView::onIndexCollapsed(const QModelIndex& idx) {
  qDebugNN << LOGSEC_GUI << "Feed list item collapsed - " << m_proxyModel->data(idx).toString();

  if (m_dontSaveExpandState) {
    qWarningNN << LOGSEC_GUI << "Don't saving collapse state - " << m_proxyModel->data(idx).toString();
    return;
  }

  RootItem* it = m_sourceModel->itemForIndex(m_proxyModel->mapToSource(idx));

  if (it != nullptr && (int(it->kind()) & int(RootItem::Kind::Category | RootItem::Kind::ServiceRoot |
                                              RootItem::Kind::Labels | RootItem::Kind::Probes)) > 0) {
    const QString setting_name = it->hashCode();

    qApp->settings()->setValue(GROUP(CategoriesExpandStates), setting_name, false);
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
  QList<RootItem*> items = item->getSubTree(RootItem::Kind::Category | RootItem::Kind::ServiceRoot |
                                            RootItem::Kind::Labels | RootItem::Kind::Probes);

  // Iterate all categories and save their expand statuses.
  for (const RootItem* it : items) {
    const QString setting_name = it->hashCode();
    QModelIndex source_index = sourceModel()->indexForItem(it);
    QModelIndex visible_index = model()->mapFromSource(source_index);

    settings->setValue(GROUP(CategoriesExpandStates), setting_name, isExpanded(visible_index));
  }
}

void FeedsView::loadAllExpandStates() {
  const Settings* settings = qApp->settings();
  QList<RootItem*> expandable_items;

  expandable_items.append(sourceModel()->rootItem()->getSubTree(RootItem::Kind::Category | RootItem::Kind::ServiceRoot |
                                                                RootItem::Kind::Labels | RootItem::Kind::Probes));

  // Iterate all categories and save their expand statuses.
  for (const RootItem* item : expandable_items) {
    const QString setting_name = item->hashCode();

    setExpanded(model()->mapFromSource(sourceModel()->indexForItem(item)),
                settings->value(GROUP(CategoriesExpandStates), setting_name, item->childCount() > 0).toBool());
  }

  sortByColumn(qApp->settings()->value(GROUP(GUI), SETTING(GUI::DefaultSortColumnFeeds)).toInt(),
               static_cast<Qt::SortOrder>(qApp->settings()
                                            ->value(GROUP(GUI), SETTING(GUI::DefaultSortOrderFeeds))
                                            .toInt()));
}

void FeedsView::expandItemDelayed(const QModelIndex& source_idx) {
  // QTimer::singleShot(100, this, [=] {
  //  Model requests to expand some items as they are visible and there is
  //  a filter active, so they maybe were not visible before.
  QModelIndex pidx = m_proxyModel->mapFromSource(source_idx);

  // NOTE: These changes are caused by filtering mechanisms
  // and we don't want to store the values.
  m_dontSaveExpandState = true;

#if QT_VERSION >= 0x050D00 // Qt >= 5.13.0
  expandRecursively(pidx);
#else
  setExpanded(pidx, true);
#endif

  m_dontSaveExpandState = false;

  //});
}

QMenu* FeedsView::initializeContextMenuCategories(RootItem* clicked_item) {
  if (m_contextMenuCategories == nullptr) {
    m_contextMenuCategories = new QMenu(tr("Context menu for categories"), this);
  }
  else {
    m_contextMenuCategories->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuFeedsList();

  m_contextMenuCategories->addActions({qApp->mainForm()->m_ui->m_actionUpdateSelectedItems,
                                       qApp->mainForm()->m_ui->m_actionEditSelectedItem,
                                       qApp->mainForm()->m_ui->m_actionEditChildFeeds,
                                       qApp->mainForm()->m_ui->m_actionEditChildFeedsRecursive,
                                       qApp->mainForm()->m_ui->m_actionCopyUrlSelectedFeed,
                                       qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode,
                                       qApp->mainForm()->m_ui->m_actionExpandCollapseItem,
                                       qApp->mainForm()->m_ui->m_actionExpandCollapseItemRecursively,
                                       qApp->mainForm()->m_ui->m_actionRearrangeCategories,
                                       qApp->mainForm()->m_ui->m_actionRearrangeFeeds,
                                       qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead,
                                       qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread,
                                       qApp->mainForm()->m_ui->m_actionDeleteSelectedItem});

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

  if (!qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::SortAlphabetically)).toBool()) {
    m_contextMenuCategories->addSeparator();
    m_contextMenuCategories->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveUp);
    m_contextMenuCategories->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveDown);
    m_contextMenuCategories->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveTop);
    m_contextMenuCategories->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveBottom);
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

  m_contextMenuFeeds->addActions(QList<QAction*>() << qApp->mainForm()->m_ui->m_actionUpdateSelectedItems
                                                   << qApp->mainForm()->m_ui->m_actionEditSelectedItem
                                                   << qApp->mainForm()->m_ui->m_actionCopyUrlSelectedFeed
                                                   << qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode
                                                   << qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead
                                                   << qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread
                                                   << qApp->mainForm()->m_ui->m_actionDeleteSelectedItem);

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

  if (!qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::SortAlphabetically)).toBool()) {
    m_contextMenuFeeds->addSeparator();
    m_contextMenuFeeds->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveUp);
    m_contextMenuFeeds->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveDown);
    m_contextMenuFeeds->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveTop);
    m_contextMenuFeeds->addAction(qApp->mainForm()->m_ui->m_actionFeedMoveBottom);
  }

  if (!specific_actions.isEmpty()) {
    m_contextMenuFeeds->addSeparator();
    m_contextMenuFeeds->addActions(specific_actions);
  }

  return m_contextMenuFeeds;
}

QMenu* FeedsView::initializeContextMenuImportant(RootItem* clicked_item) {
  if (m_contextMenuImportant == nullptr) {
    m_contextMenuImportant = new QMenu(tr("Context menu for important articles"), this);
  }
  else {
    m_contextMenuImportant->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuFeedsList();

  m_contextMenuImportant->addActions(QList<QAction*>() << qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode
                                                       << qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead
                                                       << qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread);

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

  m_contextMenuLabel->addAction(qApp->mainForm()->m_ui->m_actionEditSelectedItem);
  m_contextMenuLabel->addAction(qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead);
  m_contextMenuLabel->addAction(qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread);
  m_contextMenuLabel->addAction(qApp->mainForm()->m_ui->m_actionDeleteSelectedItem);

  if (!specific_actions.isEmpty()) {
    m_contextMenuLabel->addSeparator();
    m_contextMenuLabel->addActions(specific_actions);
  }

  return m_contextMenuLabel;
}

QMenu* FeedsView::initializeContextMenuProbe(RootItem* clicked_item) {
  if (m_contextMenuProbe == nullptr) {
    m_contextMenuProbe = new QMenu(tr("Context menu for regex query"), this);
  }
  else {
    m_contextMenuProbe->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuFeedsList();

  m_contextMenuProbe->addAction(qApp->mainForm()->m_ui->m_actionEditSelectedItem);
  m_contextMenuProbe->addAction(qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead);
  m_contextMenuProbe->addAction(qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread);
  m_contextMenuProbe->addAction(qApp->mainForm()->m_ui->m_actionDeleteSelectedItem);

  if (!specific_actions.isEmpty()) {
    m_contextMenuProbe->addSeparator();
    m_contextMenuProbe->addActions(specific_actions);
  }

  return m_contextMenuProbe;
}

void FeedsView::setupAppearance() {
  // Setup column resize strategies.
  header()->setSectionResizeMode(FDS_MODEL_TITLE_INDEX, QHeaderView::ResizeMode::Stretch);
  header()->setSectionResizeMode(FDS_MODEL_COUNTS_INDEX, QHeaderView::ResizeMode::ResizeToContents);
  header()->setStretchLastSection(false);

  setUniformRowHeights(true);
  setAnimated(true);
  setSortingEnabled(false);
  setItemsExpandable(true);
  setAutoExpandDelay(800);
  setExpandsOnDoubleClick(true);
  setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
  setIndentation(FEEDS_VIEW_INDENTATION);
  setAcceptDrops(true);
  viewport()->setAcceptDrops(true);
  setDragEnabled(true);
  setDropIndicatorShown(true);
  setDragDropMode(QAbstractItemView::DragDropMode::InternalMove);
  setAllColumnsShowFocus(false);
  setRootIsDecorated(false);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setItemDelegate(new StyledItemDelegateWithoutFocus(qApp->settings()
                                                       ->value(GROUP(GUI), SETTING(GUI::HeightRowFeeds))
                                                       .toInt(),
                                                     -1,
                                                     this));
}

void FeedsView::invalidateReadFeedsFilter(bool set_new_value, bool show_unread_only) {
  m_proxyModel->invalidateReadFeedsFilter(set_new_value, show_unread_only);
}

void FeedsView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
  RootItem* selected_item = selectedItem();

  m_proxyModel->setSelectedItem(selected_item);
  QTreeView::selectionChanged(selected, deselected);
  emit itemSelected(selected_item);

  invalidateReadFeedsFilter();

  if (!selectedIndexes().isEmpty() &&
      qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::AutoExpandOnSelection)).toBool()) {
    expand(selectedIndexes().constFirst());
  }
}

void FeedsView::keyPressEvent(QKeyEvent* event) {
  BaseTreeView::keyPressEvent(event);

  if (event->key() == Qt::Key::Key_Delete) {
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
    else if (clicked_item->kind() == RootItem::Kind::Important || clicked_item->kind() == RootItem::Kind::Unread) {
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
    else if (clicked_item->kind() == RootItem::Kind::Probe) {
      initializeContextMenuProbe(clicked_item)->exec(event->globalPos());
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

void FeedsView::drawRow(QPainter* painter, const QStyleOptionViewItem& options, const QModelIndex& index) const {
  auto opts = options;

  opts.decorationAlignment = Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignVCenter;

  BaseTreeView::drawRow(painter, opts, index);
}
