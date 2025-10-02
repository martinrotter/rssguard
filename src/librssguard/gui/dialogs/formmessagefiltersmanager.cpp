// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formmessagefiltersmanager.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "exceptions/filteringexception.h"
#include "filtering/filteringsystem.h"
#include "filtering/filterobjects.h"
#include "filtering/messagefilter.h"
#include "filtering/messagesforfiltersmodel.h"
#include "gui/guiutilities.h"
#include "gui/messagebox.h"
#include "gui/reusable/jssyntaxhighlighter.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/webfactory.h"
#include "services/abstract/accountcheckmodel.h"
#include "services/abstract/feed.h"

#include <QDateTime>
#include <QJSEngine>
#include <QProcess>
#include <QSortFilterProxyModel>

FormMessageFiltersManager::FormMessageFiltersManager(FeedReader* reader,
                                                     const QList<ServiceRoot*>& accounts,
                                                     QWidget* parent)
  : QDialog(parent), m_feedsModel(new AccountCheckSortedModel(this)), m_rootItem(new RootItem()), m_accounts(accounts),
    m_reader(reader), m_loadingFilter(false), m_msgProxyModel(new QSortFilterProxyModel(this)),
    m_msgModel(new MessagesForFiltersModel(this)) {
  m_ui.setupUi(this);

  m_defaultTextColor = m_ui.m_txtErrors->textColor();
  m_highlighter = new JsSyntaxHighlighter(m_ui.m_txtScript->document());

  std::sort(m_accounts.begin(), m_accounts.end(), [](const ServiceRoot* lhs, const ServiceRoot* rhs) {
    return lhs->title().compare(rhs->title(), Qt::CaseSensitivity::CaseInsensitive) < 0;
  });

  m_msgProxyModel->setSortCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
  m_msgProxyModel->setSourceModel(m_msgModel);
  m_ui.m_treeExistingMessages->setModel(m_msgProxyModel);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("view-list-details")));

  m_ui.m_treeFeeds->setIndentation(FEEDS_VIEW_INDENTATION);
  m_ui.m_treeFeeds->setModel(m_feedsModel);

  m_ui.m_btnCheckAll->setIcon(qApp->icons()->fromTheme(QSL("dialog-yes"), QSL("edit-select-all")));
  m_ui.m_btnUncheckAll->setIcon(qApp->icons()->fromTheme(QSL("dialog-no"), QSL("edit-select-none")));
  m_ui.m_btnAddNew->setIcon(qApp->icons()->fromTheme(QSL("list-add")));
  m_ui.m_btnRemoveSelected->setIcon(qApp->icons()->fromTheme(QSL("list-remove")));
  m_ui.m_btnBeautify->setIcon(qApp->icons()->fromTheme(QSL("format-justify-fill")));
  m_ui.m_btnTest->setIcon(qApp->icons()->fromTheme(QSL("media-playback-start")));
  m_ui.m_btnRunOnMessages->setIcon(qApp->icons()->fromTheme(QSL("media-playback-start")));
  m_ui.m_btnDetailedHelp->setIcon(qApp->icons()->fromTheme(QSL("help-contents")));

  m_ui.m_btnUp->setIcon(qApp->icons()->fromTheme(QSL("arrow-up"), QSL("go-up")));
  m_ui.m_btnDown->setIcon(qApp->icons()->fromTheme(QSL("arrow-down"), QSL("go-down")));
  m_ui.m_btnEnable->setIcon(qApp->icons()->fromTheme(QSL("media-playback-start"), QSL("kmplayer")));

  m_ui.m_txtScript->setFont(QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont));
  m_ui.m_tbMessageContents->setFont(QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont));
  m_ui.m_txtErrors->setFont(QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont));
  m_ui.m_treeExistingMessages->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_RESULT,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_ISREAD,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_ISIMPORTANT,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_ISDELETED,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_CREATED,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_SCORE,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_TITLE, QHeaderView::ResizeMode::Stretch);
  m_ui.m_treeExistingMessages->header()->setSectionsMovable(false);
  m_ui.m_treeExistingMessages->header()->setStretchLastSection(false);

  connect(m_ui.m_btnDown, &QToolButton::clicked, this, &FormMessageFiltersManager::moveFilterDown);
  connect(m_ui.m_btnUp, &QToolButton::clicked, this, &FormMessageFiltersManager::moveFilterUp);
  connect(m_ui.m_btnEnable, &QToolButton::clicked, this, &FormMessageFiltersManager::saveSelectedFilter);
  connect(m_ui.m_btnDetailedHelp, &QPushButton::clicked, this, &FormMessageFiltersManager::openDocs);
  connect(m_ui.m_listFilters, &QListWidget::currentRowChanged, this, &FormMessageFiltersManager::loadFilter);
  connect(m_ui.m_btnAddNew, &QToolButton::clicked, this, [this]() {
    addNewFilter();
  });
  connect(m_ui.m_btnRemoveSelected, &QToolButton::clicked, this, &FormMessageFiltersManager::removeSelectedFilter);
  connect(m_ui.m_txtTitle, &QLineEdit::textChanged, this, &FormMessageFiltersManager::saveSelectedFilter);
  connect(m_ui.m_txtScript, &QPlainTextEdit::textChanged, this, &FormMessageFiltersManager::saveSelectedFilter);
  connect(m_ui.m_btnTest, &QPushButton::clicked, this, &FormMessageFiltersManager::testFilter);
  connect(m_ui.m_btnBeautify, &QPushButton::clicked, this, &FormMessageFiltersManager::beautifyScript);
  connect(m_ui.m_cmbAccounts,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &FormMessageFiltersManager::onAccountChanged);
  connect(m_ui.m_btnCheckAll, &QToolButton::clicked, m_feedsModel->sourceModel(), &AccountCheckModel::checkAllItems);
  connect(m_ui.m_btnUncheckAll,
          &QToolButton::clicked,
          m_feedsModel->sourceModel(),
          &AccountCheckModel::uncheckAllItems);
  connect(m_feedsModel->sourceModel(),
          &AccountCheckModel::checkStateChanged,
          this,
          &FormMessageFiltersManager::onFeedChecked);
  connect(m_ui.m_treeFeeds->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &FormMessageFiltersManager::displayMessagesOfFeed);
  connect(m_ui.m_btnRunOnMessages, &QPushButton::clicked, this, &FormMessageFiltersManager::processCheckedFeeds);
  connect(m_ui.m_treeExistingMessages,
          &QTreeView::customContextMenuRequested,
          this,
          &FormMessageFiltersManager::showMessageContextMenu);
  connect(m_ui.m_treeExistingMessages->selectionModel(),
          &QItemSelectionModel::currentRowChanged,
          this,
          &FormMessageFiltersManager::displaySelectedMessageDetails);

  connect(m_ui.m_searchWidget, &SearchTextWidget::searchCancelled, this, [this]() {
    m_ui.m_txtScript->find(QString());
  });
  connect(m_ui.m_searchWidget, &SearchTextWidget::searchForText, this, [this](const QString& text, bool backwards) {
    m_ui.m_txtScript->find(text, backwards ? QTextDocument::FindFlag::FindBackward : QTextDocument::FindFlags());
    m_ui.m_searchWidget->setFocus();
  });

  m_ui.m_txtScript->installEventFilter(this);
  m_ui.m_searchWidget->hide();

  loadFilters();
  loadFilter();
  loadAccounts();
}

FormMessageFiltersManager::~FormMessageFiltersManager() {
  delete m_rootItem;
}

MessageFilter* FormMessageFiltersManager::selectedFilter() const {
  if (m_ui.m_listFilters->currentItem() == nullptr) {
    return nullptr;
  }
  else {
    return m_ui.m_listFilters->currentItem()->data(Qt::ItemDataRole::UserRole).value<MessageFilter*>();
  }
}

bool FormMessageFiltersManager::eventFilter(QObject* watched, QEvent* event) {
  Q_UNUSED(watched)

  if (event->type() == QEvent::KeyPress) {
    QKeyEvent* key_event = static_cast<QKeyEvent*>(event);

    // Find text.
    if (key_event->matches(QKeySequence::StandardKey::Find)) {
      m_ui.m_searchWidget->clear();
      m_ui.m_searchWidget->show();
      m_ui.m_searchWidget->setFocus();
      return true;
    }

    // Hide visible search box.
    if (key_event->key() == Qt::Key::Key_Escape && m_ui.m_searchWidget->isVisible()) {
      m_ui.m_searchWidget->hide();
      return true;
    }
  }

  return false;
}

void FormMessageFiltersManager::moveFilterDown() {
  auto* filter = selectedFilter();
  auto db = qApp->database()->driver()->connection(metaObject()->className());

  auto row = m_ui.m_listFilters->currentRow();

  DatabaseQueries::moveMessageFilter(m_reader->messageFilters(), filter, false, false, filter->sortOrder() + 1, db);
  m_ui.m_listFilters->insertItem(row + 1, m_ui.m_listFilters->takeItem(row));
  m_ui.m_listFilters->setCurrentRow(row + 1);
}

void FormMessageFiltersManager::moveFilterUp() {
  auto* filter = selectedFilter();
  auto db = qApp->database()->driver()->connection(metaObject()->className());

  auto row = m_ui.m_listFilters->currentRow();

  DatabaseQueries::moveMessageFilter(m_reader->messageFilters(), filter, false, false, filter->sortOrder() - 1, db);
  m_ui.m_listFilters->insertItem(row - 1, m_ui.m_listFilters->takeItem(row));
  m_ui.m_listFilters->setCurrentRow(row - 1);
}

void FormMessageFiltersManager::openDocs() {
  qApp->web()->openUrlInExternalBrowser(QSL("https://rssguard.readthedocs.io/en/stable/features/filters.html"));
}

void FormMessageFiltersManager::displaySelectedMessageDetails(const QModelIndex& current, const QModelIndex& previous) {
  Q_UNUSED(previous)

  QModelIndex idx = m_msgProxyModel->mapToSource(current);

  if (!idx.isValid()) {
    // NOTE: No need to clear, widget is hidden.
    // Nothing selected, just clear.
    /*
    m_ui.m_tbMessageUrl->clear();
    m_ui.m_tbMessageContents->clear();
    m_ui.m_tbMessageDbId->clear();
    m_ui.m_tbMessageCustomId->clear();
    m_ui.m_tbMessageAuthor->clear();
    */
  }
  else {
    Message* msg = m_msgModel->messageForRow(idx.row());

    m_ui.m_tbMessageUrl->setText(msg->m_url);
    m_ui.m_tbMessageContents->setPlainText(msg->m_contents);
    m_ui.m_tbMessageDbId->setText(QString::number(msg->m_id));
    m_ui.m_tbMessageCustomId->setText(msg->m_customId);
    m_ui.m_tbMessageAuthor->setText(msg->m_author);
  }

  m_ui.m_twDetails->setVisible(idx.isValid());
}

ServiceRoot* FormMessageFiltersManager::selectedAccount() const {
  auto dat = m_ui.m_cmbAccounts->currentData(Qt::ItemDataRole::UserRole);

  return dat.isNull() ? nullptr : dat.value<ServiceRoot*>();
}

void FormMessageFiltersManager::filterMessagesLikeThis(const Message& msg) {
  QString filter_script =
    QSL("function filterMessage() {\n"
        "  // Adjust the condition to suit your needs.\n"
        "  var is_message_same =\n"
        "    msg.isRead == %1 &&\n"
        "    msg.isImportant == %2 &&\n"
        "    msg.title == '%3' &&\n"
        "    msg.url == '%4';\n"
        "\n"
        "  if (is_message_same) {\n"
        "    return Msg.Accept;\n"
        "  }\n"
        "  else {\n"
        "    return Msg.Ignore;\n"
        "  }\n"
        "}")
      .arg(QString::number(int(msg.m_isRead)), QString::number(int(msg.m_isImportant)), msg.m_title, msg.m_url);

  addNewFilter(filter_script);
}

void FormMessageFiltersManager::showMessageContextMenu(QPoint pos) {
  Message* msg = m_msgModel->messageForRow(m_ui.m_treeExistingMessages->indexAt(pos).row());

  if (msg != nullptr) {
    QMenu menu(tr("Context menu"), m_ui.m_treeExistingMessages);

    menu.addAction(tr("Filter articles like this"), this, [=]() {
      filterMessagesLikeThis(*msg);
    });
    menu.exec(m_ui.m_treeExistingMessages->mapToGlobal(pos));
  }
}

void FormMessageFiltersManager::removeSelectedFilter() {
  auto* fltr = selectedFilter();

  if (fltr == nullptr) {
    return;
  }

  if (MsgBox::show(this,
                   QMessageBox::Icon::Question,
                   tr("Are you sure?"),
                   tr("Do you really want to remove selected filter?"),
                   {},
                   fltr->name(),
                   QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                   QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes) {
    m_reader->removeMessageFilter(fltr);
    delete m_ui.m_listFilters->currentItem();
  }
}

void FormMessageFiltersManager::loadFilters() {
  auto flt = m_reader->messageFilters();

  auto flt_ordered = boolinq::from(flt)
                       .orderBy([](MessageFilter* f) {
                         return f->sortOrder();
                       })
                       .toStdList();

  for (auto* fltr : std::as_const(flt_ordered)) {
    auto* it = new QListWidgetItem(fltr->name(), m_ui.m_listFilters);

    it->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue<MessageFilter*>(fltr));
    updateItemFromFilter(it, fltr);
  }
}

void FormMessageFiltersManager::addNewFilter(const QString& filter_script) {
  try {
    auto* fltr =
      m_reader->addMessageFilter(tr("New article filter"),
                                 filter_script.isEmpty() ? QSL("function filterMessage() { return Msg.Accept; }")
                                                         : filter_script);
    auto* it = new QListWidgetItem(fltr->name(), m_ui.m_listFilters);

    it->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue<MessageFilter*>(fltr));

    m_ui.m_listFilters->setCurrentRow(m_ui.m_listFilters->count() - 1);
  }
  catch (const ApplicationException& ex) {
    MsgBox::show(this,
                 QMessageBox::Icon::Critical,
                 tr("Error"),
                 tr("Cannot save new filter, error: '%1'.").arg(ex.message()));
  }
}

void FormMessageFiltersManager::saveSelectedFilter() {
  if (m_loadingFilter) {
    return;
  }

  auto* fltr = selectedFilter();

  if (fltr == nullptr || m_ui.m_txtTitle->text().isEmpty() || m_ui.m_txtScript->toPlainText().isEmpty()) {
    return;
  }

  fltr->setEnabled(m_ui.m_btnEnable->isChecked());
  fltr->setName(m_ui.m_txtTitle->text());
  fltr->setScript(m_ui.m_txtScript->toPlainText());

  updateItemFromFilter(m_ui.m_listFilters->currentItem(), fltr);

  m_reader->updateMessageFilter(fltr);
}

void FormMessageFiltersManager::updateItemFromFilter(QListWidgetItem* item, MessageFilter* filter) {
  item->setText(filter->name());
  item->setForeground(filter->enabled() ? QBrush() : QBrush(Qt::GlobalColor::red));
}

void FormMessageFiltersManager::loadFilter() {
  auto* filter = selectedFilter();
  auto* acc = selectedAccount();

  updateFilterOptions(filter);
  loadAccount(acc);
  showFilter(filter);
  loadFilterFeedAssignments(filter, acc);
}

void FormMessageFiltersManager::testFilter() {
  m_ui.m_txtErrors->clear();
  m_ui.m_txtErrors->setTextColor(m_defaultTextColor);

  // Perform per-message filtering.
  auto* selected_fd_cat = selectedCategoryFeed();
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());
  FilteringSystem filtering(FilteringSystem::FiteringUseCase::ExistingArticles,
                            database,
                            selected_fd_cat->kind() == RootItem::Kind::Feed ? selected_fd_cat->toFeed() : nullptr,
                            selectedAccount());

  filtering.filterRun().setTotalCountOfFilters(1);
  filtering.filterRun().setIndexOfCurrentFilter(0);

  connect(&filtering.filterApp(), &FilterApp::logged, this, [this](const QString& message) {
    m_ui.m_txtErrors->append(message);
  });

  auto* fltr = selectedFilter();

  // Test real messages.
  try {
    m_msgModel->testFilter(fltr, &filtering);
  }
  catch (const FilteringException& ex) {
    m_ui.m_txtErrors->setTextColor(Qt::GlobalColor::red);
    m_ui.m_txtErrors->append(ex.message());

    // See output.
    m_ui.m_twMessages->setCurrentIndex(1);
  }
}

void FormMessageFiltersManager::processCheckedFeeds() {
  QList<RootItem*> checked = m_feedsModel->sourceModel()->checkedItems();

  try {
    m_msgModel->processFeeds(selectedFilter(), selectedAccount(), checked);
  }
  catch (const ApplicationException& ex) {
    m_ui.m_txtErrors->setTextColor(Qt::GlobalColor::red);
    m_ui.m_txtErrors->append(ex.message());

    // See output.
    m_ui.m_twMessages->setCurrentIndex(1);
  }

  displayMessagesOfFeed();
}

void FormMessageFiltersManager::displayMessagesOfFeed() {
  auto* item = selectedCategoryFeed();

  if (item != nullptr) {
    m_msgModel->setMessages(item->undeletedMessages());
  }
  else {
    m_msgModel->setMessages({});
  }

  displaySelectedMessageDetails(m_ui.m_treeExistingMessages->currentIndex(), {});
}

void FormMessageFiltersManager::loadAccount(ServiceRoot* account) {
  m_feedsModel->setRootItem(account, false, true);

  if (account != nullptr) {
    m_msgModel->setMessages(account->undeletedMessages());
  }
  else {
    m_msgModel->setMessages({});
  }

  displaySelectedMessageDetails(m_ui.m_treeExistingMessages->currentIndex(), {});
}

void FormMessageFiltersManager::loadFilterFeedAssignments(MessageFilter* filter, ServiceRoot* account) {
  if (account == nullptr || filter == nullptr) {
    return;
  }

  m_loadingFilter = true;
  auto stf = account->getSubTreeFeeds();

  for (auto* feed : std::as_const(stf)) {
    if (feed->messageFilters().contains(filter)) {
      m_feedsModel->sourceModel()->setItemChecked(feed, Qt::CheckState::Checked);
    }
  }

  m_loadingFilter = false;
}

void FormMessageFiltersManager::onAccountChanged() {
  // Load feeds/categories of the account and check marks.
  auto* filter = selectedFilter();
  auto* acc = selectedAccount();

  loadAccount(acc);
  loadFilterFeedAssignments(filter, acc);
}

void FormMessageFiltersManager::onFeedChecked(RootItem* item, Qt::CheckState state) {
  if (m_loadingFilter) {
    return;
  }

  auto* feed = qobject_cast<Feed*>(item);

  if (feed == nullptr) {
    return;
  }

  // Update feed/filter assignemnts.
  switch (state) {
    case Qt::CheckState::Checked:
      m_reader->assignMessageFilterToFeed(feed, selectedFilter());
      break;

    case Qt::CheckState::Unchecked:
      m_reader->removeMessageFilterToFeedAssignment(feed, selectedFilter());
      break;

    case Qt::CheckState::PartiallyChecked:
      break;
  }
}

void FormMessageFiltersManager::showFilter(MessageFilter* filter) {
  m_loadingFilter = true;

  if (filter == nullptr) {
    m_ui.m_txtTitle->clear();
    m_ui.m_txtScript->clear();
    m_ui.m_gbDetails->setEnabled(false);

    m_ui.m_treeFeeds->setEnabled(false);
    m_ui.m_btnCheckAll->setEnabled(false);
    m_ui.m_btnUncheckAll->setEnabled(false);
    m_ui.m_cmbAccounts->setEnabled(false);
  }
  else {
    m_ui.m_txtTitle->setText(filter->name());
    m_ui.m_txtScript->setPlainText(filter->script());
    m_ui.m_gbDetails->setEnabled(true);

    m_ui.m_treeFeeds->setEnabled(true);
    m_ui.m_btnCheckAll->setEnabled(true);
    m_ui.m_btnUncheckAll->setEnabled(true);
    m_ui.m_cmbAccounts->setEnabled(true);
  }

  // See message.
  m_loadingFilter = false;
}

void FormMessageFiltersManager::loadAccounts() {
  for (auto* acc : std::as_const(m_accounts)) {
    m_ui.m_cmbAccounts->addItem(acc->icon(), acc->title(), QVariant::fromValue(acc));
  }
}

void FormMessageFiltersManager::beautifyScript() {
  QProcess proc_clang_format(this);

  proc_clang_format.setInputChannelMode(QProcess::InputChannelMode::ManagedInputChannel);
  proc_clang_format.setArguments({"--assume-filename=script.js", "--style=Chromium"});

#if defined(Q_OS_WIN)
  proc_clang_format.setProgram(qApp->applicationDirPath() + QDir::separator() + QSL("clang-format.exe"));
#else
  proc_clang_format.setProgram(QSL("clang-format"));
#endif

  if (!proc_clang_format.open() || proc_clang_format.error() == QProcess::ProcessError::FailedToStart) {
    MsgBox::show(this,
                 QMessageBox::Icon::Critical,
                 tr("Cannot find 'clang-format'"),
                 tr("Script was not beautified, because 'clang-format' tool was not found."));
    return;
  }

  proc_clang_format.write(m_ui.m_txtScript->toPlainText().toUtf8());
  proc_clang_format.closeWriteChannel();

  if (proc_clang_format.waitForFinished(3000)) {
    if (proc_clang_format.exitCode() == 0) {
      auto script = proc_clang_format.readAllStandardOutput();

      m_ui.m_txtScript->setPlainText(script);
    }
    else {
      auto err = proc_clang_format.readAllStandardError();

      MsgBox::show(this,
                   QMessageBox::Icon::Critical,
                   tr("Error"),
                   tr("Script was not beautified, because 'clang-format' tool thrown error."),
                   QString(),
                   err);
    }
  }
  else {
    proc_clang_format.kill();
    MsgBox::show(this,
                 QMessageBox::Icon::Critical,
                 tr("Beautifier was running for too long time"),
                 tr("Script was not beautified, is 'clang-format' installed?"));
  }
}

void FormMessageFiltersManager::updateFilterOptions(MessageFilter* filter) {
  m_ui.m_btnRemoveSelected->setEnabled(filter != nullptr);
  m_ui.m_btnUp->setEnabled(filter != nullptr && filter->sortOrder() > 0);
  m_ui.m_btnDown->setEnabled(filter != nullptr && filter->sortOrder() < m_reader->messageFilters().size() - 1);
  m_ui.m_btnEnable->setEnabled(filter != nullptr);
  m_ui.m_btnEnable->setChecked(filter != nullptr && filter->enabled());
}

RootItem* FormMessageFiltersManager::selectedCategoryFeed() const {
  return m_feedsModel->sourceModel()->itemForIndex(m_feedsModel->mapToSource(m_ui.m_treeFeeds->currentIndex()));
}
