// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/messagesproxymodel.h"
#include "database/databasequeries.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/messagesview.h"
#include "gui/tabwidget.h"
#include "gui/toolbars/feedstoolbar.h"
#include "gui/toolbars/messagestoolbar.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "services/abstract/category.h"

#include <memory>

#include <QActionGroup>
#include <QDir>
#include <QMetaMethod>
#include <QProcess>
#include <QSet>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace {
  using Mode = FilteringProxyModel::FilterMatchMode;
  QString profilePath;

  class TestServiceRoot : public ServiceRoot {
    public:
      QString code() const override {
        return QSL("filter-match-mode-test");
      }

      QList<Message> obtainNewMessages(Feed*,
                                       const QHash<BagOfMessages, QStringList>&,
                                       const QHash<QString, QStringList>&) override {
        return {};
      }
  };

  QMetaMethod filterSignal(QObject* toolbar) {
    const auto* meta = toolbar->metaObject();
    for (int i = 0; i < meta->methodCount(); ++i) {
      const auto method = meta->method(i);
      if (method.methodType() == QMetaMethod::Signal &&
          (method.name() == "feedFilterChanged" || method.name() == "messageFilterChanged")) {
        return method;
      }
    }
    return {};
  }

  QToolButton* filterButton(QObject* toolbar) {
    for (auto* button : toolbar->findChildren<QToolButton*>()) {
      if (button->menu() != nullptr && button->menu()->findChild<QAction*>(QSL("filter_match_any")) != nullptr) {
        return button;
      }
    }
    return nullptr;
  }

  void prepareProfile(const QString& profile) {
    QDir().mkpath(profile + QDir::separator() + QSL(APP_CFG_PATH));
    QSettings settings(profile + QDir::separator() + QSL(APP_CFG_PATH) + QDir::separator() + QSL(APP_CFG_FILE),
                       QSettings::IniFormat);
    settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
    settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
    settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
    settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::FeedsUpdateOnStartup, false);
    settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::AutoUpdateEnabled, false);
    settings.setValue(GROUP(Messages) + QSL("/") + Messages::ArticleMarkOnSelection,
                      int(MessagesView::ArticleMarkingPolicy::MarkOnlyManually));
    settings.sync();
  }
} // namespace

class TestFilterMatchMode : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();
    void defaultAndInvalidModes();
    void feedsMatchAnyAndAll();
    void feedTextSearchIntersectsWithBothModes();
    void foldersAndSelectedFeedRemainVisible();
    void articlesRefreshWhenOnlyModeChanges();
    void textSearchIntersectsWithBothModes();
    void selectedArticleRetainsItsException();
    void modeControlsAreSeparateFromCriteria_data();
    void modeControlsAreSeparateFromCriteria();
    void independentModesAndCriteriaSurviveRestart();
    void restoreWithoutCriteriaAppliesMode();

  private:
    FeedMessageViewer* viewer() const;
    bool visible(const RootItem* item) const;
    void trigger(QObject* toolbar, const QString& name);
    void selectArticles();
    void clearArticleSelection();
    QSet<QString> articleTitles() const;
    void verifyRestart(Mode feeds,
                       Mode articles,
                       const QStringList& feed_criteria = {},
                       const QStringList& article_criteria = {});

    std::unique_ptr<FormMain> m_window;
    TestServiceRoot* m_account = nullptr;
    Category* m_folder = nullptr;
    Feed* m_enabledUnread = nullptr;
    Feed* m_disabledUnread = nullptr;
    Feed* m_enabledRead = nullptr;
};

void TestFilterMatchMode::initTestCase() {
  qRegisterMetaType<Mode>();
  qRegisterMetaType<FeedsProxyModel::FeedListFilter>();
  qRegisterMetaType<MessagesProxyModel::MessageListFilter>();
  m_account = new TestServiceRoot();
  m_account->setTitle(QSL("Filter mode account"));
  m_folder = new Category(m_account);
  m_folder->setTitle(QSL("Parent folder"));
  m_account->appendChild(m_folder);
  auto make_feed = [this](const QString& title, RootItem* parent) {
    auto* feed = new Feed(parent);
    feed->setTitle(title);
    feed->setCustomId(title);
    parent->appendChild(feed);
    return feed;
  };
  m_enabledUnread = make_feed(QSL("Enabled unread"), m_folder);
  m_disabledUnread = make_feed(QSL("Disabled unread"), m_folder);
  m_disabledUnread->setIsSwitchedOff(true);
  m_enabledRead = make_feed(QSL("Enabled read"), m_account);
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::createOverwriteAccount(db, m_account);
    DatabaseQueries::createOverwriteCategory(db, m_folder, m_account->accountId(), NO_PARENT_CATEGORY);
    for (Feed* feed : {m_enabledUnread, m_disabledUnread, m_enabledRead}) {
      DatabaseQueries::createOverwriteFeed(db,
                                           feed,
                                           m_account->accountId(),
                                           feed->parent() == m_account ? NO_PARENT_CATEGORY : m_folder->id());
    }
  });
  QList<Message> articles;
  for (int i = 0; i < 4; ++i) {
    Message article;
    article.m_title = QStringList{QSL("Both"), QSL("Unread"), QSL("Important"), QSL("Neither")}.at(i);
    article.m_customId = article.m_title;
    article.m_contents = QSL("Filter mode article");
    article.m_accountId = m_account->accountId();
    article.m_feedId = m_enabledUnread->id();
    article.m_isRead = i >= 2;
    article.m_isImportant = i == 0 || i == 2;
    article.m_created = QDateTime::currentDateTimeUtc().addSecs(-i);
    article.m_retrieved = article.m_created;
    articles.append(article);
  }
  DatabaseQueries::updateMessages(qApp->database(), qApp->settings(), articles, m_enabledUnread, false, true)
    .throwIfFailed();
  qApp->feedReader()->feedsModel()->rootItem()->appendChild(m_account);
  qApp->feedReader()->feedsModel()->reloadWholeLayout();
  m_window = std::make_unique<FormMain>();
  QCOMPARE(qApp->feedReader()->feedsProxyModel()->filterMatchMode(), Mode::Any);
  QCOMPARE(qApp->feedReader()->messagesProxyModel()->filterMatchMode(), Mode::Any);
}

void TestFilterMatchMode::init() {
  qApp->settings()->remove(GROUP(GUI), GUI::FeedsFilterMatchMode);
  qApp->settings()->remove(GROUP(GUI), GUI::MessagesFilterMatchMode);
  m_enabledUnread->setCountOfUnreadMessages(2);
  m_disabledUnread->setCountOfUnreadMessages(1);
  m_enabledRead->setCountOfUnreadMessages(0);
  for (BaseBar* bar :
       {static_cast<BaseBar*>(viewer()->feedsToolBar()), static_cast<BaseBar*>(viewer()->messagesToolBar())}) {
    auto* toolbar = dynamic_cast<QObject*>(bar);
    const bool feeds = toolbar == viewer()->feedsToolBar();
    trigger(toolbar, feeds ? QSL("feedlist_no_filtering") : QSL("articlelist_no_filtering"));
    trigger(toolbar, QSL("filter_match_any"));
    qApp->settings()->setValue(GROUP(GUI),
                               feeds ? GUI::FeedsToolbarActions : GUI::MessagesToolbarDefaultButtons,
                               feeds ? GUI::FeedsToolbarActionsDef : GUI::MessagesToolbarDefaultButtonsDef);
    bar->loadSavedActions();
    auto* search = toolbar->findChild<QLineEdit*>();
    search->clear();
    QTest::keyClick(search, Qt::Key_Return);
  }
  const auto account_index =
    qApp->feedReader()->feedsProxyModel()->mapFromSource(qApp->feedReader()->feedsModel()->indexForItem(m_account));
  viewer()->feedsView()->selectionModel()->setCurrentIndex(account_index, QItemSelectionModel::ClearAndSelect);
  viewer()->feedsView()->selectionModel()->clear();
  clearArticleSelection();
}

void TestFilterMatchMode::cleanup() {
  viewer()->feedsView()->selectionModel()->clear();
  clearArticleSelection();
}

void TestFilterMatchMode::cleanupTestCase() {
  m_window.reset();
}

FeedMessageViewer* TestFilterMatchMode::viewer() const {
  return m_window->tabWidget()->feedMessageViewer();
}

bool TestFilterMatchMode::visible(const RootItem* item) const {
  return qApp->feedReader()
    ->feedsProxyModel()
    ->mapFromSource(qApp->feedReader()->feedsModel()->indexForItem(item))
    .isValid();
}

void TestFilterMatchMode::trigger(QObject* toolbar, const QString& name) {
  auto* action = toolbar->findChild<QAction*>(name);
  QVERIFY2(action != nullptr, qPrintable(name));
  action->trigger();
}

void TestFilterMatchMode::clearArticleSelection() {
  viewer()->messagesView()->selectionModel()->clear();
}

void TestFilterMatchMode::selectArticles() {
  auto* proxy = qApp->feedReader()->feedsProxyModel();
  const auto index = proxy->mapFromSource(qApp->feedReader()->feedsModel()->indexForItem(m_enabledUnread));
  QVERIFY(index.isValid());
  viewer()->feedsView()->selectionModel()->setCurrentIndex(index,
                                                           QItemSelectionModel::ClearAndSelect |
                                                             QItemSelectionModel::Rows);
  QTRY_COMPARE(viewer()->messagesView()->model()->rowCount(), 4);
  clearArticleSelection();
}

QSet<QString> TestFilterMatchMode::articleTitles() const {
  QSet<QString> titles;
  auto* model = viewer()->messagesView()->model();
  for (int row = 0; row < model->rowCount(); ++row) {
    titles.insert(model->index(row, MSG_MDL_TITLE_INDEX).data(Qt::EditRole).toString());
  }
  return titles;
}

void TestFilterMatchMode::verifyRestart(Mode feeds,
                                        Mode articles,
                                        const QStringList& feed_criteria,
                                        const QStringList& article_criteria) {
  qApp->settings()->sync();
  QProcess process;
  process.start(QCoreApplication::applicationFilePath(),
                {QSL("--verify-filter-modes"),
                 profilePath,
                 QString::number(int(feeds)),
                 QString::number(int(articles)),
                 feed_criteria.join(QL1C(';')),
                 article_criteria.join(QL1C(';'))});
  QVERIFY(process.waitForStarted());
  QVERIFY(process.waitForFinished(60000));
  const QByteArray output = process.readAllStandardOutput() + process.readAllStandardError();
  QVERIFY2(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0, output.constData());
}

void TestFilterMatchMode::defaultAndInvalidModes() {
  for (QObject* toolbar :
       {static_cast<QObject*>(viewer()->feedsToolBar()), static_cast<QObject*>(viewer()->messagesToolBar())}) {
    QVERIFY(toolbar->findChild<QAction*>(QSL("filter_match_any"))->isChecked());
    QVERIFY(!toolbar->findChild<QAction*>(QSL("filter_match_all"))->isChecked());
  }
  QCOMPARE(qApp->feedReader()->feedsProxyModel()->filterMatchMode(), Mode::Any);
  QCOMPARE(qApp->feedReader()->messagesProxyModel()->filterMatchMode(), Mode::Any);
  qApp->settings()->setValue(GROUP(GUI), GUI::FeedsFilterMatchMode, 99);
  qApp->settings()->setValue(GROUP(GUI), GUI::MessagesFilterMatchMode, -1);
  verifyRestart(Mode::Any, Mode::Any);
}

void TestFilterMatchMode::feedsMatchAnyAndAll() {
  auto* toolbar = viewer()->feedsToolBar();
  trigger(toolbar, QSL("feedlist_switched_on"));
  QVERIFY(visible(m_enabledUnread));
  QVERIFY(visible(m_enabledRead));
  QVERIFY(!visible(m_disabledUnread));
  trigger(toolbar, QSL("feedlist_show_unread"));
  QVERIFY(visible(m_disabledUnread));
  trigger(toolbar, QSL("filter_match_all"));
  QVERIFY(visible(m_enabledUnread));
  QVERIFY(!visible(m_enabledRead));
  QVERIFY(!visible(m_disabledUnread));
  QVERIFY(visible(m_folder));
  trigger(toolbar, QSL("feedlist_switched_on"));
  QVERIFY(visible(m_disabledUnread));
  QVERIFY(!visible(m_enabledRead));
  trigger(toolbar, QSL("feedlist_no_filtering"));
  QVERIFY(visible(m_enabledUnread));
  QVERIFY(visible(m_enabledRead));
  QVERIFY(visible(m_disabledUnread));
  QCOMPARE(qApp->feedReader()->feedsProxyModel()->filterMatchMode(), Mode::All);
}

void TestFilterMatchMode::foldersAndSelectedFeedRemainVisible() {
  auto* toolbar = viewer()->feedsToolBar();
  const auto selected = qApp->feedReader()
                          ->feedsProxyModel()
                          ->mapFromSource(qApp->feedReader()->feedsModel()->indexForItem(m_disabledUnread));
  QVERIFY(selected.isValid());
  viewer()->feedsView()->selectionModel()->setCurrentIndex(selected,
                                                           QItemSelectionModel::ClearAndSelect |
                                                             QItemSelectionModel::Rows);
  QCOMPARE(viewer()->feedsView()->selectionModel()->selectedRows().size(), 1);
  trigger(toolbar, QSL("feedlist_show_feeds"));
  trigger(toolbar, QSL("feedlist_show_folders"));
  QVERIFY(visible(m_enabledUnread));
  trigger(toolbar, QSL("filter_match_all"));
  QVERIFY(!visible(m_enabledUnread));
  QVERIFY(!visible(m_enabledRead));
  QVERIFY(visible(m_disabledUnread));
  QVERIFY(visible(m_folder));
  QVERIFY(visible(m_account));
  viewer()->feedsView()->selectionModel()->clear();
  QVERIFY(!visible(m_disabledUnread));
  trigger(toolbar, QSL("filter_match_any"));
  QVERIFY(visible(m_enabledUnread));
}

void TestFilterMatchMode::feedTextSearchIntersectsWithBothModes() {
  auto* toolbar = viewer()->feedsToolBar();
  trigger(toolbar, QSL("feedlist_switched_on"));
  trigger(toolbar, QSL("feedlist_show_unread"));
  auto* search = toolbar->findChild<QLineEdit*>();
  QVERIFY(search != nullptr);
  search->setText(QSL("Enabled read"));
  QTest::keyClick(search, Qt::Key_Return);
  QTRY_VERIFY(visible(m_enabledRead));
  QTRY_VERIFY(!visible(m_enabledUnread));
  QVERIFY(!visible(m_disabledUnread));
  trigger(toolbar, QSL("filter_match_all"));
  QVERIFY(!visible(m_enabledRead));
  trigger(toolbar, QSL("feedlist_no_filtering"));
  QVERIFY(visible(m_enabledRead));
  QVERIFY(!visible(m_enabledUnread));
}

void TestFilterMatchMode::articlesRefreshWhenOnlyModeChanges() {
  selectArticles();
  auto* toolbar = viewer()->messagesToolBar();
  trigger(toolbar, QSL("articlelist_show_unread"));
  clearArticleSelection();
  trigger(toolbar, QSL("articlelist_show_important"));
  QCOMPARE(articleTitles(), (QSet<QString>{QSL("Both"), QSL("Unread"), QSL("Important")}));
  clearArticleSelection();
  trigger(toolbar, QSL("filter_match_all"));
  QCOMPARE(articleTitles(), (QSet<QString>{QSL("Both")}));
  clearArticleSelection();
  trigger(toolbar, QSL("articlelist_show_important"));
  QCOMPARE(articleTitles(), (QSet<QString>{QSL("Both"), QSL("Unread")}));
  clearArticleSelection();
  trigger(toolbar, QSL("articlelist_show_read"));
  QVERIFY(articleTitles().isEmpty());
  trigger(toolbar, QSL("filter_match_any"));
  QCOMPARE(articleTitles().size(), 4);
}

void TestFilterMatchMode::textSearchIntersectsWithBothModes() {
  selectArticles();
  auto* toolbar = viewer()->messagesToolBar();
  trigger(toolbar, QSL("articlelist_show_unread"));
  clearArticleSelection();
  trigger(toolbar, QSL("articlelist_show_important"));
  clearArticleSelection();
  auto* search = static_cast<QLineEdit*>(toolbar->findChild<QLineEdit*>());
  QVERIFY(search != nullptr);
  search->setText(QSL("Important"));
  QTest::keyClick(search, Qt::Key_Return);
  QTRY_COMPARE(articleTitles(), (QSet<QString>{QSL("Important")}));
  clearArticleSelection();
  trigger(toolbar, QSL("filter_match_all"));
  QVERIFY(articleTitles().isEmpty());
  trigger(toolbar, QSL("articlelist_no_filtering"));
  QCOMPARE(articleTitles(), (QSet<QString>{QSL("Important")}));
}

void TestFilterMatchMode::selectedArticleRetainsItsException() {
  selectArticles();
  auto* view = viewer()->messagesView();
  for (int row = 0; row < view->model()->rowCount(); ++row) {
    const auto index = view->model()->index(row, MSG_MDL_TITLE_INDEX);
    if (index.data(Qt::EditRole).toString() == QSL("Neither")) {
      view->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
      break;
    }
  }
  auto* toolbar = viewer()->messagesToolBar();
  trigger(toolbar, QSL("articlelist_show_unread"));
  trigger(toolbar, QSL("articlelist_show_important"));
  trigger(toolbar, QSL("filter_match_all"));
  QCOMPARE(articleTitles(), (QSet<QString>{QSL("Both"), QSL("Neither")}));
}

void TestFilterMatchMode::modeControlsAreSeparateFromCriteria_data() {
  QTest::addColumn<bool>("feeds");
  QTest::newRow("feeds") << true;
  QTest::newRow("articles") << false;
}

void TestFilterMatchMode::modeControlsAreSeparateFromCriteria() {
  QFETCH(bool, feeds);
  QObject* toolbar =
    feeds ? static_cast<QObject*>(viewer()->feedsToolBar()) : static_cast<QObject*>(viewer()->messagesToolBar());
  BaseBar* bar =
    feeds ? static_cast<BaseBar*>(viewer()->feedsToolBar()) : static_cast<BaseBar*>(viewer()->messagesToolBar());
  const auto key = feeds ? GUI::FeedsToolbarActions : GUI::MessagesToolbarDefaultButtons;
  const auto first = feeds ? QSL("feedlist_switched_on") : QSL("articlelist_show_unread");
  const auto second = feeds ? QSL("feedlist_show_unread") : QSL("articlelist_show_important");
  trigger(toolbar, first);
  trigger(toolbar, second);
  QSignalSpy changes(toolbar, filterSignal(toolbar));
  QVERIFY(changes.isValid());
  auto* button = filterButton(toolbar);
  QVERIFY(button != nullptr);
  const auto icon = button->icon().pixmap(32, 32).toImage();
  const auto saved = qApp->settings()->value(GROUP(GUI), key).toString();
  trigger(toolbar, QSL("filter_match_all"));
  QCOMPARE(changes.count(), 1);
  QCOMPARE(changes.first().at(1).value<Mode>(), Mode::All);
  QCOMPARE(qApp->settings()->value(GROUP(GUI), key).toString(), saved);
  QCOMPARE(button->icon().pixmap(32, 32).toImage(), icon);
  QVERIFY(button->toolTip().startsWith(toolbar->findChild<QAction*>(QSL("filter_match_all"))->text()));
  for (auto* action : bar->availableActions()) {
    QVERIFY(!action->objectName().startsWith(QSL("filter_match_")));
  }
  trigger(toolbar, feeds ? QSL("feedlist_no_filtering") : QSL("articlelist_no_filtering"));
  QVERIFY(toolbar->findChild<QAction*>(QSL("filter_match_all"))->isChecked());
  QVERIFY(!toolbar->findChild<QAction*>(first)->isChecked());
  QVERIFY(!toolbar->findChild<QAction*>(second)->isChecked());
  QVERIFY(qApp->settings()->value(GROUP(GUI), key).toString().contains(QSL("filter[]")));
  QVERIFY(!qApp->settings()->value(GROUP(GUI), key).toString().contains(QSL("filter_match_")));
}

void TestFilterMatchMode::independentModesAndCriteriaSurviveRestart() {
  trigger(viewer()->feedsToolBar(), QSL("feedlist_switched_on"));
  trigger(viewer()->feedsToolBar(), QSL("feedlist_show_unread"));
  trigger(viewer()->feedsToolBar(), QSL("filter_match_all"));
  trigger(viewer()->messagesToolBar(), QSL("articlelist_show_unread"));
  const auto saved_feeds = qApp->settings()->value(GROUP(GUI), GUI::FeedsToolbarActions).toString();
  verifyRestart(Mode::All,
                Mode::Any,
                {QSL("feedlist_switched_on"), QSL("feedlist_show_unread")},
                {QSL("articlelist_show_unread")});
  QCOMPARE(qApp->feedReader()->feedsProxyModel()->filterMatchMode(), Mode::All);
  QCOMPARE(qApp->feedReader()->messagesProxyModel()->filterMatchMode(), Mode::Any);
  QVERIFY(viewer()->feedsToolBar()->findChild<QAction*>(QSL("feedlist_switched_on"))->isChecked());
  QVERIFY(viewer()->feedsToolBar()->findChild<QAction*>(QSL("feedlist_show_unread"))->isChecked());
  QVERIFY(viewer()->messagesToolBar()->findChild<QAction*>(QSL("articlelist_show_unread"))->isChecked());
  QCOMPARE(qApp->settings()->value(GROUP(GUI), GUI::FeedsToolbarActions).toString(), saved_feeds);
  QSignalSpy restored(viewer()->feedsToolBar(), filterSignal(viewer()->feedsToolBar()));
  static_cast<BaseBar*>(viewer()->feedsToolBar())->loadSavedActions();
  QCOMPARE(restored.count(), 1);
  QCOMPARE(restored.first().at(1).value<Mode>(), Mode::All);
  trigger(viewer()->messagesToolBar(), QSL("filter_match_all"));
  trigger(viewer()->feedsToolBar(), QSL("filter_match_any"));
  qApp->settings()->sync();
  QSettings stored(qApp->settings()->fileName(), QSettings::IniFormat);
  QCOMPARE(stored.value(GROUP(GUI) + QSL("/") + GUI::FeedsFilterMatchMode).toInt(), int(Mode::Any));
  QCOMPARE(stored.value(GROUP(GUI) + QSL("/") + GUI::MessagesFilterMatchMode).toInt(), int(Mode::All));
  verifyRestart(Mode::Any,
                Mode::All,
                {QSL("feedlist_switched_on"), QSL("feedlist_show_unread")},
                {QSL("articlelist_show_unread")});
  QCOMPARE(qApp->feedReader()->feedsProxyModel()->filterMatchMode(), Mode::Any);
  QCOMPARE(qApp->feedReader()->messagesProxyModel()->filterMatchMode(), Mode::All);
}

void TestFilterMatchMode::restoreWithoutCriteriaAppliesMode() {
  for (BaseBar* bar :
       {static_cast<BaseBar*>(viewer()->feedsToolBar()), static_cast<BaseBar*>(viewer()->messagesToolBar())}) {
    auto* toolbar = dynamic_cast<QObject*>(bar);
    trigger(toolbar, QSL("filter_match_all"));
    QSignalSpy changes(toolbar, filterSignal(toolbar));
    bar->loadSavedActions();
    QCOMPARE(changes.count(), 1);
    QCOMPARE(changes.first().at(0).toInt(), 1);
    QCOMPARE(changes.first().at(1).value<Mode>(), Mode::All);
  }
  verifyRestart(Mode::All, Mode::All);
  QCOMPARE(qApp->feedReader()->feedsProxyModel()->filterMatchMode(), Mode::All);
  QCOMPARE(qApp->feedReader()->messagesProxyModel()->filterMatchMode(), Mode::All);
  QVERIFY(visible(m_enabledUnread));
  QVERIFY(visible(m_disabledUnread));
}

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  const bool verify_modes = argc == 7 && QString::fromLocal8Bit(argv[1]) == QSL("--verify-filter-modes");
  profilePath = verify_modes ? QString::fromLocal8Bit(argv[2]) : directory.path();
  if (!verify_modes) {
    prepareProfile(profilePath);
  }
  QStringList arguments{QString::fromLocal8Bit(argv[0]),
                        QSL("--data"),
                        profilePath,
                        QSL("--no-single-instance"),
                        QSL("--force-text-browser")};
  Application application(QSL("rssguard-filter-match-mode-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  application.setFeedReader(new FeedReader(&application));
  if (verify_modes) {
    FormMain window;
    auto* viewer = window.tabWidget()->feedMessageViewer();
    auto verify = [](BaseBar* bar, Mode actual, const QString& expected, const QString& criteria) {
      const auto mode = static_cast<Mode>(expected.toInt());
      auto* toolbar = dynamic_cast<QObject*>(bar);
      auto* action =
        toolbar->findChild<QAction*>(mode == Mode::All ? QSL("filter_match_all") : QSL("filter_match_any"));
      if (actual != mode || action == nullptr || !action->isChecked()) {
        qWarning() << "Restored filter mode differs from expected mode" << expected;
        return false;
      }
      const auto names = criteria.split(QL1C(';'), SPLIT_BEHAVIOR::SkipEmptyParts);
      for (const auto& name : names) {
        auto* criterion = toolbar->findChild<QAction*>(name);
        if (criterion == nullptr || !criterion->isChecked()) {
          qWarning() << "Criterion was not restored" << name;
          return false;
        }
      }
      for (auto* criterion : bar->extraActions()) {
        const auto name = criterion->objectName();
        if ((name.startsWith(QSL("feedlist_")) || name.startsWith(QSL("articlelist_show_"))) &&
            criterion->isChecked() != names.contains(name)) {
          qWarning() << "Unexpected restored criterion" << name;
          return false;
        }
      }
      return true;
    };
    return verify(viewer->feedsToolBar(),
                  application.feedReader()->feedsProxyModel()->filterMatchMode(),
                  QString::fromLocal8Bit(argv[3]),
                  QString::fromLocal8Bit(argv[5])) &&
               verify(viewer->messagesToolBar(),
                      application.feedReader()->messagesProxyModel()->filterMatchMode(),
                      QString::fromLocal8Bit(argv[4]),
                      QString::fromLocal8Bit(argv[6]))
             ? 0
             : 2;
  }
  TestFilterMatchMode tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_filtermatchmode.moc"
