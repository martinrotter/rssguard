// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagesview.h"

#include "3rd-party/boolinq/boolinq.h"
#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "gui/reusable/labelsmenu.h"
#include "gui/reusable/styleditemdelegatewithoutfocus.h"
#include "gui/reusable/treeviewcolumnsmenu.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/serviceroot.h"

#include <QFileIconProvider>
#include <QKeyEvent>
#include <QMenu>
#include <QProcess>
#include <QScrollBar>
#include <QTimer>
#include <QTimer>

MessagesView::MessagesView(QWidget* parent)
  : BaseTreeView(parent), m_contextMenu(nullptr), m_columnsAdjusted(false), m_processingMouse(false) {
  m_sourceModel = qApp->feedReader()->messagesModel();
  m_proxyModel = qApp->feedReader()->messagesProxyModel();

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
}

void MessagesView::restoreHeaderState(const QByteArray& dta) {
  QByteArray arr = dta;
  QDataStream inn(&arr, QIODevice::OpenModeFlag::ReadOnly);

  inn.setVersion(QDataStream::Version::Qt_4_7);

  int saved_header_count; inn >> saved_header_count;

  if (std::abs(saved_header_count - header()->count()) > 10) {
    qWarningNN << LOGSEC_GUI << "Detected invalid state for list view.";
    return;
  }

  int saved_sort_order; inn >> saved_sort_order;
  int saved_sort_column; inn >> saved_sort_column;

  for (int i = 0; i < saved_header_count && i < header()->count(); i++) {
    int vi, ss;
    bool ish;

    inn >> vi;
    inn >> ss;
    inn >> ish;

    if (vi < header()->count()) {
      header()->swapSections(header()->visualIndex(i), vi);
    }

    header()->resizeSection(i, ss);
    header()->setSectionHidden(i, ish);
  }

  if (saved_sort_column < header()->count()) {
    header()->setSortIndicator(saved_sort_column, Qt::SortOrder(saved_sort_order));
  }
}

void MessagesView::sort(int column, Qt::SortOrder order,
                        bool repopulate_data, bool change_header,
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
  const QModelIndex mapped_current_index = m_proxyModel->mapToSource(current_index);
  const Message selected_message = m_sourceModel->messageAt(mapped_current_index.row());
  const int col = header()->sortIndicatorSection();
  const Qt::SortOrder ord = header()->sortIndicatorOrder();

  // Reload the model now.
  sort(col, ord, true, false, false, true);

  // Now, we must find the same previously focused message.
  if (selected_message.m_id > 0) {
    if (m_proxyModel->rowCount() == 0) {
      current_index = QModelIndex();
    }
    else {
      for (int i = 0; i < m_proxyModel->rowCount(); i++) {
        QModelIndex msg_idx = m_proxyModel->index(i, MSG_DB_TITLE_INDEX);
        Message msg = m_sourceModel->messageAt(m_proxyModel->mapToSource(msg_idx).row());

        if (msg.m_id == selected_message.m_id) {
          current_index = msg_idx;
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
    setCurrentIndex(current_index);
    reselectIndexes(QModelIndexList() << current_index);
  }
  else {
    // Messages were probably removed from the model, nothing can
    // be selected and no message can be displayed.
    emit currentMessageRemoved();
  }

  const QDateTime dt2 = QDateTime::currentDateTime();

  qDebugNN << LOGSEC_GUI
           << "Reloading of msg selections took "
           << dt1.msecsTo(dt2)
           << " miliseconds.";
}

void MessagesView::setupAppearance() {
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setUniformRowHeights(true);
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

  setItemDelegate(new StyledItemDelegateWithoutFocus(this));
  header()->setDefaultSectionSize(MESSAGES_VIEW_DEFAULT_COL);
  header()->setMinimumSectionSize(MESSAGES_VIEW_MINIMUM_COL);
  header()->setFirstSectionMovable(true);
  header()->setCascadingSectionResizes(false);
  header()->setStretchLastSection(false);
}

void MessagesView::focusInEvent(QFocusEvent* event) {
  QTreeView::focusInEvent(event);

  if (currentIndex().isValid()) {
    selectionModel()->select(currentIndex(), QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);
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
    auto rows = boolinq::from(mapped_indexes).select([](const QModelIndex& idx) {
      return idx.row();
    }).toStdList();

    selected_messages = m_sourceModel->messagesAt(FROM_STD_LIST(QList<int>, rows));
  }

  // External tools.
  QFileIconProvider icon_provider;
  QMenu* menu_ext_tools = new QMenu(tr("Open with external tool"), m_contextMenu);
  auto tools = ExternalTool::toolsFromSettings();

  menu_ext_tools->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

  for (const ExternalTool& tool : qAsConst(tools)) {
    QAction* act_tool = new QAction(QFileInfo(tool.executable()).fileName(), menu_ext_tools);

    act_tool->setIcon(icon_provider.icon(tool.executable()));
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
      emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()), m_sourceModel->loadedItem());
    }
    else {
      emit currentMessageRemoved();
    }
  });

  // Rest.
  m_contextMenu->addMenu(menu_ext_tools);
  m_contextMenu->addMenu(menu_labels);
  m_contextMenu->addActions(
    QList<QAction*>()
      << qApp->mainForm()->m_ui->m_actionSendMessageViaEmail
      << qApp->mainForm()->m_ui->m_actionOpenSelectedSourceArticlesExternally
      << qApp->mainForm()->m_ui->m_actionOpenSelectedMessagesInternally
      << qApp->mainForm()->m_ui->m_actionMarkSelectedMessagesAsRead
      << qApp->mainForm()->m_ui->m_actionMarkSelectedMessagesAsUnread
      << qApp->mainForm()->m_ui->m_actionSwitchImportanceOfSelectedMessages
      << qApp->mainForm()->m_ui->m_actionDeleteSelectedMessages);

  if (m_sourceModel->loadedItem() != nullptr) {
    if (m_sourceModel->loadedItem()->kind() == RootItem::Kind::Bin) {
      m_contextMenu->addAction(qApp->mainForm()->m_ui->m_actionRestoreSelectedMessages);
    }

    auto extra_context_menu = m_sourceModel->loadedItem()->getParentServiceRoot()->contextMenuMessagesList(selected_messages);

    if (!extra_context_menu.isEmpty()) {
      m_contextMenu->addSeparator();
      m_contextMenu->addActions(extra_context_menu);
    }
  }
}

void MessagesView::mousePressEvent(QMouseEvent* event) {
  m_processingMouse = true;
  QTreeView::mousePressEvent(event);
  m_processingMouse = false;

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

  qDebugNN << LOGSEC_GUI
           << "Current row changed - proxy '"
           << current_index << "', source '"
           << mapped_current_index << "'.";

  if (mapped_current_index.isValid() && selected_rows.count() > 0) {
    Message message = m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row());

    // Set this message as read only if current item
    // wasn't changed by "mark selected messages unread" action.
    m_sourceModel->setMessageRead(mapped_current_index.row(), RootItem::ReadStatus::Read);
    message.m_isRead = true;

    emit currentMessageChanged(message, m_sourceModel->loadedItem());
  }
  else {
    emit currentMessageRemoved();
  }

  if (!m_processingMouse &&
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

  // Messages are loaded, make sure that previously
  // active message is not shown in browser.
  emit currentMessageRemoved();
}

void MessagesView::switchShowUnreadOnly(bool set_new_value, bool show_unread_only) {
  if (set_new_value) {
    m_proxyModel->setShowUnreadOnly(show_unread_only);
  }

  reloadSelections();
}

void MessagesView::openSelectedSourceMessagesExternally() {
  auto rws = selectionModel()->selectedRows();

  for (const QModelIndex& index : qAsConst(rws)) {
    QString link = m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row())
                   .m_url
                   .replace(QRegularExpression(QSL("[\\t\\n]")), QString());

    qApp->web()->openUrlInExternalBrowser(link);
  }

  // Finally, mark opened messages as read.
  if (!selectionModel()->selectedRows().isEmpty()) {
    QTimer::singleShot(0, this, &MessagesView::markSelectedMessagesRead);
  }

  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally)).toBool()) {
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

void MessagesView::sendSelectedMessageViaEmail() {
  if (selectionModel()->selectedRows().size() == 1) {
    const Message message = m_sourceModel->messageAt(m_proxyModel->mapToSource(selectionModel()->selectedRows().at(0)).row());

    if (!qApp->web()->sendMessageViaEmail(message)) {
      MessageBox::show(this, QMessageBox::Critical, tr("Problem with starting external e-mail client"),
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
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  QModelIndexList selected_indexes = selectionModel()->selectedRows();
  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesRead(mapped_indexes, read);
  current_index = m_proxyModel->index(current_index.row(), current_index.column());

  if (current_index.isValid()) {
    emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()), m_sourceModel->loadedItem());
  }
  else {
    emit currentMessageRemoved();
  }
}

void MessagesView::deleteSelectedMessages() {
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  const QModelIndexList selected_indexes = selectionModel()->selectedRows();
  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesDeleted(mapped_indexes);
  current_index = moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);

  if (current_index.isValid()) {
    setCurrentIndex(current_index);
  }
  else {
    emit currentMessageRemoved();
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
    emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()), m_sourceModel->loadedItem());
  }
  else {
    emit currentMessageRemoved();
  }
}

void MessagesView::switchSelectedMessagesImportance() {
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  QModelIndexList selected_indexes = selectionModel()->selectedRows();
  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->switchBatchMessageImportance(mapped_indexes);
  current_index = m_proxyModel->index(current_index.row(), current_index.column());

  if (current_index.isValid()) {
    emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()), m_sourceModel->loadedItem());
  }
  else {
    // Messages were probably removed from the model, nothing can
    // be selected and no message can be displayed.
    emit currentMessageRemoved();
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
  const QModelIndex index_next = moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);

  if (index_next.isValid()) {
    setCurrentIndex(index_next);

    scrollTo(index_next,
             !m_processingMouse && qApp->settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool()
             ? QAbstractItemView::ScrollHint::PositionAtCenter
             : QAbstractItemView::ScrollHint::PositionAtTop);

    selectionModel()->select(index_next, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    setFocus();
  }
}

void MessagesView::selectPreviousItem() {
  const QModelIndex index_previous = moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);

  if (index_previous.isValid()) {
    setCurrentIndex(index_previous);

    scrollTo(index_previous,
             !m_processingMouse && qApp->settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool()
             ? QAbstractItemView::ScrollHint::PositionAtCenter
             : QAbstractItemView::ScrollHint::PositionAtTop);

    selectionModel()->select(index_previous, QItemSelectionModel::Select | QItemSelectionModel::Rows);
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
    // We found unread message, mark it.
    setCurrentIndex(next_unread);

    // Make sure that item is properly visible even if
    // message previewer was hidden and shows up.
    qApp->processEvents();

    scrollTo(next_unread,
             !m_processingMouse && qApp->settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool()
             ? QAbstractItemView::ScrollHint::PositionAtCenter
             : QAbstractItemView::ScrollHint::PositionAtTop);

    selectionModel()->select(next_unread,
                             QItemSelectionModel::SelectionFlag::Select |
                             QItemSelectionModel::SelectionFlag::Rows);
    setFocus();
  }
}

void MessagesView::searchMessages(const QString& pattern) {
  qDebugNN << LOGSEC_GUI
           << "Running search of messages with pattern"
           << QUOTE_W_SPACE_DOT(pattern);

#if QT_VERSION < 0x050C00 // Qt < 5.12.0
  m_proxyModel->setFilterRegExp(pattern.toLower());
#else
  m_proxyModel->setFilterRegularExpression(pattern.toLower());
#endif

  if (selectionModel()->selectedRows().isEmpty()) {
    emit currentMessageRemoved();
  }
  else {
    // Scroll to selected message, it could become scrolled out due to filter change.
    scrollTo(selectionModel()->selectedRows().at(0),
             !m_processingMouse && qApp->settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool()
             ? QAbstractItemView::ScrollHint::PositionAtCenter
             : QAbstractItemView::ScrollHint::EnsureVisible);
  }
}

void MessagesView::filterMessages(MessagesModel::MessageHighlighter filter) {
  m_sourceModel->highlightMessages(filter);
}

void MessagesView::openSelectedMessagesWithExternalTool() {
  auto* sndr = qobject_cast<QAction*>(sender());

  if (sndr != nullptr) {
    auto tool = sndr->data().value<ExternalTool>();
    auto rws = selectionModel()->selectedRows();

    for (const QModelIndex& index : qAsConst(rws)) {
      const QString link = m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row())
                           .m_url
                           .replace(QRegularExpression(QSL("[\\t\\n]")), QString());

      if (!link.isEmpty()) {
        if (!tool.run(link)) {
          qApp->showGuiMessage(Notification::Event::GeneralEvent,
                               tr("Cannot run external tool"),
                               tr("External tool '%1' could not be started.").arg(tool.executable()),
                               QSystemTrayIcon::MessageIcon::Critical);
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
    header()->setSectionResizeMode(MSG_DB_READ_INDEX, QHeaderView::ResizeMode::ResizeToContents);
    header()->setSectionResizeMode(MSG_DB_IMPORTANT_INDEX, QHeaderView::ResizeMode::ResizeToContents);
    header()->setSectionResizeMode(MSG_DB_SCORE_INDEX, QHeaderView::ResizeMode::ResizeToContents);
    header()->setSectionResizeMode(MSG_DB_HAS_ENCLOSURES, QHeaderView::ResizeMode::ResizeToContents);

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
    hideColumn(MSG_DB_HAS_ENCLOSURES);
  }
}

void MessagesView::onSortIndicatorChanged(int column, Qt::SortOrder order) {
  // Repopulate the shit.
  sort(column, order, true, false, false, false);

  emit currentMessageRemoved();
}
