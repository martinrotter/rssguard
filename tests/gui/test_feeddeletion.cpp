// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "database/databasefactory.h"
#include "database/sqlquery.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/messagesview.h"
#include "gui/tabwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "services/abstract/serviceentrypoint.h"
#include "services/abstract/serviceroot.h"

#include <atomic>
#include <memory>

#include <QDir>
#include <QPixmap>
#include <QPluginLoader>
#include <QPointer>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace {
  std::atomic<int> inconsistentModelWarnings{0};
  QtMessageHandler previousMessageHandler = nullptr;

  void observeModelWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (message.contains(QSL("QSortFilterProxyModel: inconsistent changes reported by source model"))) {
      ++inconsistentModelWarnings;
    }
    if (previousMessageHandler != nullptr) {
      previousMessageHandler(type, context, message);
    }
  }

  void prepareProfile(const QString& profile) {
    QDir().mkpath(profile + QDir::separator() + QSL(APP_CFG_PATH));
    QSettings settings(profile + QDir::separator() + QSL(APP_CFG_PATH) + QDir::separator() + QSL(APP_CFG_FILE),
                       QSettings::IniFormat);
    settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
    settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
    settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseTrayIcon, false);
    settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
    settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::FeedsUpdateOnStartup, false);
    settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::AutoUpdateEnabled, false);
    settings.setValue(GROUP(Messages) + QSL("/") + Messages::ArticleMarkOnSelection,
                      int(MessagesView::ArticleMarkingPolicy::MarkOnlyManually));
    settings.setValue(GROUP(MessageBoxDontShows) + QSL("/delete_selected_items"), true);
    settings.sync();
  }
} // namespace

class TestFeedDeletion : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void deleteMultipleFeeds_data();
    void deleteMultipleFeeds();
    void cleanupTestCase();

  private:
    QPluginLoader m_plugin;
    QHash<int, ServiceRoot*> m_accounts;
    std::unique_ptr<FormMain> m_window;
};

void TestFeedDeletion::initTestCase() {
  previousMessageHandler = qInstallMessageHandler(observeModelWarnings);
  m_plugin.setFileName(QString::fromUtf8(TEST_STANDARD_PLUGIN));
  m_plugin.setLoadHints(QLibrary::PreventUnloadHint);
  auto* service = qobject_cast<ServiceEntryPoint*>(m_plugin.instance());
  QVERIFY2(service != nullptr, qPrintable(m_plugin.errorString()));

  // Each data row has its own account. Load the fixtures through the real RSS plugin,
  // so deletion exercises the database, source model, proxy, selection, and article list.
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    for (int account = 1; account <= 4; ++account) {
      query.exec(QSL("INSERT INTO Accounts (id,ordr,type) VALUES (%1,%2,'std-rss');").arg(account).arg(account - 1));
      query.exec(QSL("INSERT INTO Categories (id,ordr,parent_id,title,account_id,custom_id) "
                     "VALUES (%1,0,-1,'Deletion test folder',%1,'folder');")
                   .arg(account));
      for (int i = 0; i < 6; ++i) {
        const int feed_id = account * 10 + i;
        query.exec(QSL("INSERT INTO Feeds (id,ordr,title,category,source,update_type,account_id,custom_id) "
                       "VALUES (%1,%2,'%3 test feed',%4,'https://example.invalid/%1',0,%5,'%1');")
                     .arg(feed_id)
                     .arg(i < 4 ? i : i - 4)
                     .arg(QChar('A' + i))
                     .arg(i < 4 ? account : NO_PARENT_CATEGORY)
                     .arg(account));
        query.exec(QSL("INSERT INTO Messages (feed,title,date_created,account_id,is_read,custom_id) "
                       "VALUES (%1,'Deletion test article',1700000000000,%2,%3,'article-%1');")
                     .arg(feed_id)
                     .arg(account)
                     .arg(i % 2));
      }
    }
  });
  auto* source = qApp->feedReader()->feedsModel();
  for (auto* account : service->initializeSubtree()) {
    m_accounts.insert(account->accountId(), account);
    QVERIFY(source->addServiceAccount(account, false));
  }
  QCOMPARE(m_accounts.size(), 4);
  m_window = std::make_unique<FormMain>();
  m_window->resize(1000, 700);
  m_window->show();
  QTRY_VERIFY(m_window->isVisible());
}

void TestFeedDeletion::deleteMultipleFeeds_data() {
  QTest::addColumn<int>("account_id");
  QTest::addColumn<bool>("in_folder");
  QTest::addColumn<bool>("reverse_selection");
  QTest::newRow("root-current-second") << 1 << false << false;
  QTest::newRow("folder-current-second") << 2 << true << false;
  QTest::newRow("root-current-first") << 3 << false << true;
  QTest::newRow("folder-current-first") << 4 << true << true;
}

void TestFeedDeletion::deleteMultipleFeeds() {
  QFETCH(int, account_id);
  QFETCH(bool, in_folder);
  QFETCH(bool, reverse_selection);
  auto* account = m_accounts.value(account_id);
  QVERIFY(account != nullptr);
  auto* component = m_window->tabWidget()->feedMessageViewer();
  auto* view = component->feedsView();
  auto* source = qApp->feedReader()->feedsModel();
  auto* proxy = qApp->feedReader()->feedsProxyModel();
  QVERIFY(proxy->dynamicSortFilter());
  const auto original_feeds = account->getSubTreeFeeds();
  QCOMPARE(original_feeds.size(), 6);
  const int first_id = account_id * 10 + (in_folder ? 0 : 4);
  const int second_id = first_id + 1;
  Feed* first = nullptr;
  Feed* second = nullptr;
  for (auto* feed : original_feeds) {
    if (feed->id() == first_id) {
      first = feed;
    }
    if (feed->id() == second_id) {
      second = feed;
    }
  }
  QVERIFY(first != nullptr);
  QVERIFY(second != nullptr);
  QPointer<Feed> first_guard(first);
  QPointer<Feed> second_guard(second);
  const auto first_index = proxy->mapFromSource(source->indexForItem(reverse_selection ? second : first));
  const auto second_index = proxy->mapFromSource(source->indexForItem(reverse_selection ? first : second));
  QVERIFY(first_index.isValid());
  QVERIFY(second_index.isValid());
  view->selectionModel()->setCurrentIndex(first_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  view->selectionModel()->setCurrentIndex(second_index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
  QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
  QCOMPARE(view->selectedItem(), reverse_selection ? first : second);
  QCOMPARE(component->messagesView()->model()->rowCount(), 1);

  inconsistentModelWarnings = 0;
  QSignalSpy source_removals(source, &QAbstractItemModel::rowsRemoved);
  QSignalSpy proxy_resets(proxy, &QAbstractItemModel::modelReset);
  view->deleteSelectedItem();
  QCOMPARE(inconsistentModelWarnings.load(), 0);
  QCOMPARE(source_removals.count(), 2);
  QCOMPARE(proxy_resets.count(), 0);
  QVERIFY(first_guard.isNull());
  QVERIFY(second_guard.isNull());
  QCOMPARE(account->getSubTreeFeeds().size(), 4);
  QVERIFY(view->selectionModel()->selectedRows().isEmpty());
  QVERIFY(view->selectedItem() == nullptr);
  // A stale loaded feed leaves cached articles behind and crashes on later rendering.
  QCOMPARE(component->messagesView()->model()->rowCount(), 0);
  const int remaining_rows = qApp->database()->worker()->read<int>([=](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("SELECT COUNT(*) FROM Feeds WHERE account_id = %1;").arg(account_id));
    return query.next() ? query.value(0).toInt() : -1;
  });
  QCOMPARE(remaining_rows, 4);
  const int deleted_articles = qApp->database()->worker()->read<int>([=](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("SELECT COUNT(*) FROM Messages WHERE feed IN (%1,%2);").arg(first_id).arg(second_id));
    return query.next() ? query.value(0).toInt() : -1;
  });
  QCOMPARE(deleted_articles, 0);
  for (auto* survivor : account->getSubTreeFeeds()) {
    const auto index = source->indexForItem(survivor);
    QVERIFY(index.isValid());
    QVERIFY(proxy->mapFromSource(index).isValid());
  }
  QCoreApplication::processEvents();
  QPixmap image(m_window->size());
  m_window->render(&image);
  QCOMPARE(inconsistentModelWarnings.load(), 0);
}

void TestFeedDeletion::cleanupTestCase() {
  if (m_window != nullptr) {
    // Deliver hide notifications while the main window is still fully alive.
    m_window->hide();
    QCoreApplication::processEvents();
    m_window.reset();
  }
  qInstallMessageHandler(previousMessageHandler);
}

int main(int argc, char* argv[]) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  prepareProfile(directory.path());
  QStringList arguments{QString::fromLocal8Bit(argv[0]),
                        QSL("--data"),
                        directory.path(),
                        QSL("--no-single-instance"),
                        QSL("--force-text-browser")};
  Application application(QSL("rssguard-feed-deletion-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  application.setFeedReader(new FeedReader(&application));
  TestFeedDeletion tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_feeddeletion.moc"
