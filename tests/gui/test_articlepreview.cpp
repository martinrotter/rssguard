// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/messagepreviewer.h"
#include "gui/tabwidget.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"

#include <memory>

#include <QDir>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace {
  class TestServiceRoot : public ServiceRoot {
    public:
      QString code() const override {
        return QSL("article-preview-test");
      }

      QList<Message> obtainNewMessages(Feed*,
                                       const QHash<BagOfMessages, QStringList>&,
                                       const QHash<QString, QStringList>&) override {
        return {};
      }
  };

  class ShowEventCounter : public QObject {
    public:
      int count = 0;

    protected:
      bool eventFilter(QObject*, QEvent* event) override {
        if (event->type() == QEvent::Show) {
          ++count;
        }
        return false;
      }
  };

  void prepareProfile(const QString& profile) {
    QDir().mkpath(profile + QDir::separator() + QSL(APP_CFG_PATH));
    QSettings settings(profile + QDir::separator() + QSL(APP_CFG_PATH) + QDir::separator() + QSL(APP_CFG_FILE),
                       QSettings::IniFormat);
    settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
    settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
    settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
    settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::FeedsUpdateOnStartup, false);
    settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::AutoUpdateEnabled, false);
    settings.sync();
  }
} // namespace

class TestArticlePreview : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void delayedTabLoadPreservesVisibility_data();
    void delayedTabLoadPreservesVisibility();
    void embeddedPreviewAfterClear_data();
    void embeddedPreviewAfterClear();
    void embeddedPreviewDoesNotRevealBackgroundTab();

  private:
    int openArticleTab();
    bool displayArticle(RootItem* selected_item);

    std::unique_ptr<TestServiceRoot> m_account;
    Feed* m_feed = nullptr;
    Message m_message;
    std::unique_ptr<FormMain> m_window;
};

void TestArticlePreview::initTestCase() {
  m_account = std::make_unique<TestServiceRoot>();
  m_feed = new Feed(m_account.get());
  m_feed->setTitle(QSL("Preview test feed"));
  m_account->appendChild(m_feed);
  m_message.m_id = 1;
  m_message.m_title = QSL("Preview test article");
  m_message.m_contents = QSL("<p>Article preview regression test.</p>");
  m_message.m_created = QDateTime::currentDateTimeUtc();
  m_message.m_retrieved = m_message.m_created;
  m_window = std::make_unique<FormMain>();
  m_window->resize(1000, 700);
  m_window->show();
  QTRY_VERIFY(m_window->isVisible());
}

void TestArticlePreview::init() {
  auto* tabs = m_window->tabWidget();
  QVERIFY(QMetaObject::invokeMethod(tabs, "closeAllTabs", Qt::DirectConnection));
  tabs->setCurrentIndex(0);
  qApp->settings()->setValue(GROUP(Messages), Messages::EnableMessagePreview, true);
  qApp->settings()->setValue(GROUP(Messages), Messages::AlwaysDisplayItemPreview, false);
  tabs->feedMessageViewer()->updateArticleViewerSettings();
  QVERIFY(tabs->feedMessageViewer()->messagesBrowser()->isHidden());
}

int TestArticlePreview::openArticleTab() {
  int index = -1;
  // Invoke the real slot, including its delayed load, without exporting the internal TabWidget class.
  QMetaObject::invokeMethod(m_window->tabWidget(),
                            "addSingleMessageView",
                            Qt::DirectConnection,
                            Q_RETURN_ARG(int, index),
                            Q_ARG(RootItem*, m_feed),
                            Q_ARG(Message, m_message),
                            Q_ARG(Feed*, m_feed));
  return index;
}

bool TestArticlePreview::displayArticle(RootItem* selected_item) {
  return QMetaObject::invokeMethod(m_window->tabWidget()->feedMessageViewer(),
                                   "displayMessage",
                                   Qt::DirectConnection,
                                   Q_ARG(Message, m_message),
                                   Q_ARG(RootItem*, selected_item),
                                   Q_ARG(Feed*, m_feed));
}

void TestArticlePreview::delayedTabLoadPreservesVisibility_data() {
  QTest::addColumn<int>("selection");
  QTest::newRow("background") << 0;
  QTest::newRow("selected-before-load") << 1;
  QTest::newRow("switched-back-before-load") << 2;
}

void TestArticlePreview::delayedTabLoadPreservesVisibility() {
  QFETCH(int, selection);
  auto* tabs = m_window->tabWidget();
  const int index = openArticleTab();
  QCOMPARE(index, 1);
  auto* page = tabs->widget(index);
  auto* browser = page->findChild<WebBrowser*>();
  QVERIFY(browser != nullptr);
  QVERIFY(page->isHidden());
  QSignalSpy loaded(dynamic_cast<QObject*>(browser->viewer()), SIGNAL(loadingFinished(bool)));
  QVERIFY(loaded.isValid());
  if (selection != 0) {
    tabs->setCurrentIndex(index);
    QVERIFY(page->isVisible());
    if (selection == 2) {
      tabs->setCurrentIndex(0);
    }
  }
  ShowEventCounter shows;
  page->installEventFilter(&shows);
  QTRY_COMPARE(loaded.count(), 1);
  QVERIFY(loaded.constFirst().constFirst().toBool());
  QCOMPARE(shows.count, 0);
  QCOMPARE(tabs->currentIndex(), selection == 1 ? index : 0);
  QCOMPARE(page->isVisible(), selection == 1);
  QCOMPARE(page->isHidden(), selection != 1);
  QVERIFY(browser->viewer()->plainText().contains(QSL("Article preview regression test.")));

  // A loaded background page must still display normally when the user selects it.
  tabs->setCurrentIndex(index);
  QVERIFY(page->isVisible());
  QVERIFY(browser->isVisible());
  QVERIFY(tabs->feedMessageViewer()->isHidden());
}

void TestArticlePreview::embeddedPreviewAfterClear_data() {
  QTest::addColumn<bool>("valid_item");
  QTest::newRow("selected-article") << true;
  QTest::newRow("no-selected-item") << false;
}

void TestArticlePreview::embeddedPreviewAfterClear() {
  QFETCH(bool, valid_item);
  auto* feeds = m_window->tabWidget()->feedMessageViewer();
  auto* preview = feeds->messagesBrowser();
  for (int pass = 0; pass < 2; ++pass) {
    QVERIFY(QMetaObject::invokeMethod(preview, "clear", Qt::DirectConnection));
    QVERIFY(preview->isHidden());
    QVERIFY(displayArticle(valid_item ? m_feed : nullptr));
    QCOMPARE(preview->isVisible(), valid_item);
    QCOMPARE(preview->isHidden(), !valid_item);
    if (valid_item) {
      QVERIFY(feeds->webBrowser()->isVisible());
      QVERIFY(feeds->webBrowser()->viewer()->plainText().contains(QSL("Article preview regression test.")));
    }
  }
}

void TestArticlePreview::embeddedPreviewDoesNotRevealBackgroundTab() {
  auto* tabs = m_window->tabWidget();
  auto* feeds = tabs->feedMessageViewer();
  const int index = openArticleTab();
  QCOMPARE(index, 1);
  tabs->setCurrentIndex(index);
  ShowEventCounter shows;
  feeds->installEventFilter(&shows);
  QVERIFY(displayArticle(m_feed));
  QVERIFY(!feeds->messagesBrowser()->isHidden());
  QVERIFY(!feeds->messagesBrowser()->isVisible());
  QVERIFY(feeds->isHidden());
  QCOMPARE(tabs->currentIndex(), index);
  QCOMPARE(shows.count, 0);
  tabs->setCurrentIndex(0);
  QVERIFY(feeds->messagesBrowser()->isVisible());
  QVERIFY(feeds->webBrowser()->viewer()->plainText().contains(QSL("Article preview regression test.")));
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
  Application application(QSL("rssguard-article-preview-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  application.setFeedReader(new FeedReader(&application));
  TestArticlePreview tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_articlepreview.moc"
