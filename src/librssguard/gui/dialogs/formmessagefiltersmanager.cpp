// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formmessagefiltersmanager.h"

#include "3rd-party/boolinq/boolinq.h"
#include "core/messagefilter.h"
#include "core/messagesforfiltersmodel.h"
#include "database/databasequeries.h"
#include "exceptions/filteringexception.h"
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

FormMessageFiltersManager::FormMessageFiltersManager(FeedReader* reader,
                                                     const QList<ServiceRoot*>& accounts,
                                                     QWidget* parent)
  : QDialog(parent), m_feedsModel(new AccountCheckSortedModel(this)), m_rootItem(new RootItem()), m_accounts(accounts),
    m_reader(reader), m_loadingFilter(false), m_msgModel(new MessagesForFiltersModel(this)) {
  m_ui.setupUi(this);

  m_highlighter = new JsSyntaxHighlighter(m_ui.m_txtScript->document());

  std::sort(m_accounts.begin(), m_accounts.end(), [](const ServiceRoot* lhs, const ServiceRoot* rhs) {
    return lhs->title().compare(rhs->title(), Qt::CaseSensitivity::CaseInsensitive) < 0;
  });

  // TODO: Add sorting.
  m_ui.m_treeExistingMessages->setModel(m_msgModel);

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
  m_ui.m_txtScript->setFont(QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont));
  m_ui.m_treeExistingMessages->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_ISREAD,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_ISIMPORTANT,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_ISDELETED,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_AUTHOR,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_CREATED,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_SCORE,
                                                              QHeaderView::ResizeMode::ResizeToContents);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_TITLE, QHeaderView::ResizeMode::Interactive);
  m_ui.m_treeExistingMessages->header()->setSectionResizeMode(MFM_MODEL_URL, QHeaderView::ResizeMode::Interactive);

  connect(m_ui.m_btnDetailedHelp, &QPushButton::clicked, this, []() {
    qApp->web()->openUrlInExternalBrowser(QSL(MSG_FILTERING_HELP));
  });
  connect(m_ui.m_listFilters, &QListWidget::currentRowChanged, this, &FormMessageFiltersManager::loadFilter);
  connect(m_ui.m_btnAddNew, &QPushButton::clicked, this, [this]() {
    addNewFilter();
  });
  connect(m_ui.m_btnRemoveSelected, &QPushButton::clicked, this, &FormMessageFiltersManager::removeSelectedFilter);
  connect(m_ui.m_txtTitle, &QLineEdit::textChanged, this, &FormMessageFiltersManager::saveSelectedFilter);
  connect(m_ui.m_txtScript, &QPlainTextEdit::textChanged, this, &FormMessageFiltersManager::saveSelectedFilter);
  connect(m_ui.m_btnTest, &QPushButton::clicked, this, &FormMessageFiltersManager::testFilter);
  connect(m_ui.m_btnBeautify, &QPushButton::clicked, this, &FormMessageFiltersManager::beautifyScript);
  connect(m_ui.m_cmbAccounts,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &FormMessageFiltersManager::onAccountChanged);
  connect(m_ui.m_btnCheckAll, &QPushButton::clicked, m_feedsModel->sourceModel(), &AccountCheckModel::checkAllItems);
  connect(m_ui.m_btnUncheckAll,
          &QPushButton::clicked,
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

  connect(m_ui.m_searchWidget, &SearchTextWidget::searchCancelled, this, [this]() {
    m_ui.m_txtScript->find(QString());
  });
  connect(m_ui.m_searchWidget, &SearchTextWidget::searchForText, this, [this](const QString& text, bool backwards) {
    m_ui.m_txtScript->find(text, backwards ? QTextDocument::FindFlag::FindBackward : QTextDocument::FindFlags());
    m_ui.m_searchWidget->setFocus();
  });

  m_ui.m_txtScript->installEventFilter(this);
  m_ui.m_searchWidget->hide();

  initializeTestingMessage();
  initializePremadeFilters();
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
        "    return MessageObject.Accept;\n"
        "  }\n"
        "  else {\n"
        "    return MessageObject.Ignore;\n"
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

  for (auto* fltr : std::as_const(flt)) {
    auto* it = new QListWidgetItem(fltr->name(), m_ui.m_listFilters);

    it->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue<MessageFilter*>(fltr));
  }
}

void FormMessageFiltersManager::addNewFilter(const QString& filter_script) {
  try {
    auto* fltr = m_reader->addMessageFilter(tr("New article filter"),
                                            filter_script.isEmpty()
                                              ? QSL("function filterMessage() { return MessageObject.Accept; }")
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

  fltr->setName(m_ui.m_txtTitle->text());
  fltr->setScript(m_ui.m_txtScript->toPlainText());
  m_ui.m_listFilters->currentItem()->setText(fltr->name());

  m_reader->updateMessageFilter(fltr);
}

void FormMessageFiltersManager::loadFilter() {
  auto* filter = selectedFilter();
  auto* acc = selectedAccount();

  loadAccount(acc);
  showFilter(filter);
  loadFilterFeedAssignments(filter, acc);
}

void FormMessageFiltersManager::testFilter() {
  m_ui.m_txtErrors->clear();

  // Perform per-message filtering.
  auto* selected_fd_cat = selectedCategoryFeed();
  QJSEngine filter_engine;
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());
  MessageObject msg_obj(&database,
                        selected_fd_cat->kind() == RootItem::Kind::Feed ? selected_fd_cat->toFeed() : nullptr,
                        selectedAccount(),
                        false);
  auto* fltr = selectedFilter();

  MessageFilter::initializeFilteringEngine(filter_engine, &msg_obj);

  // Test real messages.
  try {
    m_msgModel->testFilter(fltr, &filter_engine, &msg_obj);
  }
  catch (const FilteringException& ex) {
    m_ui.m_txtErrors->setTextColor(Qt::GlobalColor::red);
    m_ui.m_txtErrors->insertPlainText(tr("EXISTING articles filtering error: '%1'.\n").arg(ex.message()));

    // See output.
    m_ui.m_twMessages->setCurrentIndex(2);
  }

  // Test sample message.
  Message msg = testingMessage();

  msg_obj.setMessage(&msg);

  try {
    MessageObject::FilteringAction decision = fltr->filterMessage(&filter_engine);

    m_ui.m_txtErrors->setTextColor(decision == MessageObject::FilteringAction::Accept ? Qt::GlobalColor::darkGreen
                                                                                      : Qt::GlobalColor::red);

    QString answer = tr("Article will be %1.\n\n")
                       .arg(decision == MessageObject::FilteringAction::Accept ? tr("ACCEPTED") : tr("REJECTED"));

    answer += tr("Output (modified) article is:\n"
                 "  Title = '%1'\n"
                 "  URL = '%2'\n"
                 "  Author = '%3'\n"
                 "  Is read/important = '%4/%5'\n"
                 "  Created on = '%6'\n"
                 "  Contents = '%7'\n"
                 "  RAW contents = '%8'")
                .arg(msg.m_title,
                     msg.m_url,
                     msg.m_author,
                     msg.m_isRead ? tr("yes") : tr("no"),
                     msg.m_isImportant ? tr("yes") : tr("no"),
                     QString::number(msg.m_created.toMSecsSinceEpoch()),
                     msg.m_contents,
                     msg.m_rawContents);

    m_ui.m_txtErrors->insertPlainText(answer);
  }
  catch (const FilteringException& ex) {
    m_ui.m_txtErrors->setTextColor(Qt::GlobalColor::red);
    m_ui.m_txtErrors->insertPlainText(tr("SAMPLE article filtering error: '%1'.\n").arg(ex.message()));

    // See output.
    m_ui.m_twMessages->setCurrentIndex(2);
  }
}

void FormMessageFiltersManager::displayMessagesOfFeed() {
  auto* item = selectedCategoryFeed();

  if (item != nullptr) {
    m_msgModel->setMessages(item->undeletedMessages());
  }
  else {
    m_msgModel->setMessages({});
  }
}

void FormMessageFiltersManager::processCheckedFeeds() {
  QList<RootItem*> checked = m_feedsModel->sourceModel()->checkedItems();
  auto* fltr = selectedFilter();
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  for (RootItem* it : checked) {
    if (it->kind() == RootItem::Kind::Feed) {
      QJSEngine filter_engine;
      MessageObject msg_obj(&database, it->toFeed(), selectedAccount(), false);

      MessageFilter::initializeFilteringEngine(filter_engine, &msg_obj);

      // We process messages of the feed.
      QList<Message> msgs = it->undeletedMessages();
      QList<Message> read_msgs, important_msgs;

      for (int i = 0; i < msgs.size(); i++) {
        auto labels_in_message = DatabaseQueries::getLabelsForMessage(database, msgs[i], msg_obj.availableLabels());

        // Create backup of message.
        Message* msg = &msgs[i];

        msg->m_assignedLabels = labels_in_message;
        msg->m_rawContents = Message::generateRawAtomContents(*msg);

        Message msg_backup(*msg);

        msg_obj.setMessage(msg);

        bool remove_from_list = false;

        try {
          MessageObject::FilteringAction result = fltr->filterMessage(&filter_engine);

          if (result == MessageObject::FilteringAction::Purge) {
            remove_from_list = true;

            // Purge the message completely and remove leftovers.
            DatabaseQueries::purgeMessage(database, msg->m_id);
          }
          else if (result == MessageObject::FilteringAction::Ignore) {
            remove_from_list = true;
          }
        }
        catch (const FilteringException& ex) {
          qCriticalNN << LOGSEC_CORE << "Error when running script when processing existing messages:"
                      << QUOTE_W_SPACE_DOT(ex.message());

          continue;
        }

        if (!msg_backup.m_isRead && msg->m_isRead) {
          qDebugNN << LOGSEC_FEEDDOWNLOADER << "Message with custom ID: '" << msg_backup.m_customId
                   << "' was marked as read by message scripts.";

          read_msgs << *msg;
        }

        if (!msg_backup.m_isImportant && msg->m_isImportant) {
          qDebugNN << LOGSEC_FEEDDOWNLOADER << "Message with custom ID: '" << msg_backup.m_customId
                   << "' was marked as important by message scripts.";

          important_msgs << *msg;
        }

        // Process changed labels.
        for (Label* lbl : std::as_const(msg_backup.m_assignedLabels)) {
          if (!msg->m_assignedLabels.contains(lbl)) {
            // Label is not there anymore, it was deassigned.
            lbl->deassignFromMessage(*msg);

            qDebugNN << LOGSEC_FEEDDOWNLOADER << "It was detected that label" << QUOTE_W_SPACE(lbl->customId())
                     << "was DEASSIGNED from message" << QUOTE_W_SPACE(msg->m_customId) << "by message filter(s).";
          }
        }

        for (Label* lbl : std::as_const(msg->m_assignedLabels)) {
          if (!msg_backup.m_assignedLabels.contains(lbl)) {
            // Label is in new message, but is not in old message, it
            // was newly assigned.
            lbl->assignToMessage(*msg);

            qDebugNN << LOGSEC_FEEDDOWNLOADER << "It was detected that label" << QUOTE_W_SPACE(lbl->customId())
                     << "was ASSIGNED to message" << QUOTE_W_SPACE(msg->m_customId) << "by message filter(s).";
          }
        }

        if (remove_from_list) {
          // Do not update message.
          msgs.removeAt(i--);
        }
      }

      if (!read_msgs.isEmpty()) {
        // Now we push new read states to the service.
        if (it->getParentServiceRoot()->onBeforeSetMessagesRead(it, read_msgs, RootItem::ReadStatus::Read)) {
          qDebugNN << LOGSEC_FEEDDOWNLOADER << "Notified services about messages marked as read by message filters.";
        }
        else {
          qCriticalNN << LOGSEC_FEEDDOWNLOADER
                      << "Notification of services about messages marked as read by message filters FAILED.";
        }
      }

      if (!important_msgs.isEmpty()) {
        // Now we push new read states to the service.
        auto list = boolinq::from(important_msgs)
                      .select([](const Message& msg) {
                        return ImportanceChange(msg, RootItem::Importance::Important);
                      })
                      .toStdList();
        QList<ImportanceChange> chngs = FROM_STD_LIST(QList<ImportanceChange>, list);

        if (it->getParentServiceRoot()->onBeforeSwitchMessageImportance(it, chngs)) {
          qDebugNN << LOGSEC_FEEDDOWNLOADER
                   << "Notified services about messages marked as important by message filters.";
        }
        else {
          qCriticalNN << LOGSEC_FEEDDOWNLOADER
                      << "Notification of services about messages marked as important by message filters FAILED.";
        }
      }

      // Update messages in DB and reload selection.
      it->getParentServiceRoot()->updateMessages(msgs, it->toFeed(), true, nullptr);
      displayMessagesOfFeed();
    }
  }
}

void FormMessageFiltersManager::loadAccount(ServiceRoot* account) {
  m_feedsModel->setRootItem(account, false, true);

  if (account != nullptr) {
    m_msgModel->setMessages(account->undeletedMessages());
  }
  else {
    m_msgModel->setMessages({});
  }
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
  m_ui.m_twMessages->setCurrentIndex(0);
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
  proc_clang_format.setProgram(qApp->applicationDirPath() + QDir::separator() + QSL("clang-format") +
                               QDir::separator() + QSL("clang-format.exe"));
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

void FormMessageFiltersManager::insertPremadeFilter(QAction* act_filter) {
  QString filter_path = QSL(":/scripts/filters/") + act_filter->text();

  try {
    m_ui.m_txtScript->setPlainText(QString::fromUtf8(IOFactory::readFile(filter_path)));
  }
  catch (...) {
  }
}

void FormMessageFiltersManager::initializePremadeFilters() {
  QMenu* mn_filters = new QMenu(this);

  connect(mn_filters, &QMenu::triggered, this, &FormMessageFiltersManager::insertPremadeFilter);

  auto js_files = QDir(QSL(":/scripts/filters")).entryList();

  for (const QString& js_file : js_files) {
    mn_filters->addAction(js_file);
  }

  m_ui.m_btnPremadeFilters->setMenu(mn_filters);
}

void FormMessageFiltersManager::initializeTestingMessage() {
  m_ui.m_cbSampleImportant->setChecked(true);
  m_ui.m_txtSampleUrl->setText(QSL("https://mynews.com/news/5"));
  m_ui.m_txtSampleTitle->setText(QSL("Year of Linux Desktop"));
  m_ui.m_txtSampleAuthor->setText(QSL("Napoleon Bonaparte"));
  m_ui.m_txtSampleContents->setPlainText(QSL("<p>Browsers usually insert quotation marks around the q element.</p>"
                                             "<p>WWF's goal is to: <q>Build a future where people live in harmony "
                                             "with nature.</q></p>"));
  m_ui.m_txtSampleCreatedOn->setText(QString::number(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));
}

RootItem* FormMessageFiltersManager::selectedCategoryFeed() const {
  return m_feedsModel->sourceModel()->itemForIndex(m_feedsModel->mapToSource(m_ui.m_treeFeeds->currentIndex()));
}

Message FormMessageFiltersManager::testingMessage() const {
  Message msg;

  msg.m_feedId = QString::number(NO_PARENT_CATEGORY);
  msg.m_url = m_ui.m_txtSampleUrl->text();
  msg.m_customId = m_ui.m_txtSampleUrl->text();
  msg.m_title = m_ui.m_txtSampleTitle->text();
  msg.m_author = m_ui.m_txtSampleAuthor->text();
  msg.m_isRead = m_ui.m_cbSampleRead->isChecked();
  msg.m_isImportant = m_ui.m_cbSampleImportant->isChecked();
  msg.m_created = QDateTime::fromMSecsSinceEpoch(m_ui.m_txtSampleCreatedOn->text().toLongLong());
  msg.m_contents = m_ui.m_txtSampleContents->toPlainText();
  msg.m_rawContents = Message::generateRawAtomContents(msg);

  return msg;
}
