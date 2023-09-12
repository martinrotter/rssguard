// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagesview.h"

#include "3rd-party/boolinq/boolinq.h"
#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "gui/reusable/labelsmenu.h"
#include "gui/reusable/styleditemdelegatewithoutfocus.h"
#include "gui/reusable/treeviewcolumnsmenu.h"
#include "gui/toolbars/messagestoolbar.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "network-web/webfactory.h"
#include "qnamespace.h"
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

  /*
   *
   *
    QByteArray arr;
  QDataStream outt(&arr, QIODevice::OpenModeFlag::WriteOnly);

  outt.setVersion(QDataStream::Version::Qt_4_7);
  outt << header()->count();
  outt << int(header()->sortIndicatorOrder());
  outt << header()->sortIndicatorSection();

  // Save column data.
  for (int i = 0; i < header()->count(); i++) {
    outt << header()->visualIndex(i);
    outt << header()->sectionSize(i);
    outt << header()->isSectionHidden(i);
  }

  return arr;
  */
}

void MessagesView::restoreHeaderState(const QByteArray& dta) {
  QJsonObject obj = QJsonDocument::fromJson(dta).object();
  int saved_header_count = obj[QSL("header_count")].toInt();

  if (saved_header_count < header()->count()) {
    qWarningNN << LOGSEC_GUI << "Detected invalid state for list view.";
    return;
  }

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
  }

  // Restore sort attributes.
  int saved_sort_count = obj[QSL("sort_count")].toInt();

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

  /*
  QByteArray arr = dta;
  QDataStream inn(&arr, QIODevice::OpenModeFlag::ReadOnly);

  inn.setVersion(QDataStream::Version::Qt_4_7);

  int saved_header_count;
  inn >> saved_header_count;

  if (std::abs(saved_header_count - header()->count()) > 10) {
    qWarningNN << LOGSEC_GUI << "Detected invalid state for list view.";
    return;
  }

  int saved_sort_order;
  inn >> saved_sort_order;
  int saved_sort_column;
  inn >> saved_sort_column;

  for (int i = 0; i < saved_header_count && i < header()->count(); i++) {
    int vi, ss;
    bool ish;

    inn >> vi;
    inn >> ss;
    inn >> ish;

    // auto ax = m_sourceModel->headerData(i, Qt::Orientation::Horizontal, Qt::ItemDataRole::DisplayRole).toString();

    if (vi < header()->count()) {
      header()->swapSections(header()->visualIndex(i), vi);
    }

    header()->resizeSection(i, ss);
    header()->setSectionHidden(i, ish);
  }

  if (saved_sort_column < header()->count()) {
    header()->setSortIndicator(saved_sort_column, Qt::SortOrder(saved_sort_order));
  }
  */
}

void MessagesView::copyUrlOfSelectedArticles() const {
  const QModelIndexList selected_indexes = selectionModel()->selectedRows();

  if (selected_indexes.isEmpty()) {
    return;
  }

  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);
  QStringList urls;

  for (const auto article_idx : mapped_indexes) {
    urls << m_sourceModel->data(m_sourceModel->index(article_idx.row(), MSG_DB_URL_INDEX), Qt::ItemDataRole::EditRole)
              .toString();
  }

  if (qApp->clipboard() != nullptr && !urls.isEmpty()) {
    qApp->clipboard()->setText(urls.join(TextFactory::newline()), QClipboard::Mode::Clipboard);
  }
}

void MessagesView::sort(int column,
                        Qt::SortOrder order,
                        bool repopulate_data,
                        bool change_header,
                        bool emit_changed_from_header,
                        bool ignore_multicolumn_sorting) {
  if (change_header && !emit_changed_from_header) {
    header()->blockSignals(true);
  }

  m_sourceModel->addSortState(column, order, ignore_multicolumn_sorting);

  if (repopulate_data) {
    m_sourceModel->repopulate();
  }

  if (change_header) {
    header()->setSortIndicator(column, order);
    header()->blockSignals(false);
  }
}

void MessagesView::createConnections() {
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

void MessagesView::reloadSelections() {
  const QDateTime dt1 = QDateTime::currentDateTime();
  QModelIndex current_index = selectionModel()->currentIndex();
  const bool is_current_selected =
    selectionModel()->selectedRows().contains(m_proxyModel->index(current_index.row(), 0, current_index.parent()));
  const QModelIndex mapped_current_index = m_proxyModel->mapToSource(current_index);
  const int selected_message_id =
    m_sourceModel->data(mapped_current_index.row(), MSG_DB_ID_INDEX, Qt::ItemDataRole::EditRole).toInt();
  const int col = header()->sortIndicatorSection();
  const Qt::SortOrder ord = header()->sortIndicatorOrder();
  bool do_not_mark_read_on_select = false;

  // Reload the model now.
  sort(col, ord, true, false, false, true);

  // Now, we must find the same previously focused message.
  if (selected_message_id > 0) {
    if (m_proxyModel->rowCount() == 0 || !is_current_selected) {
      current_index = QModelIndex();
    }
    else {
      for (int i = 0; i < m_proxyModel->rowCount(); i++) {
        QModelIndex msg_idx = m_proxyModel->index(i, MSG_DB_TITLE_INDEX);
        QModelIndex msg_source_idx = m_proxyModel->mapToSource(msg_idx);
        int msg_id = m_sourceModel->data(msg_source_idx.row(), MSG_DB_ID_INDEX, Qt::ItemDataRole::EditRole).toInt();

        if (msg_id == selected_message_id) {
          current_index = msg_idx;

          if (!m_sourceModel->data(msg_source_idx.row(), MSG_DB_READ_INDEX, Qt::ItemDataRole::EditRole)
                 .toBool() /* && selected_message.m_isRead */) {
            do_not_mark_read_on_select = true;
          }

          break;
        }

        if (i == m_proxyModel->rowCount() - 1) {
          current_index = QModelIndex();
        }
      }
    }
  }

  if (current_index.isValid()) {
    scrollTo(current_index);

    m_processingRightMouseButton = do_not_mark_read_on_select;

    setCurrentIndex(current_index);
    reselectIndexes({current_index});

    m_processingRightMouseButton = false;
  }
  else {
    // Messages were probably removed from the model, nothing can
    // be selected and no message can be displayed.
    emit currentMessageRemoved(m_sourceModel->loadedItem());
  }

  const QDateTime dt2 = QDateTime::currentDateTime();

  qDebugNN << LOGSEC_GUI << "Reloading of msg selections took " << dt1.msecsTo(dt2) << " miliseconds.";
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
  setAllColumnsShowFocus(false);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setItemDelegate(new StyledItemDelegateWithoutFocus(qApp->settings()
                                                       ->value(GROUP(GUI), SETTING(GUI::HeightRowMessages))
                                                       .toInt(),
                                                     qApp->settings()
                                                       ->value(GROUP(Messages), SETTING(Messages::ArticleListPadding))
                                                       .toInt(),
                                                     this));

  header()->setDefaultSectionSize(MESSAGES_VIEW_DEFAULT_COL);
  header()->setMinimumSectionSize(MESSAGES_VIEW_MINIMUM_COL);
  header()->setFirstSectionMovable(true);
  header()->setCascadingSectionResizes(false);
  header()->setStretchLastSection(false);

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
    auto rows = boolinq::from(mapped_indexes)
                  .select([](const QModelIndex& idx) {
                    return idx.row();
                  })
                  .toStdList();

    selected_messages = m_sourceModel->messagesAt(FROM_STD_LIST(QList<int>, rows));
  }

  // External tools.
  QFileIconProvider icon_provider;
  QMenu* menu_ext_tools = new QMenu(tr("Open with external tool"), m_contextMenu);
  auto tools = ExternalTool::toolsFromSettings();

  menu_ext_tools->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

  for (const ExternalTool& tool : qAsConst(tools)) {
    QAction* act_tool = new QAction(QFileInfo(tool.executable()).fileName(), menu_ext_tools);

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
  auto labels = m_sourceModel->loadedItem() != nullptr
                  ? m_sourceModel->loadedItem()->getParentServiceRoot()->labelsNode()->labels()
                  : QList<Label*>();
  LabelsMenu* menu_labels = new LabelsMenu(selected_messages, labels, m_contextMenu);

  connect(menu_labels, &LabelsMenu::labelsChanged, this, [this]() {
    QModelIndex current_index = selectionModel()->currentIndex();

    if (current_index.isValid()) {
      emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()),
                                 m_sourceModel->loadedItem());
    }
    else {
      emit currentMessageRemoved(m_sourceModel->loadedItem());
    }
  });

  // Rest.
  m_contextMenu->addMenu(menu_ext_tools);
  m_contextMenu->addMenu(menu_labels);
  m_contextMenu->addActions(QList<QAction*>() << qApp->mainForm()->m_ui->m_actionSendMessageViaEmail
                                              << qApp->mainForm()->m_ui->m_actionOpenSelectedSourceArticlesExternally
                                              << qApp->mainForm()->m_ui->m_actionOpenSelectedMessagesInternally
                                              << qApp->mainForm()->m_ui->m_actionOpenSelectedMessagesInternallyNoTab
                                              << qApp->mainForm()->m_ui->m_actionCopyUrlSelectedArticles
                                              << qApp->mainForm()->m_ui->m_actionMarkSelectedMessagesAsRead
                                              << qApp->mainForm()->m_ui->m_actionMarkSelectedMessagesAsUnread
                                              << qApp->mainForm()->m_ui->m_actionSwitchImportanceOfSelectedMessages
                                              << qApp->mainForm()->m_ui->m_actionDeleteSelectedMessages);

  if (m_sourceModel->loadedItem() != nullptr) {
    if (m_sourceModel->loadedItem()->kind() == RootItem::Kind::Bin) {
      m_contextMenu->addAction(qApp->mainForm()->m_ui->m_actionRestoreSelectedMessages);
    }

    auto extra_context_menu =
      m_sourceModel->loadedItem()->getParentServiceRoot()->contextMenuMessagesList(selected_messages);

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

        if (mapped_index.column() == MSG_DB_IMPORTANT_INDEX) {
          if (m_sourceModel->switchMessageImportance(mapped_index.row())) {
            emit currentMessageChanged(m_sourceModel->messageAt(mapped_index.row()), m_sourceModel->loadedItem());
          }
        }
      }

      break;
    }

    case Qt::MouseButton::MiddleButton: {
      // Make sure that message importance is switched when user
      // clicks the "important" column.
      const QModelIndex clicked_index = indexAt(event->pos());

      if (clicked_index.isValid()) {
        const QModelIndex mapped_index = m_proxyModel->mapToSource(clicked_index);
        const QString url = m_sourceModel->messageAt(mapped_index.row()).m_url;

        if (!url.isEmpty()) {
          qApp->mainForm()->tabWidget()->addLinkedBrowser(url);
        }
      }

      break;
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

  qDebugNN << LOGSEC_GUI << "Current row changed - proxy '" << current_index << "', source '" << mapped_current_index
           << "'.";

  if (mapped_current_index.isValid() && selected_rows.size() == 1) {
    Message message = m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row());

    // Set this message as read only if current item
    // wasn't changed by "mark selected messages unread" action.
    if (!m_processingRightMouseButton) {
      m_sourceModel->setMessageRead(mapped_current_index.row(), RootItem::ReadStatus::Read);
      message.m_isRead = true;
    }

    emit currentMessageChanged(message, m_sourceModel->loadedItem());
  }
  else {
    emit currentMessageRemoved(m_sourceModel->loadedItem());
  }

  if (selected_rows.isEmpty()) {
    setCurrentIndex({});
  }

  if (!m_processingAnyMouseButton &&
      qApp->settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool()) {
    scrollTo(currentIndex(), QAbstractItemView::ScrollHint::PositionAtCenter);
  }

  QTreeView::selectionChanged(selected, deselected);
}

void MessagesView::loadItem(RootItem* item) {
  const int col = header()->sortIndicatorSection();
  const Qt::SortOrder ord = header()->sortIndicatorOrder();

  scrollToTop();
  sort(col, ord, false, true, false, true);
  m_sourceModel->loadMessages(item);

  /*
  if (item->kind() == RootItem::Kind::Feed) {
    if (item->toFeed()->isRtl()) {
      setLayoutDirection(Qt::LayoutDirection::RightToLeft);
    }
    else {
      setLayoutDirection(Qt::LayoutDirection::LeftToRight);
    }
  }
  else {
    setLayoutDirection(Qt::LayoutDirection::LeftToRight);
  }
  */

  // Messages are loaded, make sure that previously
  // active message is not shown in browser.
  emit currentMessageRemoved(m_sourceModel->loadedItem());
}

void MessagesView::changeFilter(MessagesProxyModel::MessageListFilter filter) {
  m_proxyModel->setMessageListFilter(filter);
  reloadSelections();
}

void MessagesView::openSelectedSourceMessagesExternally() {
  auto rws = selectionModel()->selectedRows();

  for (const QModelIndex& index : qAsConst(rws)) {
    QString link = m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row())
                     .m_url.replace(QRegularExpression(QSL("[\\t\\n]")), QString());

    qApp->web()->openUrlInExternalBrowser(link);
  }

  // Finally, mark opened messages as read.
  if (!selectionModel()->selectedRows().isEmpty()) {
    QTimer::singleShot(0, this, &MessagesView::markSelectedMessagesRead);
  }

  if (qApp->settings()
        ->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally))
        .toBool()) {
    QTimer::singleShot(1000, this, []() {
      qApp->mainForm()->display();
    });
  }
}

void MessagesView::openSelectedMessagesInternally() {
  QList<Message> messages;
  auto rws = selectionModel()->selectedRows();

  for (const QModelIndex& index : qAsConst(rws)) {
    messages << m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row());
  }

  if (!messages.isEmpty()) {
    emit openMessagesInNewspaperView(m_sourceModel->loadedItem(), messages);
  }
}

void MessagesView::openSelectedMessageUrl() {
  auto rws = selectionModel()->selectedRows();

  if (!rws.isEmpty()) {
    auto msg = m_sourceModel->messageAt(m_proxyModel->mapToSource(rws.at(0)).row());

    if (!msg.m_url.isEmpty()) {
      emit openLinkMiniBrowser(msg.m_url);
    }
  }
}

void MessagesView::sendSelectedMessageViaEmail() {
  if (selectionModel()->selectedRows().size() == 1) {
    const Message message =
      m_sourceModel->messageAt(m_proxyModel->mapToSource(selectionModel()->selectedRows().at(0)).row());

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
    emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()),
                               m_sourceModel->loadedItem());
  }
  else {
    emit currentMessageRemoved(m_sourceModel->loadedItem());
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
    emit currentMessageRemoved(m_sourceModel->loadedItem());
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
    emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()),
                               m_sourceModel->loadedItem());
  }
  else {
    emit currentMessageRemoved(m_sourceModel->loadedItem());
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
    emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()),
                               m_sourceModel->loadedItem());
  }
  else {
    // Messages were probably removed from the model, nothing can
    // be selected and no message can be displayed.
    emit currentMessageRemoved(m_sourceModel->loadedItem());
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

  MessagesToolBar::SearchFields where_search = MessagesToolBar::SearchFields(custom_criteria);

  m_proxyModel->setFilterKeyColumn(where_search == MessagesToolBar::SearchFields::SearchTitleOnly ? MSG_DB_TITLE_INDEX
                                                                                                  : -1);

  if (selectionModel()->selectedRows().isEmpty()) {
    emit currentMessageRemoved(m_sourceModel->loadedItem());
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

    for (const QModelIndex& index : qAsConst(rws)) {
      const QString link =
        m_sourceModel->data(m_proxyModel->mapToSource(index).row(), MSG_DB_URL_INDEX, Qt::ItemDataRole::EditRole)
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
  if (header()->count() > 0 && !m_columnsAdjusted) {
    m_columnsAdjusted = true;

    // Setup column resize strategies.
    for (int i = 0; i < header()->count(); i++) {
      header()->setSectionResizeMode(i, QHeaderView::ResizeMode::Interactive);
    }

    header()->setSectionResizeMode(MSG_DB_TITLE_INDEX, QHeaderView::ResizeMode::Stretch);

    // Hide columns.
    hideColumn(MSG_DB_ID_INDEX);
    hideColumn(MSG_DB_DELETED_INDEX);
    hideColumn(MSG_DB_URL_INDEX);
    hideColumn(MSG_DB_CONTENTS_INDEX);
    hideColumn(MSG_DB_PDELETED_INDEX);
    hideColumn(MSG_DB_ENCLOSURES_INDEX);
    hideColumn(MSG_DB_SCORE_INDEX);
    hideColumn(MSG_DB_ACCOUNT_ID_INDEX);
    hideColumn(MSG_DB_CUSTOM_ID_INDEX);
    hideColumn(MSG_DB_CUSTOM_HASH_INDEX);
    hideColumn(MSG_DB_FEED_CUSTOM_ID_INDEX);
    hideColumn(MSG_DB_FEED_TITLE_INDEX);
    hideColumn(MSG_DB_FEED_IS_RTL_INDEX);
    hideColumn(MSG_DB_HAS_ENCLOSURES);
    hideColumn(MSG_DB_LABELS);
    hideColumn(MSG_DB_LABELS_IDS);
  }
}

void MessagesView::onSortIndicatorChanged(int column, Qt::SortOrder order) {
  // Repopulate the shit.
  sort(column, order, true, false, false, false);

  emit currentMessageRemoved(m_sourceModel->loadedItem());
}
