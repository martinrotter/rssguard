// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "database/sqlquery.h"
#include "filtering/messagefilter.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/reusable/lineeditwithstatus.h"
#include "gui/reusable/texteditwithstatus.h"
#include "gui/tabwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "services/abstract/feed.h"
#include "services/abstract/serviceentrypoint.h"
#include "services/abstract/serviceroot.h"

#include <algorithm>
#include <functional>
#include <memory>

#include <QAction>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QPluginLoader>
#include <QPushButton>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QtConcurrentRun>

namespace {
  class OtherService : public ServiceRoot {
    public:
      QString code() const override {
        return QSL("clone-test-other");
      }

      QList<Message> obtainNewMessages(Feed*,
                                       const QHash<BagOfMessages, QStringList>&,
                                       const QHash<QString, QStringList>&) override {
        return {};
      }
  };

  Feed* feedById(ServiceRoot* account, int id) {
    auto* item = account->getItemFromSubTree([id](const RootItem* candidate) {
      return candidate->kind() == RootItem::Kind::Feed && candidate->id() == id;
    });
    return item == nullptr ? nullptr : item->toFeed();
  }
} // namespace

class TestFeedCloning : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void loadedAccountOwnsGuiThreadActions();
    void actionTargetsSingleStandardFeed();
    void cloneAndEdit_data();
    void cloneAndEdit();
    void failedAssignmentRollsBack();
    void busyUpdateRefusesClone();
    void cleanupTestCase();

  private:
    FeedsView* view() const;
    QModelIndex proxyIndex(RootItem* item) const;
    void select(RootItem* item);
    bool openMenu(RootItem* item,
                  QContextMenuEvent::Reason reason = QContextMenuEvent::Mouse,
                  bool trigger_clone = false,
                  const std::function<void(QDialog*)>& edit = {});
    int savedFeedCount() const;

    QPluginLoader m_plugin;
    ServiceEntryPoint* m_service = nullptr;
    ServiceRoot* m_account = nullptr;
    Feed* m_otherFeed = nullptr;
    Feed* m_rootFeed = nullptr;
    Feed* m_nestedFeed = nullptr;
    MessageFilter* m_filter = nullptr;
    QString m_rssPath;
    QString m_editedRssPath;
    std::unique_ptr<FormMain> m_window;
};

void TestFeedCloning::initTestCase() {
  m_plugin.setFileName(QString::fromUtf8(TEST_STANDARD_PLUGIN));
  m_plugin.setLoadHints(QLibrary::PreventUnloadHint);
  m_service = qobject_cast<ServiceEntryPoint*>(m_plugin.instance());
  QVERIFY2(m_service != nullptr, qPrintable(m_plugin.errorString()));

  m_rssPath = QDir(qApp->userDataFolder()).filePath(QSL("clone-source.xml"));
  m_editedRssPath = QDir(qApp->userDataFolder()).filePath(QSL("edited-source.xml"));
  QFile rss(m_rssPath);
  QVERIFY(rss.open(QIODevice::WriteOnly));
  QVERIFY(rss.write(R"(<rss version="2.0"><channel><title>Clone test</title>
    <link>https://example.invalid/</link><description>Clone test</description>
    <item><title>Same article</title><guid isPermaLink="false">stable-guid</guid>
    <description>A local fixture</description></item></channel></rss>)") > 0);
  rss.close();
  QVERIFY(QFile::copy(m_rssPath, m_editedRssPath));

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("INSERT INTO Accounts (id, ordr, type) VALUES (1, 0, 'std-rss');"));
    query.exec(QSL("INSERT INTO Categories (id, ordr, parent_id, title, account_id, custom_id) "
                   "VALUES (1, 0, -1, 'Clone folder', 1, 'folder');"));
    for (int id : {11, 12}) {
      query.prepare(QSL("INSERT INTO Feeds (id, ordr, title, description, category, source, update_type, "
                        "update_interval, is_quiet, keep_article_customize, keep_article_count, "
                        "exclude_global_unread, account_id, custom_id) "
                        "VALUES (:id, 0, :title, 'Fixture description', :parent, :source, 1, 3760, 1, 1, 42, 1, 1, :custom);"));
      query.bindValue(QSL(":id"), id);
      query.bindValue(QSL(":title"), id == 11 ? QSL("Root source") : QSL("Nested source"));
      query.bindValue(QSL(":parent"), id == 11 ? NO_PARENT_CATEGORY : 1);
      query.bindValue(QSL(":source"), m_rssPath);
      query.bindValue(QSL(":custom"), QSL("original-%1").arg(id));
      query.exec();
      DatabaseQueries::storeCustomData(db,
                                       QSL("Feeds"),
                                       id,
                                       {{QSL("source_type"), 2},
                                        {QSL("type"), 1},
                                        {QSL("encoding"), QSL("UTF-8")},
                                        {QSL("ignore_cookies"), true},
                                        {QSL("use_account_proxy"), true},
                                        {QSL("update_timeout"), 4500},
                                        {QSL("http_headers"), QVariantHash{{QSL("X-Clone-Test"), QSL("copied")}}},
                                        {QSL("mediawiki"), QVariantHash{{QSL("backfill"), QSL("old cursor")}}}});
      query.exec(QSL("INSERT INTO Messages (feed, title, date_created, account_id, custom_id) "
                     "VALUES (%1, 'Old article', 1700000000000, 1, 'old');")
                   .arg(id));
    }
  });

  // Keep the load off the GUI thread even if Qt runs a pending database job in the caller.
  QSemaphore load_started;
  auto load = QtConcurrent::run([&]() {
    load_started.release();
    return m_service->initializeSubtree();
  });
  load_started.acquire();
  const auto accounts = load.result();
  QCOMPARE(accounts.size(), 1);
  m_account = accounts.constFirst();
  auto* reader = qApp->feedReader();
  QVERIFY(reader->feedsModel()->addServiceAccount(m_account, false));
  m_rootFeed = feedById(m_account, 11);
  m_nestedFeed = feedById(m_account, 12);
  QVERIFY(m_rootFeed != nullptr);
  QVERIFY(m_nestedFeed != nullptr);
  QCOMPARE(m_rootFeed->countOfAllMessages(), 1);
  QCOMPARE(m_nestedFeed->countOfAllMessages(), 1);

  m_filter = reader->addMessageFilter(QSL("Shared clone fixture filter"), QSL("return Msg.Accept;"));
  QVERIFY(m_filter != nullptr);
  reader->assignMessageFilterToFeed(m_rootFeed, m_filter);
  reader->assignMessageFilterToFeed(m_nestedFeed, m_filter);

  auto* other = new OtherService();
  other->setTitle(QSL("Other test service"));
  other->setAccountId(2);
  m_otherFeed = new Feed(QSL("Other service feed"), QSL("other"), {}, other);
  m_otherFeed->setId(99);
  other->appendChild(m_otherFeed);
  QVERIFY(reader->feedsModel()->addServiceAccount(other, false));

  m_window = std::make_unique<FormMain>();
  m_window->resize(1000, 700);
  m_window->show();
  view()->expandAll();
  QTRY_VERIFY(m_window->isVisible());
}

FeedsView* TestFeedCloning::view() const {
  return m_window->tabWidget()->feedMessageViewer()->feedsView();
}

QModelIndex TestFeedCloning::proxyIndex(RootItem* item) const {
  return view()->model()->mapFromSource(view()->sourceModel()->indexForItem(item));
}

void TestFeedCloning::select(RootItem* item) {
  view()->selectionModel()->setCurrentIndex(proxyIndex(item),
                                            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

bool TestFeedCloning::openMenu(RootItem* item,
                              QContextMenuEvent::Reason reason,
                              bool trigger_clone,
                              const std::function<void(QDialog*)>& edit) {
  const auto index = proxyIndex(item);
  view()->scrollTo(index);
  const QPoint position = view()->visualRect(index).center();
  bool menu_opened = false;
  bool found = false;
  QTimer popup_timer;
  popup_timer.setSingleShot(true);
  connect(&popup_timer, &QTimer::timeout, this, [&]() {
    auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
    if (menu == nullptr) {
      return;
    }
    const auto close_menu = qScopeGuard([menu]() {
      if (menu->isVisible()) {
        menu->close();
      }
    });

    menu_opened = true;
    auto* action = menu->findChild<QAction*>(QSL("m_actionCloneFeed"));
    found = action != nullptr && menu->actions().contains(action);
    if (found && trigger_clone) {
      QSignalSpy triggered(action, &QAction::triggered);
      QTimer dialog_timer;
      dialog_timer.setSingleShot(true);
      connect(&dialog_timer, &QTimer::timeout, this, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (dialog != nullptr) {
          if (dialog->findChild<LineEditWithStatus*>(QSL("m_txtTitle")) != nullptr) {
            if (edit) {
              edit(dialog);
            }
            else {
              QTest::qFail("The clone editor opened after a failed clone", __FILE__, __LINE__);
            }
          }
          if (dialog->isVisible()) {
            dialog->reject();
          }
        }
      });
      dialog_timer.start(0);
      menu->setActiveAction(action);
      QTest::keyClick(menu, Qt::Key_Return);
      QCOMPARE(triggered.count(), 1);
    }
  });
  popup_timer.start(0);
  QContextMenuEvent event(reason, position, view()->viewport()->mapToGlobal(position));
  QApplication::sendEvent(view()->viewport(), &event);
  if (!menu_opened) {
    QTest::qFail("The feed context menu did not open", __FILE__, __LINE__);
  }
  return found;
}

int TestFeedCloning::savedFeedCount() const {
  return qApp->database()->worker()->read<int>([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("SELECT COUNT(*) FROM Feeds WHERE account_id = 1;"));
    return query.next() ? query.value(0).toInt() : -1;
  });
}

void TestFeedCloning::loadedAccountOwnsGuiThreadActions() {
  QCOMPARE(m_account->thread(), qApp->thread());
  for (auto* item : m_account->childItems()) {
    if (item->kind() != RootItem::Kind::Feed && item->kind() != RootItem::Kind::Category) {
      QCOMPARE(item->thread(), qApp->thread());
    }
  }

  QMenu menu;
  const auto actions = m_account->contextMenuFeedsList({m_rootFeed}, &menu);
  const auto metadata_title = QCoreApplication::translate("StandardServiceRoot", "Fetch metadata");
  const auto metadata = std::find_if(actions.cbegin(), actions.cend(), [&metadata_title](const QAction* action) {
    return action->text() == metadata_title;
  });
  QVERIFY(metadata != actions.cend());
  QCOMPARE((*metadata)->thread(), qApp->thread());
  QCOMPARE((*metadata)->parent(), static_cast<QObject*>(m_account));
}

void TestFeedCloning::actionTargetsSingleStandardFeed() {
  QVERIFY(openMenu(m_rootFeed));
  QVERIFY(!openMenu(m_account));
  QVERIFY(!openMenu(m_nestedFeed->parent()));
  QVERIFY(!openMenu(m_account->getItemFromSubTree([](const RootItem* item) {
    return item->kind() == RootItem::Kind::Unread;
  })));
  QVERIFY(!openMenu(m_otherFeed));

  select(m_rootFeed);
  view()->selectionModel()->setCurrentIndex(proxyIndex(m_nestedFeed), QItemSelectionModel::Select |
                                                                       QItemSelectionModel::Rows);
  QCOMPARE(view()->selectionModel()->selectedRows().size(), 2);
  QVERIFY(!openMenu(m_nestedFeed));
  QCOMPARE(savedFeedCount(), 2);
}

void TestFeedCloning::cloneAndEdit_data() {
  QTest::addColumn<bool>("save_edits");
  QTest::newRow("right-click-root-and-cancel") << false;
  QTest::newRow("keyboard-nested-and-save") << true;
}

void TestFeedCloning::cloneAndEdit() {
  QFETCH(bool, save_edits);
  auto* original = save_edits ? m_nestedFeed : m_rootFeed;
  QVERIFY(!qApp->feedUpdateLock()->isLocked());
  const QString original_title = original->title();
  const auto siblings_before = original->parent()->childItems();
  const int feed_count = savedFeedCount();
  original->setStatus(Feed::Status::NetworkError, QSL("Original error"));
  select(m_nestedFeed);

  Feed* clone = nullptr;
  const auto reason = save_edits ? QContextMenuEvent::Keyboard : QContextMenuEvent::Mouse;
  QVERIFY(openMenu(m_rootFeed, reason, true, [&](QDialog* dialog) {
    const auto siblings = original->parent()->childItems();
    QCOMPARE(siblings.size(), siblings_before.size() + 1);
    for (auto* item : siblings) {
      if (!siblings_before.contains(item) && item->kind() == RootItem::Kind::Feed) {
        clone = item->toFeed();
        break;
      }
    }
    QVERIFY(clone != nullptr);
    QCOMPARE(savedFeedCount(), feed_count + 1);
    QCOMPARE(clone->parent(), original->parent());
    QVERIFY(clone->id() > 0 && clone->id() != original->id());
    QVERIFY(clone->customId() != original->customId());
    QVERIFY(proxyIndex(clone).isValid());
    QCOMPARE(clone->countOfAllMessages(), 0);
    QCOMPARE(clone->countOfUnreadMessages(), 0);
    QCOMPARE(clone->status(), Feed::Status::Normal);
    QCOMPARE(clone->messageFilters().size(), 1);
    QCOMPARE(clone->messageFilters().constFirst().data(), m_filter);

    auto* title = dialog->findChild<LineEditWithStatus*>(QSL("m_txtTitle"))->lineEdit();
    auto* source = dialog->findChild<TextEditWithStatus*>(QSL("m_txtSource"));
    QVERIFY(source != nullptr);
    QCOMPARE(title->text(), clone->title());
    QVERIFY(title->text().contains(original_title));
    QVERIFY(title->text() != original_title);
    QCOMPARE(source->textEdit()->toPlainText(), m_rssPath);

    if (save_edits) {
      title->setText(QSL("Edited copy"));
      source->textEdit()->setPlainText(m_editedRssPath);
      auto* buttons = dialog->findChild<QDialogButtonBox*>();
      QVERIFY(buttons != nullptr && buttons->button(QDialogButtonBox::Ok) != nullptr);
      buttons->button(QDialogButtonBox::Ok)->click();
      QCOMPARE(dialog->result(), int(QDialog::Accepted));
    }
  }));
  QVERIFY(clone != nullptr);
  QCOMPARE(original->title(), original_title);
  QCOMPARE(original->source(), m_rssPath);
  QCOMPARE(clone->source(), save_edits ? m_editedRssPath : m_rssPath);
  if (save_edits) {
    QCOMPARE(clone->title(), QSL("Edited copy"));
  }

  const auto reloaded = m_service->initializeSubtree();
  const auto cleanup = qScopeGuard([&]() { qDeleteAll(reloaded); });
  QCOMPARE(reloaded.size(), 1);
  auto* account = reloaded.constFirst();
  account->start(false);
  auto* stored = feedById(account, clone->id());
  QVERIFY(stored != nullptr);
  QCOMPARE(stored->parent()->id(), original->parent()->id());
  QCOMPARE(stored->customId(), clone->customId());
  QCOMPARE(stored->title(), clone->title());
  QCOMPARE(stored->source(), clone->source());
  QCOMPARE(stored->description(), original->description());
  QCOMPARE(stored->autoUpdateType(), original->autoUpdateType());
  QCOMPARE(stored->autoUpdateInterval(), original->autoUpdateInterval());
  QCOMPARE(stored->isQuiet(), original->isQuiet());
  QCOMPARE(stored->excludesFromGlobalUnreadCount(), original->excludesFromGlobalUnreadCount());
  QCOMPARE(stored->articleIgnoreLimit().m_keepCountOfArticles, original->articleIgnoreLimit().m_keepCountOfArticles);
  QCOMPARE(stored->messageFilters().size(), 1);
  QCOMPARE(stored->messageFilters().constFirst().data(), m_filter);
  QCOMPARE(stored->countOfAllMessages(), 0);

  const auto settings = stored->customDatabaseData();
  const auto original_settings = original->customDatabaseData();
  for (const auto& key : {QSL("source_type"), QSL("type"), QSL("encoding"), QSL("http_headers"),
                         QSL("ignore_cookies"), QSL("use_account_proxy"), QSL("update_timeout")}) {
    QCOMPARE(settings.value(key), original_settings.value(key));
  }
  QVERIFY(original_settings.contains(QSL("mediawiki")));
  QVERIFY(!settings.contains(QSL("mediawiki")));

  if (!save_edits) {
    auto first = m_account->obtainNewMessages(original, {}, {});
    auto second = m_account->obtainNewMessages(clone, {}, {});
    QCOMPARE(first.size(), 1);
    QCOMPARE(second.size(), 1);
    QCOMPARE(first.constFirst().m_customId, second.constFirst().m_customId);
    const auto original_result = DatabaseQueries::updateMessages(qApp->database(), qApp->settings(), first,
                                                                  original, false, false);
    const auto clone_result = DatabaseQueries::updateMessages(qApp->database(), qApp->settings(), second,
                                                               clone, false, false);
    original_result.throwIfFailed();
    clone_result.throwIfFailed();
    QCOMPARE(original_result.m_all.size(), 1);
    QCOMPARE(clone_result.m_all.size(), 1);
    const int clone_article_id = clone_result.m_all.constFirst().m_id;
    QVERIFY(clone_article_id > 0 && clone_article_id != original_result.m_all.constFirst().m_id);
    qApp->database()->worker()->write([&](const QSqlDatabase& db) {
      DatabaseQueries::markMessagesReadUnread(db, {QString::number(clone_article_id)}, RootItem::ReadStatus::Read);
    });
    original->updateCounts();
    clone->updateCounts();
    QCOMPARE(original->countOfAllMessages(), 2);
    QCOMPARE(original->countOfUnreadMessages(), 2);
    QCOMPARE(clone->countOfAllMessages(), 1);
    QCOMPARE(clone->countOfUnreadMessages(), 0);
  }
}

void TestFeedCloning::failedAssignmentRollsBack() {
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("CREATE TRIGGER fail_clone_assignment BEFORE INSERT ON MessageFiltersInFeeds "
                   "BEGIN SELECT RAISE(ABORT, 'Forced filter assignment failure'); END;"));
  });
  const auto drop_trigger = qScopeGuard([]() {
    qApp->database()->worker()->write([](const QSqlDatabase& db) {
      SqlQuery query(db);
      query.exec(QSL("DROP TRIGGER fail_clone_assignment;"));
    });
  });
  const int original_count = savedFeedCount();
  QSignalSpy rows_inserted(view()->sourceModel(), &QAbstractItemModel::rowsInserted);
  QSignalSpy locks(qApp->feedUpdateLock(), &Mutex::locked);
  QVERIFY(openMenu(m_rootFeed, QContextMenuEvent::Mouse, true));
  QCOMPARE(locks.count(), 1);
  QCOMPARE(savedFeedCount(), original_count);
  QCOMPARE(rows_inserted.count(), 0);
  QVERIFY(!qApp->feedUpdateLock()->isLocked());
}

void TestFeedCloning::busyUpdateRefusesClone() {
  QVERIFY(qApp->feedUpdateLock()->tryLock());
  const auto unlock = qScopeGuard([]() { qApp->feedUpdateLock()->unlock(); });
  const int original_count = savedFeedCount();
  QVERIFY(openMenu(m_nestedFeed, QContextMenuEvent::Mouse, true));
  QCOMPARE(savedFeedCount(), original_count);
}

void TestFeedCloning::cleanupTestCase() {
  if (m_window != nullptr) {
    m_window->hide();
    QCoreApplication::processEvents();
    m_window.reset();
  }
}

int main(int argc, char* argv[]) {
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
  }
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  QDir().mkpath(directory.filePath(QSL(APP_CFG_PATH)));
  QSettings settings(directory.filePath(QSL(APP_CFG_PATH) + QL1C('/') + QSL(APP_CFG_FILE)), QSettings::IniFormat);
  settings.setValue(GROUP(General) + QL1C('/') + General::ShowSplashScreen, false);
  settings.setValue(GROUP(General) + QL1C('/') + General::UpdateOnStartup, false);
  settings.setValue(GROUP(GUI) + QL1C('/') + GUI::UseTrayIcon, false);
  settings.setValue(GROUP(GUI) + QL1C('/') + GUI::UseToastNotifications, false);
  settings.setValue(GROUP(Feeds) + QL1C('/') + Feeds::FeedsUpdateOnStartup, false);
  settings.setValue(GROUP(Feeds) + QL1C('/') + Feeds::AutoUpdateEnabled, false);
  settings.sync();
  const QStringList arguments{QString::fromLocal8Bit(argv[0]),
                              QSL("--data"),
                              directory.path(),
                              QSL("--no-single-instance"),
                              QSL("--force-text-browser")};
  Application application(QSL("rssguard-feed-cloning-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  application.setFeedReader(new FeedReader(&application));
  TestFeedCloning tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_feedcloning.moc"
