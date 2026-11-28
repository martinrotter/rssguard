// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/messagepreviewer.h"
#include "gui/tabwidget.h"
#include "gui/webbrowser.h"
#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "services/abstract/gui/custommessagepreviewer.h"

#include <memory>

#include <QAction>
#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QVBoxLayout>

namespace {
  class TestMessagePreviewer : public CustomMessagePreviewer {
    public:
      explicit TestMessagePreviewer(bool has_browser)
        : m_browser(new WebBrowser(nullptr, this)), m_hasBrowser(has_browser) {
        auto* layout = new QVBoxLayout(this);
        layout->addWidget(m_browser);
        m_browser->setToolBarVisible(false);
      }

      WebBrowser* webBrowser() const override {
        return m_hasBrowser ? m_browser : nullptr;
      }

      void clear() override {
        m_browser->clear(false);
      }

      void loadMessage(const Message& message, RootItem* selected_item, Feed* feed) override {
        auto long_message = message;
        long_message.m_contents = message.m_contents.repeated(200);
        m_browser->loadMessage(long_message, selected_item, feed);
      }

    private:
      WebBrowser* m_browser;
      bool m_hasBrowser;
  };

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

      CustomMessagePreviewer* customMessagePreviewer() override {
        return m_previewer.get();
      }

      std::unique_ptr<TestMessagePreviewer> m_previewer;
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
    settings.setValue(GROUP(Messages) + QSL("/") + Messages::AlwaysDisplayItemPreview, true);
    settings.sync();
  }
} // namespace

class TestArticlePreview : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void delayedTabLoadPreservesVisibility_data();
    void delayedTabLoadPreservesVisibility();
    void delayedTabClosesWhenFeedIsRemoved();
    void textSearchWrapsBackward();
    void linklessArticleUsesMatchingFeedBaseUrl();
    void embeddedPreviewAfterClear_data();
    void embeddedPreviewAfterClear();
    void embeddedPreviewDoesNotRevealBackgroundTab();
    void customPreviewScrollRouting_data();
    void customPreviewScrollRouting();
    void defaultBrowserWarmup_data();
    void defaultBrowserWarmup();
    void cacheCleanupWithoutSelectedBrowser_data();
    void cacheCleanupWithoutSelectedBrowser();

  private:
    int openArticleTab(Feed* feed);
    bool displayArticle(RootItem* selected_item);
    bool selectContent(int content, bool hidden);

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
  QCOMPARE(m_window->tabWidget()->feedMessageViewer()->webBrowser(), nullptr);
}

void TestArticlePreview::init() {
  auto* tabs = m_window->tabWidget();
  QVERIFY(QMetaObject::invokeMethod(tabs, "closeAllTabs", Qt::DirectConnection));
  tabs->setCurrentIndex(0);
  qApp->settings()->setValue(GROUP(Messages), Messages::EnableMessagePreview, true);
  qApp->settings()->setValue(GROUP(Messages), Messages::AlwaysDisplayItemPreview, false);
  tabs->feedMessageViewer()->updateArticleViewerSettings();
  QVERIFY(tabs->feedMessageViewer()->messagesBrowser()->isHidden());
  m_account->m_previewer.reset();
}

int TestArticlePreview::openArticleTab(Feed* feed) {
  int index = -1;
  // Invoke the real slot, including its delayed load, without exporting the internal TabWidget class.
  QMetaObject::invokeMethod(m_window->tabWidget(),
                            "addSingleMessageView",
                            Qt::DirectConnection,
                            Q_RETURN_ARG(int, index),
                            Q_ARG(RootItem*, feed),
                            Q_ARG(Message, m_message),
                            Q_ARG(Feed*, feed));
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
  const int index = openArticleTab(m_feed);
  QCOMPARE(index, 1);
  auto* page = tabs->widget(index);
  auto* browser = page->findChild<WebBrowser*>();
  QVERIFY(browser != nullptr);
  QCOMPARE(page->webBrowser(), browser);
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
  QVERIFY(m_window->findChild<QAction*>(QSL("m_actionBrowserScrollUp"))->isEnabled());
}

void TestArticlePreview::delayedTabClosesWhenFeedIsRemoved() {
  auto* tabs = m_window->tabWidget();
  auto feed = std::make_unique<Feed>(m_account.get());
  const int index = openArticleTab(feed.get());

  QCOMPARE(index, 1);
  QPointer<QWidget> page = tabs->widget(index);
  QVERIFY(!page.isNull());
  feed.reset();

  QTRY_COMPARE(tabs->count(), 1);
  QTRY_VERIFY(page.isNull());
}

void TestArticlePreview::textSearchWrapsBackward() {
  TextBrowserViewer viewer;
  const QString content = QSL("First match, second match, end of article.");
  const QString query = QSL("match");
  viewer.setPlainText(content);
  viewer.moveCursor(QTextCursor::MoveOperation::End);

  viewer.findText(query, true);
  QCOMPARE(viewer.textCursor().selectionStart(), int(content.lastIndexOf(query)));
  viewer.findText(query, true);
  QCOMPARE(viewer.textCursor().selectionStart(), int(content.indexOf(query)));
  viewer.findText(query, true);
  QCOMPARE(viewer.textCursor().selectionStart(), int(content.lastIndexOf(query)));
  QCOMPARE(viewer.textCursor().selectedText(), query);
}

void TestArticlePreview::linklessArticleUsesMatchingFeedBaseUrl() {
  TestServiceRoot account;
  auto* feed = new Feed(&account);
  feed->setId(42);
  feed->setSource(QSL("https://reader:secret@example.invalid:8443/feeds/\U0001F4F0/news.xml?token=secret#section"));
  account.appendChild(feed);

  auto message = m_message;
  message.m_feedId = feed->id();
  TextBrowserViewer viewer;
  viewer.loadMessage(message, &account, feed);
  QCOMPARE(viewer.url(), QUrl(QSL("https://example.invalid:8443/feeds/\U0001F4F0/news.xml")));
  QCOMPARE(viewer.document()->baseUrl().resolved(QUrl(QSL("images/photo.png"))),
           QUrl(QSL("https://example.invalid:8443/feeds/\U0001F4F0/images/photo.png")));

  message.m_feedId = 43;
  viewer.loadMessage(message, &account, nullptr);
  QVERIFY(viewer.url().isEmpty());
  QVERIFY(viewer.plainText().contains(QSL("Article preview regression test.")));
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
  const int index = openArticleTab(m_feed);
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

void TestArticlePreview::customPreviewScrollRouting_data() {
  QTest::addColumn<bool>("has_browser");
  QTest::newRow("browser") << true;
  QTest::newRow("no-browser") << false;
}

void TestArticlePreview::customPreviewScrollRouting() {
  QFETCH(bool, has_browser);
  auto* tabs = m_window->tabWidget();
  auto* feeds = tabs->feedMessageViewer();
  auto* preview = feeds->messagesBrowser();
  auto* default_browser = feeds->webBrowser();
  m_account->m_previewer = std::make_unique<TestMessagePreviewer>(has_browser);
  auto* browser = m_account->m_previewer->webBrowser();
  QVERIFY(displayArticle(m_feed));
  QCOMPARE(feeds->webBrowser(), browser);
  auto* scroll_down = m_window->findChild<QAction*>(QSL("m_actionBrowserScrollDown"));
  auto* scroll_up = m_window->findChild<QAction*>(QSL("m_actionBrowserScrollUp"));
  QVERIFY(scroll_down != nullptr && scroll_up != nullptr);
  QCOMPARE(scroll_down->isEnabled(), has_browser);
  QCOMPARE(scroll_up->isEnabled(), has_browser);
  if (has_browser) {
    QTRY_VERIFY(browser->isVisible());
    scroll_down->trigger();
    QTRY_VERIFY(browser->verticalScrollBarPosition() > 0.0);
    QCOMPARE(default_browser->verticalScrollBarPosition(), 0.0);
    scroll_up->trigger();
    QTRY_COMPARE(browser->verticalScrollBarPosition(), 0.0);
  }
  preview->hide();
  QCOMPARE(feeds->webBrowser(), browser);
  QVERIFY(!scroll_down->isEnabled());
  if (has_browser) {
    browser->setVerticalScrollBarPosition(100.0);
    const double position = browser->verticalScrollBarPosition();
    QVERIFY(position > 0.0);
    QVERIFY(QMetaObject::invokeMethod(tabs, "scrollUpCurrentBrowser", Qt::DirectConnection));
    QCOMPARE(browser->verticalScrollBarPosition(), position);
    QVERIFY(QMetaObject::invokeMethod(tabs, "scrollDownCurrentBrowser", Qt::DirectConnection));
    QCOMPARE(browser->verticalScrollBarPosition(), position);
  }
  preview->show();
  QCOMPARE(feeds->webBrowser(), browser);
  QCOMPARE(scroll_down->isEnabled(), has_browser);

  QVERIFY(QMetaObject::invokeMethod(preview, "showItemDetails", Qt::DirectConnection, Q_ARG(RootItem*, m_feed)));
  QCOMPARE(feeds->webBrowser(), nullptr);
  QVERIFY(!scroll_down->isEnabled());
  m_account->m_previewer.reset();
  QVERIFY(displayArticle(m_feed));
  QCOMPARE(feeds->webBrowser(), default_browser);
  QVERIFY(scroll_down->isEnabled());
}

bool TestArticlePreview::selectContent(int content, bool hidden) {
  auto* preview = m_window->tabWidget()->feedMessageViewer()->messagesBrowser();
  if (content == 1 || content == 2) {
    m_account->m_previewer = std::make_unique<TestMessagePreviewer>(content == 1);
  }
  const bool loaded =
    content == 3 ? QMetaObject::invokeMethod(preview, "showItemDetails", Qt::DirectConnection, Q_ARG(RootItem*, m_feed))
                 : displayArticle(m_feed);
  if (hidden) {
    preview->hide();
  }
  return loaded;
}

void TestArticlePreview::defaultBrowserWarmup_data() {
  QTest::addColumn<int>("content");
  QTest::addColumn<bool>("hidden");
  const QStringList names{QSL("default"), QSL("custom-browser"), QSL("custom-no-browser"), QSL("item-details")};
  for (int content = 0; content < names.size(); ++content) {
    for (bool hidden : {false, true}) {
      const auto name = names.at(content) + (hidden ? QSL("-hidden") : QSL("-visible"));
      QTest::newRow(qPrintable(name)) << content << hidden;
    }
  }
}

void TestArticlePreview::defaultBrowserWarmup() {
  QFETCH(int, content);
  QFETCH(bool, hidden);
  auto* feeds = m_window->tabWidget()->feedMessageViewer();
  auto* default_browser = feeds->webBrowser();
  QVERIFY(default_browser != nullptr);
  QVERIFY(selectContent(content, hidden));
  auto* selected_browser = feeds->webBrowser();
  QSignalSpy loaded(dynamic_cast<QObject*>(default_browser->viewer()), SIGNAL(loadingFinished(bool)));
  QVERIFY(loaded.isValid());
  feeds->warmUpBrowser();
  QTRY_VERIFY(!loaded.isEmpty() && loaded.constLast().constFirst().toBool());
  QTRY_VERIFY(default_browser->viewer()->plainText().trimmed().isEmpty());
  QCOMPARE(feeds->webBrowser(), selected_browser);
  QCOMPARE(feeds->messagesBrowser()->isHidden(), hidden);
  if (content == 1) {
    QTRY_VERIFY(selected_browser->viewer()->plainText().contains(QSL("Article preview regression test.")));
  }
}

void TestArticlePreview::cacheCleanupWithoutSelectedBrowser_data() {
  defaultBrowserWarmup_data();
}

void TestArticlePreview::cacheCleanupWithoutSelectedBrowser() {
  QFETCH(int, content);
  QFETCH(bool, hidden);
  QVERIFY(selectContent(content, hidden));
  auto* feeds = m_window->tabWidget()->feedMessageViewer();
  auto* selected_browser = feeds->webBrowser();
  QImage image(1, 1, QImage::Format::Format_ARGB32);
  image.fill(Qt::GlobalColor::red);
  QByteArray image_data;
  QBuffer buffer(&image_data);
  QVERIFY(buffer.open(QIODevice::OpenModeFlag::WriteOnly));
  QVERIFY(image.save(&buffer, "PNG"));
  const QUrl url(QSL("https://cache.example.invalid/preview.png"));
  QVERIFY(TextBrowserImageCache::saveImage(url, image_data));
  QVERIFY(TextBrowserImageCache::loadImage(url, image));
  auto* cleanup = m_window->findChild<QAction*>(QSL("m_actionCleanupWebCache"));
  QVERIFY(cleanup != nullptr && cleanup->isEnabled());
  cleanup->trigger();
  QVERIFY(!QFileInfo::exists(TextBrowserImageCache::cacheFilePath(url)));
  QCOMPARE(feeds->webBrowser(), selected_browser);
  QCOMPARE(feeds->messagesBrowser()->isHidden(), hidden);
}

void TestArticlePreview::cleanupTestCase() {
  m_window->tabWidget()->setCurrentIndex(0);
  QVERIFY(displayArticle(m_feed));
  QPointer<MessagePreviewer> preview(m_window->tabWidget()->feedMessageViewer()->messagesBrowser());
  QVERIFY(preview->isVisible());
  auto* scroll_up = m_window->findChild<QAction*>(QSL("m_actionBrowserScrollUp"));
  QVERIFY(scroll_up != nullptr);
  QVERIFY(scroll_up->isEnabled());
  preview->hide();
  QVERIFY(!scroll_up->isEnabled());
  preview->show();
  QVERIFY(scroll_up->isEnabled());
  m_window.reset();
  QVERIFY(preview.isNull());
}

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  prepareProfile(directory.path());
  QStringList arguments{QString::fromLocal8Bit(argv[0]), QSL("--data"), directory.path(), QSL("--no-single-instance")};
  if (!qEnvironmentVariableIsSet("RSSGUARD_TEST_WEBENGINE")) {
    arguments.append(QSL("--force-text-browser"));
  }
  Application application(QSL("rssguard-article-preview-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  application.setFeedReader(new FeedReader(&application));
  TestArticlePreview tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_articlepreview.moc"
