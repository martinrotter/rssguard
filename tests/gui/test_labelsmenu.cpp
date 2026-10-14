// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasequeries.h"
#include "exceptions/sqlexception.h"
#include "gui/messagepreviewer.h"
#include "gui/reusable/labelsmenu.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "services/abstract/labelsnode.h"

#include <memory>

#include <QDataStream>
#include <QDir>
#include <QPointer>
#include <QSettings>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QToolBar>

Q_DECLARE_METATYPE(QList<Message>)

#if defined(Q_OS_WIN)
// Qt's generated MessagePreviewer metatypes need these operators, which the DLL does not export.
QDataStream& operator<<(QDataStream& out, const RootItem::ReadStatus& status) {
  return out << int(status);
}

QDataStream& operator>>(QDataStream& in, RootItem::ReadStatus& status) {
  int value;
  in >> value;
  status = static_cast<RootItem::ReadStatus>(value);
  return in;
}

QDataStream& operator<<(QDataStream& out, const RootItem::Importance& importance) {
  return out << int(importance);
}

QDataStream& operator>>(QDataStream& in, RootItem::Importance& importance) {
  int value;
  in >> value;
  importance = static_cast<RootItem::Importance>(value);
  return in;
}
#endif

namespace {
  class TestServiceRoot : public ServiceRoot {
    public:
      QList<Message> obtainNewMessages(Feed*,
                                       const QHash<ServiceRoot::BagOfMessages, QStringList>&,
                                       const QHash<QString, QStringList>&) override {
        return {};
      }

      QString code() const override {
        return QSL("labels-menu-test");
      }
  };

  void prepareProfile(const QString& profile) {
    QDir().mkpath(profile + QDir::separator() + QSL(APP_CFG_PATH));
    QSettings settings(profile + QDir::separator() + QSL(APP_CFG_PATH) + QDir::separator() + QSL(APP_CFG_FILE),
                       QSettings::IniFormat);
    settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
    settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
    settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
    settings.sync();
  }

  std::unique_ptr<MessagePreviewer> createPreviewer() {
    auto previewer = std::make_unique<MessagePreviewer>();
    auto* browser = previewer->webBrowser();
    // The article renderer is real, but these main-window notifications need no window in this fixture.
    QObject::disconnect(dynamic_cast<QObject*>(browser->viewer()),
                        SIGNAL(loadingFinished(bool)),
                        browser,
                        SLOT(onLoadingFinished(bool)));
    QObject::disconnect(dynamic_cast<QObject*>(browser->viewer()),
                        SIGNAL(linkMouseHighlighted(QUrl)),
                        browser,
                        SLOT(onLinkMouseHighlighted(QUrl)));
    return previewer;
  }

  QList<QPointer<QAction>> guardedActions(const LabelsMenu& menu) {
    QList<QPointer<QAction>> result;
    for (auto* action : menu.labelActions()) {
      result.append(action);
    }
    return result;
  }

  bool allDestroyed(const QList<QPointer<QAction>>& actions) {
    for (const auto& action : actions) {
      if (!action.isNull()) {
        return false;
      }
    }
    return true;
  }
} // namespace

class TestLabelsMenu : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();
    void assignmentsSurvivePaging();
    void previewToolbarSharesAssignmentsAfterPaging();
    void replacementReleasesActionsFromEveryPage();
    void previewClearForgetsArticle();
    void previewSwitchesToAccountWithoutLabels_data();
    void previewSwitchesToAccountWithoutLabels();

  private:
    Message seedArticle(Feed* feed, const QString& custom_id);
    int assignmentCount(const Message& message, Label* label = nullptr) const;

    std::unique_ptr<TestServiceRoot> m_account;
    std::unique_ptr<TestServiceRoot> m_emptyAccount;
    Feed* m_feed = nullptr;
    Feed* m_emptyFeed = nullptr;
    Message m_message;
    Message m_emptyMessage;
    QList<Label*> m_labels;
};

void TestLabelsMenu::initTestCase() {
  qRegisterMetaType<QList<Message>>();
  m_account = std::make_unique<TestServiceRoot>();
  m_emptyAccount = std::make_unique<TestServiceRoot>();
  m_account->setTitle(QSL("Account with labels"));
  m_emptyAccount->setTitle(QSL("Account without labels"));
  m_feed = new Feed(m_account.get());
  m_emptyFeed = new Feed(m_emptyAccount.get());
  m_feed->setTitle(QSL("Labeled feed"));
  m_emptyFeed->setTitle(QSL("Unlabeled feed"));
  m_account->appendChild(m_feed);
  m_emptyAccount->appendChild(m_emptyFeed);

  for (int i = 0; i < 21; ++i) {
    auto* label = new Label(QSL("Label %1").arg(i, 2, 10, QLatin1Char('0')), {}, m_account->labelsNode());
    m_account->labelsNode()->appendChild(label);
    m_labels.append(label);
  }

  qApp->database()->worker()->write([this](const QSqlDatabase& database) {
    DatabaseQueries::createOverwriteAccount(database, m_account.get());
    DatabaseQueries::createOverwriteAccount(database, m_emptyAccount.get());
    DatabaseQueries::createOverwriteFeed(database, m_feed, m_account->accountId(), NO_PARENT_CATEGORY);
    DatabaseQueries::createOverwriteFeed(database, m_emptyFeed, m_emptyAccount->accountId(), NO_PARENT_CATEGORY);
    for (auto* label : m_labels) {
      DatabaseQueries::createLabel(qApp->icons(), database, label, m_account->accountId());
    }
  });

  m_message = seedArticle(m_feed, QSL("labeled-article"));
  m_emptyMessage = seedArticle(m_emptyFeed, QSL("unlabeled-article"));
  QVERIFY(m_message.m_id > 0);
  QVERIFY(m_emptyMessage.m_id > 0);
}

Message TestLabelsMenu::seedArticle(Feed* feed, const QString& custom_id) {
  Message message;
  message.m_title = custom_id;
  message.m_customId = custom_id;
  message.m_feedId = feed->id();
  message.m_accountId = feed->account()->accountId();
  message.m_contents = QSL("<p>Article for labels menu tests.</p>");
  message.m_created = QDateTime::currentDateTimeUtc();
  message.m_retrieved = message.m_created;
  QList<Message> messages{message};
  DatabaseQueries::updateMessages(qApp->database(), qApp->settings(), messages, feed, false, true);
  return messages.first();
}

void TestLabelsMenu::init() {
  qApp->database()->worker()->write([](const QSqlDatabase& database) {
    QSqlQuery query(database);
    if (!query.exec(QSL("DELETE FROM LabelsInMessages;"))) {
      throw SqlException(query.lastError());
    }
  });
  m_message.m_assignedLabels.clear();
  m_emptyMessage.m_assignedLabels.clear();
}

void TestLabelsMenu::cleanup() {
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

int TestLabelsMenu::assignmentCount(const Message& message, Label* label) const {
  return qApp->database()->worker()->read<int>([&](const QSqlDatabase& database) {
    QSqlQuery query(database);
    query.prepare(label == nullptr
                    ? QSL("SELECT COUNT(*) FROM LabelsInMessages WHERE message = :message;")
                    : QSL("SELECT COUNT(*) FROM LabelsInMessages WHERE message = :message AND label = :label;"));
    query.bindValue(QSL(":message"), message.m_id);
    if (label != nullptr) {
      query.bindValue(QSL(":label"), label->id());
    }
    if (!query.exec() || !query.next()) {
      throw SqlException(query.lastError());
    }
    return query.value(0).toInt();
  });
}

void TestLabelsMenu::assignmentsSurvivePaging() {
  LabelsMenu menu;
  menu.setMessages({m_message});
  menu.setLabels(m_labels);
  QSignalSpy changed(&menu, &LabelsMenu::setModelArticleLabelIds);
  QVERIFY(changed.isValid());
  auto* first = menu.labelActions().first();
  auto* last = menu.labelActions().last();

  for (int pass = 0; pass < 2; ++pass) {
    menu.actions().last()->trigger();
    QVERIFY(menu.actions().contains(last));
    menu.actions().first()->trigger();
    QVERIFY(menu.actions().contains(first));
    first->trigger();
    QCOMPARE(assignmentCount(m_message, m_labels.first()), pass == 0 ? 1 : 0);
    QCOMPARE(changed.count(), pass * 2 + 1);
    QCOMPARE(menu.messages().first().m_assignedLabels.contains(m_labels.first()), pass == 0);

    menu.actions().last()->trigger();
    last->trigger();
    QCOMPARE(assignmentCount(m_message, m_labels.last()), pass == 0 ? 1 : 0);
    QCOMPARE(changed.count(), pass * 2 + 2);
    QCOMPARE(menu.messages().first().m_assignedLabels.contains(m_labels.last()), pass == 0);
    menu.actions().first()->trigger();
  }
}

void TestLabelsMenu::previewToolbarSharesAssignmentsAfterPaging() {
  auto previewer = createPreviewer();
  previewer->loadMessage(m_message, m_feed, m_feed);
  auto* menu = previewer->menuLabels();
  auto* toolbar = previewer->findChild<QToolBar*>(QString(), Qt::FindDirectChildrenOnly);
  QVERIFY(toolbar != nullptr);
  auto* first = menu->labelActions().first();
  QVERIFY(toolbar->actions().contains(first));
  auto* button = qobject_cast<QToolButton*>(toolbar->widgetForAction(first));
  QVERIFY(button != nullptr);
  QCOMPARE(button->defaultAction(), first);
  QSignalSpy changed(menu, &LabelsMenu::setModelArticleLabelIds);

  menu->actions().last()->trigger();
  menu->actions().first()->trigger();
  first->trigger();
  QCOMPARE(assignmentCount(m_message, m_labels.first()), 1);
  QCOMPARE(changed.count(), 1);
  QVERIFY(button->isChecked());

  // The toolbar uses the same paged action, so its own button must still persist a removal.
  button->click();
  QCOMPARE(assignmentCount(m_message, m_labels.first()), 0);
  QCOMPARE(changed.count(), 2);
  QVERIFY(!first->isChecked());
}

void TestLabelsMenu::replacementReleasesActionsFromEveryPage() {
  LabelsMenu menu;
  menu.setMessages({m_message});
  menu.setLabels(m_labels);
  const auto old_actions = guardedActions(menu);
  menu.actions().last()->trigger();

  // Refreshing in response to a toggle happens while the old QAction is still emitting its signal.
  connect(&menu, &LabelsMenu::setModelArticleLabelIds, &menu, [&menu](const QList<Message>&) {
    menu.setLabels({});
  });
  menu.labelActions().last()->trigger();
  QCOMPARE(assignmentCount(m_message, m_labels.last()), 1);
  QVERIFY(menu.labelActions().isEmpty());
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QVERIFY(allDestroyed(old_actions));

  for (int refresh = 0; refresh < 3; ++refresh) {
    const QPointer<QAction> old_placeholder = menu.actions().first();
    menu.setLabels({});
    QCOMPARE(menu.actions().size(), 1);
    QVERIFY(!menu.actions().first()->isEnabled());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(old_placeholder.isNull());
  }
}

void TestLabelsMenu::previewClearForgetsArticle() {
  auto previewer = createPreviewer();
  previewer->loadMessage(m_message, m_feed, m_feed);
  auto* menu = previewer->menuLabels();
  const auto old_actions = guardedActions(*menu);
  menu->actions().last()->trigger();
  previewer->clear();
  QVERIFY(menu->messages().isEmpty());
  QVERIFY(menu->labelActions().isEmpty());
  QCOMPARE(menu->actions().size(), 1);
  QVERIFY(!menu->actions().first()->isEnabled());
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QVERIFY(allDestroyed(old_actions));
  QCOMPARE(assignmentCount(m_message), 0);
}

void TestLabelsMenu::previewSwitchesToAccountWithoutLabels_data() {
  QTest::addColumn<bool>("clear_first");
  QTest::newRow("direct-switch") << false;
  QTest::newRow("clear-before-switch") << true;
}

void TestLabelsMenu::previewSwitchesToAccountWithoutLabels() {
  QFETCH(bool, clear_first);
  auto previewer = createPreviewer();
  previewer->loadMessage(m_message, m_feed, m_feed);
  auto* menu = previewer->menuLabels();
  const auto old_actions = guardedActions(*menu);
  if (clear_first) {
    previewer->clear();
  }
  previewer->loadMessage(m_emptyMessage, m_emptyFeed, m_emptyFeed);
  QCOMPARE(menu->messages().size(), 1);
  QCOMPARE(menu->messages().first().m_id, m_emptyMessage.m_id);
  QVERIFY(menu->labelActions().isEmpty());
  QCOMPARE(menu->actions().size(), 1);
  QVERIFY(!menu->actions().first()->isEnabled());
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QVERIFY(allDestroyed(old_actions));
  QCOMPARE(assignmentCount(m_message), 0);
  QCOMPARE(assignmentCount(m_emptyMessage), 0);
}

int main(int argc, char* argv[]) {
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
  Application application(QSL("rssguard-labels-menu-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  TestLabelsMenu tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_labelsmenu.moc"
