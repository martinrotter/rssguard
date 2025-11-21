// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagesview.h"

#include "core/feedsmodel.h"
#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "gui/reusable/labelsmenu.h"
#include "gui/reusable/styleditemdelegate.h"
#include "gui/reusable/treeviewcolumnsmenu.h"
#include "gui/toolbars/messagestoolbar.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "network-web/webfactory.h"
#include "qnamespace.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/serviceroot.h"

#include <QClipboard>
#include <QFileIconProvider>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QProcess>
#include <QScrollBar>
#include <QTimer>

MessagesView::MessagesView(QWidget* parent)
  : BaseTreeView(parent), m_contextMenu(nullptr), m_columnsAdjusted(false), m_processingAnyMouseButton(false),
    m_processingRightMouseButton(false) {
  m_sourceModel = qApp->feedReader()->messagesModel();
  m_proxyModel = qApp->feedReader()->messagesProxyModel();
  m_sourceModel->setView(this);

  // Forward count changes to the view.
  createConnections();
  setModel(m_proxyModel);
  setupAppearance();
  setupArticleMarkingPolicy();

  header()->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(header(), &QHeaderView::customContextMenuRequested, this, [=](QPoint point) {
    TreeViewColumnsMenu mm(header());
    mm.exec(header()->mapToGlobal(point));
  });

  reloadFontSettings();
}

MessagesView::~MessagesView() {
  qDebugNN << LOGSEC_GUI << "Destroying MessagesView instance.";
}

void MessagesView::reloadFontSettings() {
  m_sourceModel->setupFonts();
}

void MessagesView::setupArticleMarkingPolicy() {
  m_articleMarkingPolicy =
    ArticleMarkingPolicy(qApp->settings()->value(GROUP(Messages), SETTING(Messages::ArticleMarkOnSelection)).toInt());
  m_articleMarkingDelay =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::ArticleMarkOnSelectionDelay)).toInt();

  m_delayedArticleMarker.setSingleShot(true);
  m_delayedArticleMarker.setInterval(m_articleMarkingDelay);
}

QByteArray MessagesView::saveHeaderState() const {
  QJsonObject obj;

  obj[QSL("header_count")] = header()->count();

  // Store column attributes.
  for (int i = 0; i < header()->count(); i++) {
    obj[QSL("header_%1_idx").arg(i)] = header()->visualIndex(i);
    obj[QSL("header_%1_size").arg(i)] = header()->sectionSize(i);
    obj[QSL("header_%1_hidden").arg(i)] = header()->isSectionHidden(i);
  }

  // Store sort attributes.
  SortColumnsAndOrders orders = m_sourceModel->sortColumnAndOrders();

  obj[QSL("sort_count")] = orders.m_columns.size();

  for (int i = 0; i < orders.m_columns.size(); i++) {
    obj[QSL("sort_%1_order").arg(i)] = orders.m_orders.at(i);
    obj[QSL("sort_%1_column").arg(i)] = orders.m_columns.at(i);
  }

  return QJsonDocument(obj).toJson(QJsonDocument::JsonFormat::Compact);
}

void MessagesView::restoreHeaderState(const QByteArray& dta) {
  QJsonObject obj = QJsonDocument::fromJson(dta).object();
  int saved_header_count = obj[QSL("header_count")].toInt();

  if (saved_header_count < header()->count()) {
    qWarningNN << LOGSEC_GUI << "Detected invalid state for article list.";
    return;
  }

  int last_visible_column = 0;

  // Restore column attributes.
  for (int i = 0; i < saved_header_count && i < header()->count(); i++) {
    int vi = obj[QSL("header_%1_idx").arg(i)].toInt();
    int ss = obj[QSL("header_%1_size").arg(i)].toInt();
    bool ish = obj[QSL("header_%1_hidden").arg(i)].toBool();

    if (vi < header()->count()) {
      header()->swapSections(header()->visualIndex(i), vi);
    }

    header()->resizeSection(i, ss);
    header()->setSectionHidden(i, ish);

    if (!ish && vi > last_visible_column) {
      last_visible_column = vi;
    }
  }

  // All columns are resizeable but last one is set to auto-stretch to fill remaining
  // space. Sometimes this column is saved as too wide and causes
  // horizontal scrollbar to appear. Therefore downsize it.
  header()->resizeSection(header()->logicalIndex(last_visible_column), 1);

  // Restore sort attributes.
  int saved_sort_count = obj[QSL("sort_count")].toInt();

  m_sourceModel->clearSortStates();

  for (int i = saved_sort_count - 1; i > 0; i--) {
    auto col = obj[QSL("sort_%1_column").arg(i)].toInt();
    auto ordr = Qt::SortOrder(obj[QSL("sort_%1_order").arg(i)].toInt());

    if (col < header()->count()) {
      m_sourceModel->addSortState(col, ordr, false);
    }
  }

  // Use newest sort as active.
  if (saved_sort_count > 0) {
    auto newest_col = obj[QSL("sort_0_column")].toInt();
    auto newest_ordr = Qt::SortOrder(obj[QSL("sort_0_order")].toInt());

    if (newest_col < header()->count()) {
      header()->setSortIndicator(newest_col, newest_ordr);
    }
  }
}

void MessagesView::goToMotherFeed(bool edit_feed_also) {
  if (selectionModel()->selectedRows().size() == 1) {
    const Message message =
      m_sourceModel->messageForRow(m_proxyModel->mapToSource(selectionModel()->selectedRows().at(0)).row());

    Feed* feed = m_sourceModel->feedById(message.m_feedId);

    if (feed != nullptr) {
      emit selectInFeedsView(feed);
    }

    if (edit_feed_also) {
      m_sourceModel->editFeedOfMessage(message);
    }
  }
}

void MessagesView::editFeedOfSelectedMessage() {
  goToMotherFeed(true);
}

void MessagesView::copyUrlOfSelectedArticles() const {
  const QModelIndexList selected_indexes = selectionModel()->selectedRows();

  if (selected_indexes.isEmpty()) {
    return;
  }

  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);
  QStringList urls;

  for (const auto article_idx : mapped_indexes) {
    urls << m_sourceModel->data(m_sourceModel->index(article_idx.row(), MSG_MDL_URL_INDEX), Qt::ItemDataRole::EditRole)
              .toString();
  }

  if (QGuiApplication::clipboard() != nullptr && !urls.isEmpty()) {
    QGuiApplication::clipboard()->setText(urls.join(TextFactory::newline()), QClipboard::Mode::Clipboard);
  }
}

void MessagesView::adjustSort(int column,
                              Qt::SortOrder order,
                              bool emit_changed_from_header,
                              bool ignore_multicolumn_sorting) {
  if (!emit_changed_from_header) {
    header()->blockSignals(true);
  }

  m_sourceModel->addSortState(column, order, ignore_multicolumn_sorting);

  header()->setSortIndicator(column, order);
  header()->blockSignals(false);
}

void MessagesView::reselectArticle(bool ensure_article_reviewed, bool do_not_modify_selection, int article_id) {
  if (article_id <= 0) {
    if (ensure_article_reviewed) {
      requestArticleHiding();
    }

    return;
  }

  QModelIndex idx = m_sourceModel->indexForMessage(article_id);
  QModelIndex idx_to_select;

  if (!idx.isValid()) {
    // Nothing to select because the source article is no longer available in the
    // source model.
  }
  else {
    idx_to_select = m_proxyModel->mapFromSource(idx);
  }

  if (idx_to_select.isValid()) {
    setCurrentIndex(idx_to_select);

    if (!do_not_modify_selection) {
      reselectIndexes({idx_to_select});
    }

    if (ensure_article_reviewed) {
      scrollTo(idx_to_select,
               !m_processingAnyMouseButton &&
                   qApp->settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool()
                 ? QAbstractItemView::ScrollHint::PositionAtCenter
                 : QAbstractItemView::ScrollHint::EnsureVisible);

      requestArticleDisplay(m_sourceModel->messageForRow(idx.row()));
    }
  }
  else {
    // The article is no longer available in the proxy or source models.
    requestArticleHiding();
  }
}

void MessagesView::onSortIndicatorChanged(int column, Qt::SortOrder order) {
  adjustSort(column, order, false, false);
  m_sourceModel->fetchInitialArticles();
  reselectArticle(true, false, m_sourceModel->additionalArticleId());
}

void MessagesView::reactOnExternalDataChange(RootItem* item, FeedsModel::ExternalDataChange cause) {
  if (m_sourceModel->loadedItem() == nullptr) {
    qWarningNN << LOGSEC_MESSAGEMODEL << "Not reacting on external article data change because nothing is selected.";
    return;
  }

  if (item != nullptr && item->account() != m_sourceModel->loadedItem()->account()) {
    qWarningNN << LOGSEC_MESSAGEMODEL
               << "Not reacting on external article data change because different account is selected.";
    return;
  }

  switch (cause) {
    case FeedsModel::ExternalDataChange::MarkedRead:
    case FeedsModel::ExternalDataChange::MarkedUnread: {
      // We refresh model data (no DB writes) to make sure the user
      // sees latest article versions.
      //
      // Only changed portions of the model are changed, all selections are retained.
      m_sourceModel->markArticleDataReadUnread(cause == FeedsModel::ExternalDataChange::MarkedRead);
      break;
    }

    case FeedsModel::ExternalDataChange::ListFilterChanged:
      reselectArticle(true, false, m_sourceModel->additionalArticleId());
      break;

    case FeedsModel::ExternalDataChange::AccountSyncedIn:
    case FeedsModel::ExternalDataChange::DatabaseCleaned:
    case FeedsModel::ExternalDataChange::RecycleBinRestored:
    case FeedsModel::ExternalDataChange::FeedFetchFinished:
    default: {
      bool articles_could_be_removed = cause == FeedsModel::ExternalDataChange::AccountSyncedIn ||
                                       cause == FeedsModel::ExternalDataChange::DatabaseCleaned ||
                                       cause == FeedsModel::ExternalDataChange::RecycleBinRestored;

      // With these external changes, some articles might actually be
      // removed. We do data re-fetch here, selection will be lost
      // but we try to re-select the same article as before.
      m_sourceModel->loadMessages(m_sourceModel->loadedItem(), !articles_could_be_removed);
      reselectArticle(true, false, m_sourceModel->additionalArticleId());
      break;
    }
  }
}

void MessagesView::createConnections() {
  connect(&m_delayedArticleMarker, &QTimer::timeout, this, &MessagesView::markSelectedMessagesReadDelayed);
  connect(this, &MessagesView::doubleClicked, this, &MessagesView::openSelectedSourceMessagesExternally);

  // Adjust columns when layout gets changed.
  connect(header(), &QHeaderView::geometriesChanged, this, &MessagesView::adjustColumns);
  connect(header(), &QHeaderView::sortIndicatorChanged, this, &MessagesView::onSortIndicatorChanged);
}

void MessagesView::keyboardSearch(const QString& search) {
  // WARNING: This is quite hacky way how to force selection of next item even
  // with extended selection enabled.
  setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  QTreeView::keyboardSearch(search);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
}

void MessagesView::setupAppearance() {
  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::MultilineArticleList)).toBool()) {
    // Enable some word wrapping for multiline list items.
    //
    // NOTE: If user explicitly changed height of rows, then respect this even if he enabled multiline support.
    setUniformRowHeights(qApp->settings()->value(GROUP(GUI), SETTING(GUI::HeightRowMessages)).toInt() > 0);
    setWordWrap(true);
    setTextElideMode(Qt::TextElideMode::ElideNone);
  }
  else {
    setUniformRowHeights(true);
    setWordWrap(false);
    setTextElideMode(Qt::TextElideMode::ElideRight);
  }

  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setAcceptDrops(false);
  setDragEnabled(false);
  setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
  setExpandsOnDoubleClick(false);
  setRootIsDecorated(false);
  setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
  setItemsExpandable(false);
  setSortingEnabled(true);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setItemDelegate(new StyledItemDelegate(qApp->settings()->value(GROUP(GUI), SETTING(GUI::HeightRowMessages)).toInt(),
                                         qApp->settings()
                                           ->value(GROUP(Messages), SETTING(Messages::ArticleListPadding))
                                           .toInt(),
                                         this));

  header()->setDefaultSectionSize(MESSAGES_VIEW_DEFAULT_COL);
  header()->setMinimumSectionSize(MESSAGES_VIEW_MINIMUM_COL);
  header()->setFirstSectionMovable(true);
  header()->setCascadingSectionResizes(false);
  header()->setStretchLastSection(true);

  adjustColumns();
}

void MessagesView::focusInEvent(QFocusEvent* event) {
  QTreeView::focusInEvent(event);

  qDebugNN << LOGSEC_GUI << "Message list got focus with reason" << QUOTE_W_SPACE_DOT(event->reason());

  if ((event->reason() == Qt::FocusReason::TabFocusReason || event->reason() == Qt::FocusReason::BacktabFocusReason ||
       event->reason() == Qt::FocusReason::ShortcutFocusReason) &&
      currentIndex().isValid()) {
    selectionModel()->select(currentIndex(),
                             QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);
  }
}

void MessagesView::keyPressEvent(QKeyEvent* event) {
  BaseTreeView::keyPressEvent(event);

  if (event->key() == Qt::Key::Key_Delete) {
    deleteSelectedMessages();
  }
  else if (event->key() == Qt::Key::Key_Backspace) {
    restoreSelectedMessages();
  }
}

void MessagesView::contextMenuEvent(QContextMenuEvent* event) {
  const QModelIndex clicked_index = indexAt(event->pos());

  if (!clicked_index.isValid()) {
    TreeViewColumnsMenu menu(header());
    menu.exec(event->globalPos());
  }
  else {
    // Context menu is not initialized, initialize.
    initializeContextMenu();
    m_contextMenu->exec(event->globalPos());
  }
}

void MessagesView::initializeContextMenu() {
  if (m_contextMenu == nullptr) {
    m_contextMenu = new QMenu(tr("Context menu for articles"), this);
  }

  m_contextMenu->clear();
  QList<Message> selected_messages;

  if (m_sourceModel->loadedItem() != nullptr) {
    QModelIndexList selected_indexes = selectionModel()->selectedRows();
    const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);
    auto rows = qlinq::from(mapped_indexes)
                  .select([](const QModelIndex& idx) {
                    return idx.row();
                  })
                  .toList();

    selected_messages = m_sourceModel->messagesAt(rows);
  }

  // External tools.
  QFileIconProvider icon_provider;
  QMenu* menu_ext_tools = new QMenu(tr("Open with external tool"), m_contextMenu);
  auto tools = ExternalTool::toolsFromSettings();

  menu_ext_tools->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

  for (const ExternalTool& tool : std::as_const(tools)) {
    QAction* act_tool =
      new QAction(QFileInfo(tool.name().simplified().isEmpty() ? tool.executable() : tool.name().simplified())
                    .fileName(),
                  menu_ext_tools);

    act_tool->setIcon(icon_provider.icon(QFileInfo(tool.executable())));
    act_tool->setToolTip(tool.executable());
    act_tool->setData(QVariant::fromValue(tool));
    menu_ext_tools->addAction(act_tool);

    connect(act_tool, &QAction::triggered, this, &MessagesView::openSelectedMessagesWithExternalTool);
  }

  if (menu_ext_tools->actions().isEmpty()) {
    QAction* act_not_tools = new QAction(tr("No external tools activated"));

    act_not_tools->setEnabled(false);
    menu_ext_tools->addAction(act_not_tools);
  }

  // Labels.
  auto labels = m_sourceModel->loadedItem() != nullptr ? m_sourceModel->loadedItem()->account()->labelsNode()->labels()
                                                       : QList<Label*>();
  LabelsMenu* menu_labels_add = new LabelsMenu(m_contextMenu);

  menu_labels_add->setMessages(selected_messages);
  menu_labels_add->setLabels(labels);

  connect(menu_labels_add, &LabelsMenu::setModelArticleLabelIds, this, &MessagesView::onArticleLabelIdsChanged);

  // Rest.
  m_contextMenu->addMenu(menu_ext_tools);
  m_contextMenu->addMenu(menu_labels_add);
  m_contextMenu->addActions({qApp->mainForm()->m_ui->m_actionSendMessageViaEmail,
                             qApp->mainForm()->m_ui->m_actionOpenSelectedSourceArticlesExternally,
                             qApp->mainForm()->m_ui->m_actionOpenSelectedMessagesInternally,
                             qApp->mainForm()->m_ui->m_actionGoToMotherFeed,
                             qApp->mainForm()->m_ui->m_actionEditFeedOfSelectedArticle,
                             qApp->mainForm()->m_ui->m_actionPlaySelectedArticlesInMediaPlayer,
                             qApp->mainForm()->m_ui->m_actionCopyUrlSelectedArticles,
                             qApp->mainForm()->m_ui->m_actionMarkSelectedMessagesAsRead,
                             qApp->mainForm()->m_ui->m_actionMarkSelectedMessagesAsUnread,
                             qApp->mainForm()->m_ui->m_actionSwitchImportanceOfSelectedMessages,
                             qApp->mainForm()->m_ui->m_actionDeleteSelectedMessages});

  if (m_sourceModel->loadedItem() != nullptr) {
    if (m_sourceModel->loadedItem()->kind() == RootItem::Kind::Bin) {
      m_contextMenu->addAction(qApp->mainForm()->m_ui->m_actionRestoreSelectedMessages);
    }

    auto extra_context_menu = m_sourceModel->loadedItem()->account()->contextMenuMessagesList(selected_messages);

    if (!extra_context_menu.isEmpty()) {
      m_contextMenu->addSeparator();
      m_contextMenu->addActions(extra_context_menu);
    }
  }
}

void MessagesView::mousePressEvent(QMouseEvent* event) {
  m_processingAnyMouseButton = true;
  m_processingRightMouseButton = event->button() == Qt::MouseButton::RightButton;

  QTreeView::mousePressEvent(event);

  m_processingAnyMouseButton = false;
  m_processingRightMouseButton = false;

  switch (event->button()) {
    case Qt::MouseButton::LeftButton: {
      // Make sure that message importance is switched when user
      // clicks the "important" column.
      const QModelIndex clicked_index = indexAt(event->pos());

      if (clicked_index.isValid()) {
        const QModelIndex mapped_index = m_proxyModel->mapToSource(clicked_index);

        if (mapped_index.column() == MSG_MDL_IMPORTANT_INDEX) {
          m_sourceModel->switchMessageImportance(mapped_index.row());
          requestArticleDisplay(m_sourceModel->messageForRow(mapped_index.row()));
        }
        else if (mapped_index.column() == MSG_MDL_READ_INDEX) {
          m_sourceModel->switchMessageReadUnread(mapped_index.row());
          requestArticleDisplay(m_sourceModel->messageForRow(mapped_index.row()));
        }
      }

      break;
    }

    case Qt::MouseButton::MiddleButton: {
      openSelectedMessagesInternally();
    }

    default:
      break;
  }
}

void MessagesView::mouseMoveEvent(QMouseEvent* event) {
  event->accept();
}

void MessagesView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
  const QModelIndexList selected_rows = selectionModel()->selectedRows();
  const QModelIndex current_index = currentIndex();
  const QModelIndex mapped_current_index = m_proxyModel->mapToSource(current_index);

  qDebugNN << LOGSEC_GUI << "Current row changed - proxy" << QUOTE_W_SPACE(current_index) << "and source"
           << QUOTE_W_SPACE_DOT(mapped_current_index);

  if (mapped_current_index.isValid() && selected_rows.size() == 1) {
    Message message = m_sourceModel->messageForRow(m_proxyModel->mapToSource(current_index).row());

    // Set this message as read only if current item
    // wasn't changed by "mark selected messages unread" action.
    if (!m_processingRightMouseButton) {
      if (!message.m_isRead) {
        if (m_articleMarkingPolicy == ArticleMarkingPolicy::MarkImmediately) {
          qDebugNN << LOGSEC_GUI << "Marking article as read immediately.";

          m_sourceModel->setMessageRead(mapped_current_index.row(), RootItem::ReadStatus::Read);
          message.m_isRead = true;
        }
        else if (m_articleMarkingPolicy == ArticleMarkingPolicy::MarkWithDelay) {
          qDebugNN << LOGSEC_GUI << "(Re)Starting timer to mark article as read with a delay.";
          m_delayedArticleIndex = current_index;
          m_delayedArticleMarker.start();
        }
        else {
          // NOTE: Article can only be marked as read manually, so just change.
        }
      }
    }

    requestArticleDisplay(message);
  }
  else {
    requestArticleHiding();
  }

  if (selected_rows.isEmpty()) {
    setCurrentIndex({});
  }

  if (!m_processingAnyMouseButton && currentIndex().isValid() &&
      qApp->settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool()) {
    scrollTo(currentIndex(), QAbstractItemView::ScrollHint::PositionAtCenter);
  }

  QTreeView::selectionChanged(selected, deselected);
}

void MessagesView::onArticleLabelIdsChanged(const QList<Message>& msgs) {
  m_sourceModel->setMessageLabelsById(msgs);
  QModelIndex current_index = selectionModel()->currentIndex();

  if (current_index.isValid()) {
    requestArticleDisplay(m_sourceModel->messageForRow(m_proxyModel->mapToSource(current_index).row()));
  }
  else {
    requestArticleHiding();
  }
}

void MessagesView::requestArticleDisplay(const Message& msg) {
  m_sourceModel->setAdditionalArticleId(msg.m_id);
  emit currentMessageChanged(msg, m_sourceModel->loadedItem());
}

void MessagesView::requestArticleHiding() {
  m_sourceModel->setAdditionalArticleId(0);
  emit currentMessageRemoved(m_sourceModel->loadedItem());
}

void MessagesView::markSelectedMessagesReadDelayed() {
  qDebugNN << LOGSEC_GUI << "Delay has passed! Marking article as read NOW.";

  const QModelIndexList selected_rows = selectionModel()->selectedRows();
  const QModelIndex current_index = m_delayedArticleIndex;

  if (selected_rows.size() == 1 && current_index.isValid() && !m_processingRightMouseButton &&
      m_articleMarkingPolicy == ArticleMarkingPolicy::MarkWithDelay) {
    const QModelIndex mapped_current_index = m_proxyModel->mapToSource(current_index);
    Message message = m_sourceModel->messageForRow(m_proxyModel->mapToSource(current_index).row());

    m_sourceModel->setMessageRead(mapped_current_index.row(), RootItem::ReadStatus::Read);
    message.m_isRead = true;

    requestArticleDisplay(message);
  }
}

void MessagesView::loadItem(RootItem* item) {
  m_delayedArticleMarker.stop();

  const int column = header()->sortIndicatorSection();
  const Qt::SortOrder order = header()->sortIndicatorOrder();

  scrollToTop();
  clearSelection();
  adjustSort(column, order, false, true);

  m_sourceModel->loadMessages(item);

  bool switch_entire_rtl_list =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::SwitchArticleListRtl)).toBool();

  if (switch_entire_rtl_list && item != nullptr) {
    if (item->kind() == RootItem::Kind::Feed) {
      auto* fd = item->toFeed();
      setLayoutDirection((fd->rtlBehavior() == RtlBehavior::Everywhere ||
                          fd->rtlBehavior() == RtlBehavior::EverywhereExceptFeedList)
                           ? Qt::LayoutDirection::RightToLeft
                           : Qt::LayoutDirection::LayoutDirectionAuto);
    }
    else {
      auto fds = item->getSubTreeFeeds();
      bool all_feeds_rtl = !fds.isEmpty() && std::all_of(fds.begin(), fds.end(), [](Feed* fd) {
        return fd->rtlBehavior() == RtlBehavior::Everywhere ||
               fd->rtlBehavior() == RtlBehavior::EverywhereExceptFeedList;
      });

      setLayoutDirection(all_feeds_rtl ? Qt::LayoutDirection::RightToLeft : Qt::LayoutDirection::LayoutDirectionAuto);
    }
  }
  else {
    setLayoutDirection(Qt::LayoutDirection::LayoutDirectionAuto);
  }

  requestArticleHiding();
}

void MessagesView::changeFilter(MessagesProxyModel::MessageListFilter filter) {
  m_proxyModel->setMessageListFilter(filter);

  // NOTE: Proxy model is normally not dynamically filtered, but when user changes filter
  // explicitly, then yes, we need to do something.
  m_proxyModel->invalidate();

  reactOnExternalDataChange(m_sourceModel->loadedItem(), FeedsModel::ExternalDataChange::ListFilterChanged);
}

void MessagesView::openSelectedSourceMessagesExternally() {
  auto rws = selectionModel()->selectedRows();

  for (const QModelIndex& index : std::as_const(rws)) {
    QString link = m_sourceModel->messageForRow(m_proxyModel->mapToSource(index).row())
                     .m_url.replace(QRegularExpression(QSL("[\\t\\n]")), QString());

    qApp->web()->openUrlInExternalBrowser(link);
  }

  if (qApp->settings()
        ->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally))
        .toBool()) {
    QTimer::singleShot(1000, this, []() {
      qApp->mainForm()->display();
    });
  }
}

#if defined(ENABLE_MEDIAPLAYER)
void MessagesView::playSelectedArticleInMediaPlayer() {
  auto rws = selectionModel()->selectedRows();

  if (!rws.isEmpty()) {
    auto msg = m_sourceModel->messageForRow(m_proxyModel->mapToSource(rws.first()).row());

    if (msg.m_url.isEmpty()) {
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           GuiMessage(tr("No URL"),
                                      tr("Article cannot be played in media player as it has no URL"),
                                      QSystemTrayIcon::MessageIcon::Warning),
                           GuiMessageDestination(true, true));
    }
    else {
      emit playLinkInMediaPlayer(msg.m_url);
    }
  }
}
#endif

void MessagesView::openSelectedMessagesInternally() {
  auto rws = selectionModel()->selectedRows();

  if (!rws.isEmpty()) {
    auto msg = m_sourceModel->messageForRow(m_proxyModel->mapToSource(rws.first()).row());

    emit openSingleMessageInNewTab(m_sourceModel->loadedItem(), msg);
  }
}

void MessagesView::sendSelectedMessageViaEmail() {
  if (selectionModel()->selectedRows().size() == 1) {
    const Message message =
      m_sourceModel->messageForRow(m_proxyModel->mapToSource(selectionModel()->selectedRows().at(0)).row());

    if (!qApp->web()->sendMessageViaEmail(message)) {
      MsgBox::show(this,
                   QMessageBox::Critical,
                   tr("Problem with starting external e-mail client"),
                   tr("External e-mail client could not be started."));
    }
  }
}

void MessagesView::markSelectedMessagesRead() {
  setSelectedMessagesReadStatus(RootItem::ReadStatus::Read);
}

void MessagesView::markSelectedMessagesUnread() {
  setSelectedMessagesReadStatus(RootItem::ReadStatus::Unread);
}

void MessagesView::setSelectedMessagesReadStatus(RootItem::ReadStatus read) {
  const QModelIndexList selected_indexes = selectionModel()->selectedRows();

  if (selected_indexes.isEmpty()) {
    return;
  }

  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesRead(mapped_indexes, read);
  QModelIndex current_index = selectionModel()->currentIndex();

  if (current_index.isValid() && selected_indexes.size() == 1) {
    requestArticleDisplay(m_sourceModel->messageForRow(m_proxyModel->mapToSource(current_index).row()));
  }
  else {
    requestArticleHiding();
  }
}

void MessagesView::deleteSelectedMessages() {
  const QModelIndexList selected_indexes = selectionModel()->selectedRows();

  if (selected_indexes.isEmpty()) {
    return;
  }

  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesDeleted(mapped_indexes);
  QModelIndex current_index =
    currentIndex().isValid() ? moveCursor(QAbstractItemView::CursorAction::MoveDown, Qt::KeyboardModifier::NoModifier)
                             : currentIndex();

  if (current_index.isValid() && selected_indexes.size() == 1) {
    setCurrentIndex(current_index);
  }
  else {
    requestArticleHiding();
  }
}

void MessagesView::restoreSelectedMessages() {
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  const QModelIndexList selected_indexes = selectionModel()->selectedRows();
  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesRestored(mapped_indexes);
  current_index = m_proxyModel->index(current_index.row(), current_index.column());

  if (current_index.isValid()) {
    requestArticleDisplay(m_sourceModel->messageForRow(m_proxyModel->mapToSource(current_index).row()));
  }
  else {
    requestArticleHiding();
  }
}

void MessagesView::switchSelectedMessagesImportance() {
  const QModelIndexList selected_indexes = selectionModel()->selectedRows();

  if (selected_indexes.isEmpty()) {
    return;
  }

  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->switchBatchMessageImportance(mapped_indexes);
  QModelIndex current_index = selectionModel()->currentIndex();

  if (current_index.isValid() && selected_indexes.size() == 1) {
    requestArticleDisplay(m_sourceModel->messageForRow(m_proxyModel->mapToSource(current_index).row()));
  }
  else {
    requestArticleHiding();
  }
}

void MessagesView::reselectIndexes(const QModelIndexList& indexes) {
  if (indexes.size() < RESELECT_MESSAGE_THRESSHOLD) {
    QItemSelection selection;

    for (const QModelIndex& index : indexes) {
      selection.merge(QItemSelection(index, index), QItemSelectionModel::Select);
    }

    selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  }
}

void MessagesView::selectNextItem() {
  selectItemWithCursorAction(QAbstractItemView::CursorAction::MoveDown);
}

void MessagesView::selectPreviousItem() {
  selectItemWithCursorAction(QAbstractItemView::CursorAction::MoveUp);
}

void MessagesView::selectItemWithCursorAction(CursorAction act) {
  const QModelIndex index_previous = moveCursor(act, Qt::KeyboardModifier::NoModifier);

  if (index_previous.isValid()) {
    setCurrentIndex(index_previous);
    setFocus();
  }
}

void MessagesView::selectNextUnreadItem() {
  const QModelIndexList selected_rows = selectionModel()->selectedRows();
  int active_row;

  if (!selected_rows.isEmpty()) {
    // Okay, something is selected, start from it.
    active_row = selected_rows.at(0).row();
  }
  else {
    active_row = 0;
  }

  const QModelIndex next_unread = m_proxyModel->getNextPreviousUnreadItemIndex(active_row);

  if (next_unread.isValid()) {
    setCurrentIndex(next_unread);
    setFocus();
  }
}

void MessagesView::searchMessages(SearchLineEdit::SearchMode mode,
                                  Qt::CaseSensitivity sensitivity,
                                  int custom_criteria,
                                  const QString& phrase) {
  qDebugNN << LOGSEC_GUI << "Running search of messages with pattern" << QUOTE_W_SPACE_DOT(phrase);

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

  BaseToolBar::SearchFields where_search = BaseToolBar::SearchFields(custom_criteria);

  m_proxyModel->setFilterKeyColumn(where_search == BaseToolBar::SearchFields::SearchTitleOnly ? MSG_MDL_TITLE_INDEX
                                                                                              : -1);

  if (selectionModel()->selectedRows().isEmpty()) {
    requestArticleHiding();
  }
  else {
    // Scroll to selected message, it could become scrolled out due to filter change.
    scrollTo(selectionModel()->selectedRows().at(0),
             !m_processingAnyMouseButton &&
                 qApp->settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool()
               ? QAbstractItemView::ScrollHint::PositionAtCenter
               : QAbstractItemView::ScrollHint::EnsureVisible);
  }
}

void MessagesView::highlightMessages(MessagesModel::MessageHighlighter highlighter) {
  m_sourceModel->highlightMessages(highlighter);
}

void MessagesView::openSelectedMessagesWithExternalTool() {
  auto* sndr = qobject_cast<QAction*>(sender());

  if (sndr != nullptr) {
    auto tool = sndr->data().value<ExternalTool>();
    auto rws = selectionModel()->selectedRows();

    for (const QModelIndex& index : std::as_const(rws)) {
      const QString link =
        m_sourceModel->data(m_proxyModel->mapToSource(index).row(), MSG_MDL_URL_INDEX, Qt::ItemDataRole::EditRole)
          .toString()
          .replace(QRegularExpression(QSL("[\\t\\n]")), QString());

      if (!link.isEmpty()) {
        if (!tool.run(link)) {
          qApp->showGuiMessage(Notification::Event::GeneralEvent,
                               {tr("Cannot run external tool"),
                                tr("External tool '%1' could not be started.").arg(tool.executable()),
                                QSystemTrayIcon::MessageIcon::Critical});
        }
      }
    }
  }
}

void MessagesView::adjustColumns() {
  qDebugNN << LOGSEC_GUI << "Article list header geometries changed.";

  if (header()->count() > 0 && !m_columnsAdjusted) {
    m_columnsAdjusted = true;

    // Setup column resize strategies.
    for (int i = 0; i < header()->count(); i++) {
      header()->setSectionResizeMode(i, QHeaderView::ResizeMode::Interactive);
    }

    // Hide columns.
    hideColumn(MSG_MDL_ID_INDEX);
    hideColumn(MSG_MDL_DELETED_INDEX);
    hideColumn(MSG_MDL_URL_INDEX);
    hideColumn(MSG_MDL_CONTENTS_INDEX);
    hideColumn(MSG_MDL_PDELETED_INDEX);
    hideColumn(MSG_MDL_SCORE_INDEX);
    hideColumn(MSG_MDL_ACCOUNT_ID_INDEX);
    hideColumn(MSG_MDL_CUSTOM_ID_INDEX);
    hideColumn(MSG_MDL_CUSTOM_HASH_INDEX);
    hideColumn(MSG_MDL_FEED_ID_INDEX);
    hideColumn(MSG_MDL_FEED_TITLE_INDEX);
    hideColumn(MSG_MDL_HAS_ENCLOSURES);
    hideColumn(MSG_MDL_LABELS);
  }
}

void MessagesView::verticalScrollbarValueChanged(int value) {
  if (value == verticalScrollBar()->maximum()) {
    emit reachedEndOfList();
  }

  QTreeView::verticalScrollbarValueChanged(value);
}
