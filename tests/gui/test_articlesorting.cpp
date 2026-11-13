// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feedsmodel.h"
#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "database/databasequeries.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/messagesview.h"
#include "gui/tabwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "services/abstract/category.h"

#include <memory>

#include <QCheckBox>
#include <QClipboard>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QProcess>
#include <QSettings>
#include <QSignalBlocker>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

namespace {
  using SortStates = BaseTreeView::ColumnSortStates;

  class TestServiceRoot : public ServiceRoot {
    public:
      QString code() const override {
        return m_code;
      }

      QList<ArticleColumn> articleColumns() const override {
        return m_columns;
      }

      QVariant articleColumnData(const Message& message, const QString& id, int role) const override {
        if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) {
          return QSL("extra-%1-%2").arg(id, message.m_title);
        }
        return {};
      }

      QString m_code = QSL("article-sorting-test");
      QList<ArticleColumn> m_columns;

      QList<Message> obtainNewMessages(Feed*,
                                       const QHash<BagOfMessages, QStringList>&,
                                       const QHash<QString, QStringList>&) override {
        return {};
      }
  };

  const SortStates defaultSort{{MSG_MDL_DCREATED_INDEX, Qt::DescendingOrder},
                               {MSG_MDL_TITLE_INDEX, Qt::AscendingOrder}};

  QJsonObject headerState(BaseTreeView* view) {
    return QJsonDocument::fromJson(view->saveHeaderState()).object();
  }

  void setSortStates(QJsonObject& state, const SortStates& sorts) {
    state[QSL("sort_count")] = sorts.size();
    for (int i = 0; i < sorts.size(); ++i) {
      state[QSL("sort_%1_column").arg(i)] = sorts.at(i).first;
      state[QSL("sort_%1_order").arg(i)] = sorts.at(i).second;
    }
  }

  SortStates sortStates(const QJsonObject& state) {
    SortStates sorts;
    for (int i = 0; i < state[QSL("sort_count")].toInt(); ++i) {
      sorts.append({state[QSL("sort_%1_column").arg(i)].toInt(),
                    Qt::SortOrder(state[QSL("sort_%1_order").arg(i)].toInt())});
    }
    return sorts;
  }

  void saveProfile(const QString& key, const QJsonObject& state) {
    qApp->settings()->setValue(GROUP(GUI), key, QString(QJsonDocument(state).toJson().toBase64()));
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
    settings.sync();
  }
} // namespace

class TestArticleSorting : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void completeSortingSurvivesRestore_data();
    void completeSortingSurvivesRestore();
    void invalidSortingFallsBack_data();
    void invalidSortingFallsBack();
    void sortingValidationIsIndependentOfLayout();
    void feedSelectionUsesModelSorting();
    void secondaryCtrlSortingKeepsPrimaryIndicator();
    void hiddenSortingSurvivesVisibilityAndProfiles();
    void profileToggleRefreshesArticles_data();
    void profileToggleRefreshesArticles();
    void nestedHeaderSignalBlockingIsPreserved();
    void sortingSurvivesRestartWithoutSelectingFeed();
    void pluginColumnsUseExistingLayouts_data();
    void pluginColumnsUseExistingLayouts();
    void pluginColumnIdentitiesSurviveSchemaChanges();
    void pluginColumnsCannotChangeSorting();
    void pluginColumnRolesAndSearch();
    void pluginColumnDescriptorsAreValidated();
    void legacyLayoutRestoresIntoExpandedSchema();

  private:
    FeedMessageViewer* viewer() const;
    MessagesView* view() const;
    void loadItem(RootItem* item);
    QStringList articles() const;
    bool toggleProfiles(bool settings_dialog);

    std::unique_ptr<FormMain> m_window;
    TestServiceRoot* m_account = nullptr;
    Category* m_folder = nullptr;
    Feed* m_first = nullptr;
    Feed* m_second = nullptr;
    TestServiceRoot* m_otherAccount = nullptr;
    Feed* m_otherFeed = nullptr;
};

void TestArticleSorting::initTestCase() {
  m_account = new TestServiceRoot();
  m_account->setTitle(QSL("Sorting account"));
  m_folder = new Category(m_account);
  m_folder->setTitle(QSL("Sorting folder"));
  m_account->appendChild(m_folder);
  m_first = new Feed(m_folder);
  m_first->setTitle(QSL("First feed"));
  m_first->setCustomId(QSL("first"));
  m_folder->appendChild(m_first);
  m_second = new Feed(m_account);
  m_second->setTitle(QSL("Second feed"));
  m_second->setCustomId(QSL("second"));
  m_account->appendChild(m_second);
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::createOverwriteAccount(db, m_account);
    DatabaseQueries::createOverwriteCategory(db, m_folder, m_account->accountId(), NO_PARENT_CATEGORY);
    DatabaseQueries::createOverwriteFeed(db, m_first, m_account->accountId(), m_folder->id());
    DatabaseQueries::createOverwriteFeed(db, m_second, m_account->accountId(), NO_PARENT_CATEGORY);
  });
  for (auto* feed : {m_first, m_second}) {
    QList<Message> articles;
    for (int i = 0; i < 4; ++i) {
      Message article;
      article.m_title = QStringList{QSL("Zulu"), QSL("Alpha"), QSL("Alpha"), QSL("Beta")}.at(i);
      article.m_author = QStringList{QSL("A"), QSL("Z"), QSL("A"), QSL("B")}.at(i);
      article.m_customId = QString::number(i);
      article.m_accountId = m_account->accountId();
      article.m_feedId = feed->id();
      article.m_created = QDateTime::fromMSecsSinceEpoch(1700000000000).addSecs(i == 3 ? -1 : 0);
      article.m_retrieved = article.m_created;
      articles.append(article);
    }
    DatabaseQueries::updateMessages(qApp->database(), qApp->settings(), articles, feed, false, true).throwIfFailed();
  }
  qApp->feedReader()->feedsModel()->rootItem()->appendChild(m_account);
  m_otherAccount = new TestServiceRoot();
  m_otherAccount->m_code = QSL("other-sorting-provider");
  m_otherAccount->setTitle(QSL("Other provider"));
  m_otherFeed = new Feed(m_otherAccount);
  m_otherFeed->setTitle(QSL("Other provider feed"));
  m_otherFeed->setCustomId(QSL("other-provider-feed"));
  m_otherAccount->appendChild(m_otherFeed);
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::createOverwriteAccount(db, m_otherAccount);
    DatabaseQueries::createOverwriteFeed(db, m_otherFeed, m_otherAccount->accountId(), NO_PARENT_CATEGORY);
  });
  qApp->feedReader()->feedsModel()->rootItem()->appendChild(m_otherAccount);
  qApp->feedReader()->feedsModel()->reloadWholeLayout();
  m_window = std::make_unique<FormMain>();
}

void TestArticleSorting::init() {
  QTest::keyRelease(view(), Qt::Key_Control);
  loadItem(nullptr);
  m_account->m_columns.clear();
  qApp->settings()->setValue(GROUP(Messages), Messages::ArticleListColumnProfiles, false);
  for (const auto& key : {GUI::MessageViewState,
                          GUI::MessageViewStateFeed,
                          GUI::MessageViewStateCategory,
                          GUI::MessageViewStateAccount}) {
    qApp->settings()->remove(GROUP(GUI), key);
    qApp->settings()->remove(GROUP(GUI), key + QSL("_provider_article-sorting-test"));
  }
  viewer()->loadSize();
}

void TestArticleSorting::cleanupTestCase() {
  m_window.reset();
}

FeedMessageViewer* TestArticleSorting::viewer() const {
  return m_window->tabWidget()->feedMessageViewer();
}

MessagesView* TestArticleSorting::view() const {
  return viewer()->messagesView();
}

void TestArticleSorting::loadItem(RootItem* item) {
  QVERIFY(QMetaObject::invokeMethod(view(), "loadItem", Qt::DirectConnection, Q_ARG(RootItem*, item)));
}

QStringList TestArticleSorting::articles() const {
  const QAbstractItemModel* model = view()->model();
  QStringList articles;
  for (int row = 0; row < model->rowCount(); ++row) {
    articles.append(model->data(model->index(row, MSG_MDL_TITLE_INDEX), Qt::EditRole).toString() + QSL("|") +
                    model->data(model->index(row, MSG_MDL_AUTHOR_INDEX), Qt::EditRole).toString());
  }
  return articles;
}

void TestArticleSorting::completeSortingSurvivesRestore_data() {
  QTest::addColumn<SortStates>("sorts");
  QTest::newRow("single-key") << SortStates{{MSG_MDL_TITLE_INDEX, Qt::AscendingOrder}};
  QTest::newRow("three-keys") << SortStates{{MSG_MDL_DCREATED_INDEX, Qt::DescendingOrder},
                                            {MSG_MDL_TITLE_INDEX, Qt::AscendingOrder},
                                            {MSG_MDL_AUTHOR_INDEX, Qt::DescendingOrder}};
  QTest::newRow("hidden-id") << SortStates{{MSG_MDL_ID_INDEX, Qt::DescendingOrder}};
}

void TestArticleSorting::completeSortingSurvivesRestore() {
  QFETCH(SortStates, sorts);
  auto state = headerState(view());
  setSortStates(state, sorts);
  saveProfile(GUI::MessageViewState, state);
  {
    const QSignalBlocker blocker(view()->header());
    view()->header()->setSortIndicator(sorts.first().first, sorts.first().second);
  }
  QSignalSpy header_changes(view()->header(), &QHeaderView::sortIndicatorChanged);
  // Restoring the same indicator must still restore every SQL key, without a header signal.
  QTest::keyPress(view(), Qt::Key_Control, Qt::ControlModifier);
  QVERIFY(QApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier));
  viewer()->loadSize();
  QTest::keyRelease(view(), Qt::Key_Control);
  QCOMPARE(sortStates(headerState(view())), sorts);
  QCOMPARE(header_changes.count(), 0);
  loadItem(m_first);
  QCOMPARE(sortStates(headerState(view())), sorts);
  if (sorts.size() == 3) {
    QCOMPARE(articles(), QStringList({QSL("Alpha|Z"), QSL("Alpha|A"), QSL("Zulu|A"), QSL("Beta|B")}));
  }
  viewer()->saveSize();
  viewer()->loadSize();
  QCOMPARE(sortStates(headerState(view())), sorts);
}

void TestArticleSorting::invalidSortingFallsBack_data() {
  QTest::addColumn<QByteArray>("state");
  QTest::newRow("malformed-json") << QByteArray("{");
  QTest::newRow("empty-object") << QByteArray("{}");
  QTest::newRow("empty-sorts") << QByteArray("{\"header_count\":19,\"sort_count\":0}");
  QTest::newRow("missing-column") << QByteArray("{\"header_count\":19,\"sort_count\":1,\"sort_0_order\":0}");
  QTest::newRow("missing-order") << QByteArray("{\"header_count\":19,\"sort_count\":1,\"sort_0_column\":6}");
  QTest::newRow("invalid-column")
    << QByteArray("{\"header_count\":19,\"sort_count\":1,\"sort_0_column\":99,\"sort_0_order\":0}");
  QTest::newRow("invalid-order")
    << QByteArray("{\"header_count\":19,\"sort_count\":1,\"sort_0_column\":6,\"sort_0_order\":7}");
}

void TestArticleSorting::invalidSortingFallsBack() {
  QFETCH(QByteArray, state);
  qApp->settings()->setValue(GROUP(GUI), GUI::MessageViewState, QString(state.toBase64()));
  viewer()->loadSize();
  QCOMPARE(sortStates(headerState(view())), defaultSort);
  QCOMPARE(view()->header()->sortIndicatorSection(), MSG_MDL_DCREATED_INDEX);
  QCOMPARE(view()->header()->sortIndicatorOrder(), Qt::DescendingOrder);
}

void TestArticleSorting::sortingValidationIsIndependentOfLayout() {
  auto state = headerState(view());
  const SortStates expected{{MSG_MDL_TITLE_INDEX, Qt::DescendingOrder},
                            {MSG_MDL_ID_INDEX, Qt::AscendingOrder},
                            {MSG_MDL_DCREATED_INDEX, Qt::DescendingOrder}};
  setSortStates(state,
                {{99, Qt::AscendingOrder},
                 {MSG_MDL_TITLE_INDEX, Qt::DescendingOrder},
                 {MSG_MDL_TITLE_INDEX, Qt::AscendingOrder},
                 {MSG_MDL_AUTHOR_INDEX, Qt::SortOrder(7)},
                 {MSG_MDL_ID_INDEX, Qt::AscendingOrder},
                 {MSG_MDL_DCREATED_INDEX, Qt::DescendingOrder},
                 {MSG_MDL_DRETRIEVED_INDEX, Qt::AscendingOrder}});
  state[QSL("header_count")] = 1;
  saveProfile(GUI::MessageViewState, state);
  viewer()->loadSize();
  QCOMPARE(sortStates(headerState(view())), expected);
  QVERIFY(view()->isColumnHidden(MSG_MDL_ID_INDEX));
}

void TestArticleSorting::feedSelectionUsesModelSorting() {
  const SortStates sorts{{MSG_MDL_DCREATED_INDEX, Qt::DescendingOrder},
                         {MSG_MDL_TITLE_INDEX, Qt::AscendingOrder},
                         {MSG_MDL_AUTHOR_INDEX, Qt::DescendingOrder}};
  auto state = headerState(view());
  setSortStates(state, sorts);
  saveProfile(GUI::MessageViewState, state);
  viewer()->loadSize();
  {
    const QSignalBlocker blocker(view()->header());
    view()->header()->setSortIndicator(MSG_MDL_AUTHOR_INDEX, Qt::DescendingOrder);
  }
  for (auto* feed : {m_first, m_second, m_first}) {
    loadItem(feed);
    QCOMPARE(sortStates(headerState(view())), sorts);
    QCOMPARE(view()->header()->sortIndicatorSection(), MSG_MDL_DCREATED_INDEX);
    QCOMPARE(view()->header()->sortIndicatorOrder(), Qt::DescendingOrder);
    QCOMPARE(articles(), QStringList({QSL("Alpha|Z"), QSL("Alpha|A"), QSL("Zulu|A"), QSL("Beta|B")}));
  }
}

void TestArticleSorting::hiddenSortingSurvivesVisibilityAndProfiles() {
  const SortStates sorts{{MSG_MDL_TITLE_INDEX, Qt::AscendingOrder}, {MSG_MDL_ID_INDEX, Qt::DescendingOrder}};
  auto state = headerState(view());
  setSortStates(state, sorts);
  state[QSL("header_%1_hidden").arg(MSG_MDL_TITLE_INDEX)] = true;
  saveProfile(GUI::MessageViewState, state);
  saveProfile(GUI::MessageViewStateFeed, state);
  viewer()->loadSize();
  loadItem(m_first);
  const auto sorted_articles = articles();
  for (bool hidden : {false, true}) {
    view()->setColumnHidden(MSG_MDL_TITLE_INDEX, hidden);
    QCOMPARE(sortStates(headerState(view())), sorts);
    QCOMPARE(articles(), sorted_articles);
  }
  QVERIFY(toggleProfiles(false));
  loadItem(m_folder);
  loadItem(m_second);
  QCOMPARE(sortStates(headerState(view())), sorts);
  QCOMPARE(articles(), sorted_articles);
  QVERIFY(view()->isColumnHidden(MSG_MDL_TITLE_INDEX));
  QVERIFY(view()->isColumnHidden(MSG_MDL_ID_INDEX));
  QCOMPARE(view()->header()->sortIndicatorSection(), MSG_MDL_TITLE_INDEX);
}

void TestArticleSorting::secondaryCtrlSortingKeepsPrimaryIndicator() {
  auto state = headerState(view());
  setSortStates(state, {{MSG_MDL_DCREATED_INDEX, Qt::DescendingOrder}});
  saveProfile(GUI::MessageViewState, state);
  viewer()->loadSize();
  QTest::keyPress(view(), Qt::Key_Control, Qt::ControlModifier);
  QVERIFY(QApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier));
  for (const auto order : {Qt::AscendingOrder, Qt::DescendingOrder, Qt::AscendingOrder}) {
    // With the arrow on the primary key, Qt starts a secondary header click at AscendingOrder.
    view()->header()->setSortIndicator(MSG_MDL_TITLE_INDEX, Qt::AscendingOrder);
    const SortStates expected{{MSG_MDL_DCREATED_INDEX, Qt::DescendingOrder}, {MSG_MDL_TITLE_INDEX, order}};
    QCOMPARE(sortStates(headerState(view())), expected);
    QCOMPARE(view()->header()->sortIndicatorSection(), MSG_MDL_DCREATED_INDEX);
    QCOMPARE(view()->header()->sortIndicatorOrder(), Qt::DescendingOrder);
  }
  QTest::keyRelease(view(), Qt::Key_Control);
}

bool TestArticleSorting::toggleProfiles(bool settings_dialog) {
  bool toggled = false;
  if (settings_dialog) {
    auto* action = m_window->findChild<QAction*>(QSL("m_actionSettings"));
    if (action == nullptr) {
      return false;
    }
    QTimer::singleShot(0, [&]() {
      auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
      if (dialog != nullptr) {
        QMetaObject::invokeMethod(dialog, "openSettingsCategory", Qt::DirectConnection, Q_ARG(int, 4));
        auto* checkbox = dialog->findChild<QCheckBox*>(QSL("m_checkArticleListColumnProfiles"));
        if (checkbox != nullptr) {
          checkbox->setChecked(!checkbox->isChecked());
          toggled = QMetaObject::invokeMethod(dialog, "applySettings", Qt::DirectConnection);
        }
        dialog->done(QDialog::Rejected);
      }
    });
    action->trigger();
  }
  else {
    QTimer::singleShot(0, [&]() {
      auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
      if (menu != nullptr) {
        for (auto* action : menu->actions()) {
          if (action->text() ==
              QCoreApplication::translate("MessagesView", "Use source-specific article list columns")) {
            action->trigger();
            toggled = true;
            break;
          }
        }
        menu->close();
      }
    });
    QMetaObject::invokeMethod(view()->header(),
                              "customContextMenuRequested",
                              Qt::DirectConnection,
                              Q_ARG(QPoint, QPoint(5, 5)));
  }
  return toggled;
}

void TestArticleSorting::profileToggleRefreshesArticles_data() {
  QTest::addColumn<bool>("settings_dialog");
  QTest::newRow("header-menu") << false;
  QTest::newRow("settings-dialog") << true;
}

void TestArticleSorting::profileToggleRefreshesArticles() {
  QFETCH(bool, settings_dialog);
  const SortStates common{{MSG_MDL_TITLE_INDEX, Qt::AscendingOrder}};
  const SortStates feeds{{MSG_MDL_TITLE_INDEX, Qt::DescendingOrder}};
  auto state = headerState(view());
  setSortStates(state, common);
  saveProfile(GUI::MessageViewState, state);
  setSortStates(state, feeds);
  saveProfile(GUI::MessageViewStateFeed, state);
  viewer()->loadSize();
  loadItem(m_first);
  const auto common_articles = articles();
  const QAbstractItemModel* model = view()->sourceModel();
  QSignalSpy resets(model, &QAbstractItemModel::modelReset);
  QVERIFY(toggleProfiles(settings_dialog));
  QCOMPARE(sortStates(headerState(view())), feeds);
  QCOMPARE(articles().first(), QSL("Zulu|A"));
  QCOMPARE(resets.count(), 1);
  QVERIFY(toggleProfiles(settings_dialog));
  QCOMPARE(sortStates(headerState(view())), common);
  QCOMPARE(articles(), common_articles);
  QCOMPARE(resets.count(), 2);
  const auto saved_common = QJsonDocument::fromJson(QByteArray::fromBase64(qApp->settings()
                                                                             ->value(GROUP(GUI), GUI::MessageViewState)
                                                                             .toString()
                                                                             .toLatin1()))
                              .object();
  const auto saved_feeds =
    QJsonDocument::fromJson(QByteArray::fromBase64(qApp->settings()
                                                     ->value(GROUP(GUI), GUI::MessageViewStateFeed)
                                                     .toString()
                                                     .toLatin1()))
      .object();
  QCOMPARE(sortStates(saved_common), common);
  QCOMPARE(sortStates(saved_feeds), feeds);
}

void TestArticleSorting::nestedHeaderSignalBlockingIsPreserved() {
  const QSignalBlocker blocker(view()->header());
  QVERIFY(QMetaObject::invokeMethod(view(),
                                    "onSortIndicatorChanged",
                                    Qt::DirectConnection,
                                    Q_ARG(int, MSG_MDL_AUTHOR_INDEX),
                                    Q_ARG(Qt::SortOrder, Qt::AscendingOrder)));
  QVERIFY(view()->header()->signalsBlocked());
}

void TestArticleSorting::sortingSurvivesRestartWithoutSelectingFeed() {
  auto state = headerState(view());
  setSortStates(state, {{MSG_MDL_ID_INDEX, Qt::DescendingOrder}});
  state[QSL("header_%1_hidden").arg(MSG_MDL_ID_INDEX)] = true;
  saveProfile(GUI::MessageViewState, state);
  qApp->settings()->sync();
  const auto profile = QFileInfo(qApp->settings()->fileName()).absoluteDir().absoluteFilePath(QSL(".."));
  for (int restart = 0; restart < 2; ++restart) {
    QProcess process;
    process.start(QCoreApplication::applicationFilePath(), {QSL("--verify-sorting"), profile});
    QVERIFY(process.waitForFinished(30000));
    const QByteArray output = process.readAllStandardOutput() + process.readAllStandardError();
    QVERIFY2(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0, output.constData());
  }
}

void TestArticleSorting::pluginColumnsUseExistingLayouts_data() {
  QTest::addColumn<bool>("profiles");
  QTest::newRow("common") << false;
  QTest::newRow("source-profiles") << true;
}

void TestArticleSorting::pluginColumnsUseExistingLayouts() {
  QFETCH(bool, profiles);
  qApp->settings()->setValue(GROUP(Messages), Messages::ArticleListColumnProfiles, profiles);
  m_account->m_columns = {{QSL("location"), QSL("Location")}, {QSL("end"), QSL("Ends")}};
  loadItem(m_first);
  constexpr int location = MSG_MDL_LABELS + 1;
  constexpr int end = location + 1;
  QCOMPARE(view()->sourceModel()->columnCount(), end + 1);
  QVERIFY(view()->isColumnHidden(location));
  view()->setColumnHidden(location, false);
  view()->header()->setSectionResizeMode(location, QHeaderView::Interactive);
  view()->header()->resizeSection(location, 171);
  view()->header()->setSectionResizeMode(end, QHeaderView::ResizeToContents);
  view()->header()->moveSection(view()->header()->visualIndex(location), 1);
  const auto state = headerState(view());
  loadItem(m_otherFeed);
  QCOMPARE(view()->sourceModel()->columnCount(), MSG_MDL_LABELS + 1);
  loadItem(m_second);
  QCOMPARE(headerState(view()).value(QSL("header_%1_size").arg(location)),
           state.value(QSL("header_%1_size").arg(location)));
  QVERIFY(!view()->isColumnHidden(location));
  QCOMPARE(view()->header()->sectionResizeMode(end), QHeaderView::ResizeToContents);
  QCOMPARE(view()->header()->visualIndex(location), 1);
  loadItem(m_otherFeed);
  loadItem(m_first);
  QCOMPARE(view()->header()->sectionSize(location), 171);
}

void TestArticleSorting::pluginColumnIdentitiesSurviveSchemaChanges() {
  m_account->m_columns = {{QSL("location"), QSL("Location")}, {QSL("end"), QSL("Ends")}};
  loadItem(m_first);
  constexpr int first_extra = MSG_MDL_LABELS + 1;
  view()->setColumnHidden(first_extra + 1, false);
  view()->header()->setSectionResizeMode(first_extra + 1, QHeaderView::Interactive);
  view()->header()->resizeSection(first_extra + 1, 222);
  view()->header()->moveSection(view()->header()->visualIndex(first_extra + 1), 0);
  m_account->m_columns = {{QSL("end"), QSL("Ends")}, {QSL("new"), QSL("New")}};
  loadItem(m_first);
  QCOMPARE(view()->header()->sectionSize(first_extra), 222);
  QVERIFY(!view()->isColumnHidden(first_extra));
  QCOMPARE(view()->header()->visualIndex(first_extra), 0);
  QVERIFY(view()->isColumnHidden(first_extra + 1));
  QCOMPARE(view()->sourceModel()->headerData(first_extra, Qt::Horizontal, COLUMN_ID_ROLE).toString(),
           QSL("article-sorting-test:end"));
}

void TestArticleSorting::pluginColumnsCannotChangeSorting() {
  m_account->m_columns = {{QSL("location"), QSL("Location")}};
  loadItem(m_first);
  const auto before = sortStates(headerState(view()));
  QSignalSpy resets(view()->sourceModel(), &QAbstractItemModel::modelReset);
  const int extra = MSG_MDL_LABELS + 1;
  view()->header()->setSortIndicator(extra, Qt::AscendingOrder);
  QCOMPARE(sortStates(headerState(view())), before);
  QCOMPARE(view()->header()->sortIndicatorSection(), before.first().first);
  QCOMPARE(resets.count(), 0);
  QTest::keyPress(view(), Qt::Key_Control, Qt::ControlModifier);
  view()->header()->setSortIndicator(extra, Qt::DescendingOrder);
  QTest::keyRelease(view(), Qt::Key_Control);
  QCOMPARE(sortStates(headerState(view())), before);
  auto forged = headerState(view());
  setSortStates(forged, {{extra, Qt::AscendingOrder}});
  view()->restoreHeaderState(QJsonDocument(forged).toJson());
  QCOMPARE(sortStates(headerState(view())), defaultSort);
  view()->setColumnHidden(MSG_MDL_DCREATED_INDEX, true);
  QCOMPARE(sortStates(headerState(view())), defaultSort);
}

void TestArticleSorting::pluginColumnRolesAndSearch() {
  m_account->m_columns = {{QSL("location"), QSL("Location")}, {QSL("start"), QSL("Starts"), MSG_MDL_DCREATED_INDEX}};
  loadItem(m_first);
  auto* source = view()->sourceModel();
  const int extra = MSG_MDL_LABELS + 1;
  const auto idx = source->index(0, extra);
  QVERIFY(source->data(idx, Qt::EditRole).toString().startsWith(QSL("extra-location-")));
  QVERIFY(!source->flags(idx).testFlag(Qt::ItemIsEditable));
  QVERIFY(!source->setData(idx, QSL("changed")));
  QCOMPARE(source->headerData(MSG_MDL_DCREATED_INDEX, Qt::Horizontal, Qt::EditRole).toString(), QSL("Starts"));
  QVERIFY(source->data(source->index(0, MSG_MDL_DCREATED_INDEX), Qt::EditRole).toDateTime().isValid());
  const bool tooltips = qApp->settings()->value(GROUP(Feeds), Feeds::EnableTooltipsFeedsMessages).toBool();
  qApp->settings()->setValue(GROUP(Feeds), Feeds::EnableTooltipsFeedsMessages, false);
  QVERIFY(!source->data(idx, Qt::ToolTipRole).isValid());
  qApp->settings()->setValue(GROUP(Feeds), Feeds::EnableTooltipsFeedsMessages, tooltips);
  auto* proxy = view()->model();
  proxy->setFilterKeyColumn(-1);
  proxy->setFilterFixedString(QSL("extra-location"));
  QCOMPARE(proxy->rowCount(), 4);
  proxy->setFilterKeyColumn(MSG_MDL_TITLE_INDEX);
  QCOMPARE(proxy->rowCount(), 0);
  proxy->setFilterFixedString(QString());
  proxy->setFilterKeyColumn(-1);
  const auto old_pattern = qApp->settings()->value(GROUP(Messages), Messages::CopyArticlePattern);
  qApp->settings()->setValue(GROUP(Messages), Messages::CopyArticlePattern, QSL("%19%"));
  const auto selected = proxy->index(0, MSG_MDL_TITLE_INDEX);
  view()->setCurrentIndex(selected);
  view()->selectionModel()->select(selected, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  QVERIFY(QMetaObject::invokeMethod(view(), "copyDataOfSelectedArticlesWithoutDialog", Qt::DirectConnection));
  QVERIFY(QApplication::clipboard()->text().startsWith(QSL("extra-location-")));
  qApp->settings()->setValue(GROUP(Messages), Messages::CopyArticlePattern, old_pattern);
}

void TestArticleSorting::pluginColumnDescriptorsAreValidated() {
  m_account->m_columns = {{QSL("valid"), QSL("Valid")},
                          {QSL("valid"), QSL("Duplicate")},
                          {QString(), QSL("Missing ID")},
                          {QSL("bad"), QSL("Bad target"), 999},
                          {QSL("title"), QSL("Event"), MSG_MDL_TITLE_INDEX},
                          {QSL("title-two"), QSL("Duplicate target"), MSG_MDL_TITLE_INDEX}};
  loadItem(m_first);
  QCOMPARE(view()->sourceModel()->columnCount(), MSG_MDL_LABELS + 2);
  QCOMPARE(view()->sourceModel()->headerData(MSG_MDL_TITLE_INDEX, Qt::Horizontal, Qt::EditRole).toString(),
           QSL("Event"));
}

void TestArticleSorting::legacyLayoutRestoresIntoExpandedSchema() {
  auto legacy = headerState(view());
  for (int column = 0; column <= MSG_MDL_LABELS; ++column) {
    legacy.remove(QSL("header_%1_id").arg(column));
  }
  legacy[QSL("header_%1_size").arg(MSG_MDL_AUTHOR_INDEX)] = 183;
  legacy[QSL("header_%1_resize_mode").arg(MSG_MDL_AUTHOR_INDEX)] = int(QHeaderView::Fixed);
  legacy[QSL("header_%1_hidden").arg(MSG_MDL_DCREATED_INDEX)] = true;
  setSortStates(legacy, {{MSG_MDL_DCREATED_INDEX, Qt::AscendingOrder}});
  m_account->m_columns = {{QSL("location"), QSL("Location")}};
  loadItem(m_otherFeed);
  saveProfile(GUI::MessageViewState + QSL("_provider_article-sorting-test"), legacy);
  loadItem(m_first);
  QCOMPARE(view()->sourceModel()->columnCount(), MSG_MDL_LABELS + 2);
  QCOMPARE(view()->header()->sectionSize(MSG_MDL_AUTHOR_INDEX), 183);
  QCOMPARE(view()->header()->sectionResizeMode(MSG_MDL_AUTHOR_INDEX), QHeaderView::Fixed);
  QVERIFY(view()->isColumnHidden(MSG_MDL_DCREATED_INDEX));
  QVERIFY(view()->isColumnHidden(MSG_MDL_LABELS + 1));
  QCOMPARE(sortStates(headerState(view())), SortStates({{MSG_MDL_DCREATED_INDEX, Qt::AscendingOrder}}));
  QCOMPARE(view()->header()->sortIndicatorSection(), MSG_MDL_DCREATED_INDEX);
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
  const bool verify_sorting = argc == 3 && QString::fromLocal8Bit(argv[1]) == QSL("--verify-sorting");
  const QString profile = verify_sorting ? QString::fromLocal8Bit(argv[2]) : directory.path();
  if (!verify_sorting) {
    prepareProfile(profile);
  }
  QStringList arguments{QString::fromLocal8Bit(argv[0]),
                        QSL("--data"),
                        profile,
                        QSL("--no-single-instance"),
                        QSL("--force-text-browser")};
  Application application(QSL("rssguard-article-sorting-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  application.setFeedReader(new FeedReader(&application));
  if (verify_sorting) {
    FormMain window;
    auto* viewer = window.tabWidget()->feedMessageViewer();
    auto* view = viewer->messagesView();
    if (sortStates(headerState(view)) != SortStates{{MSG_MDL_ID_INDEX, Qt::DescendingOrder}} ||
        !view->isColumnHidden(MSG_MDL_ID_INDEX)) {
      qWarning() << "Article sorting was lost on restart";
      return 2;
    }
    viewer->saveSize();
    application.settings()->sync();
    return 0;
  }
  TestArticleSorting tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_articlesorting.moc"
