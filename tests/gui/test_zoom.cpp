// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webbrowser.h"
#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "network-web/webfactory.h"

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
#include "gui/webviewers/qtwebengine/webengineviewer.h"

#include <QWebEnginePage>
#endif

#include <limits>
#include <memory>

#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QWheelEvent>

namespace {
  const qreal initial_zoom = 1.4;

  bool usesWebEngine() {
    return qEnvironmentVariableIsSet("RSSGUARD_TEST_WEBENGINE");
  }

  QString settingsPath(const QString& profile) {
    return profile + QDir::separator() + QSL(APP_CFG_PATH) + QDir::separator() + QSL(APP_CFG_FILE);
  }

  void prepareProfile(const QString& profile, const QVariant& zoom) {
    QDir().mkpath(profile + QDir::separator() + QSL(APP_CFG_PATH));
    QSettings settings(settingsPath(profile), QSettings::IniFormat);
    settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
    settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
    settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
    settings.setValue(GROUP(Messages) + QSL("/") + Messages::Zoom, zoom);
    settings.sync();
  }

  std::unique_ptr<WebBrowser> createBrowser() {
    auto browser = std::make_unique<WebBrowser>();
#if defined(Q_OS_WIN) && QT_VERSION_MAJOR == 5
    browser->setAttribute(Qt::WA_DontShowOnScreen);
#endif
    // No main window is needed for zoom tests; suppress its delayed status-bar notification.
    QObject::disconnect(dynamic_cast<QObject*>(browser->viewer()),
                        SIGNAL(loadingFinished(bool)),
                        browser.get(),
                        SLOT(onLoadingFinished(bool)));
    QObject::disconnect(dynamic_cast<QObject*>(browser->viewer()),
                        SIGNAL(linkMouseHighlighted(QUrl)),
                        browser.get(),
                        SLOT(onLinkMouseHighlighted(QUrl)));
    return browser;
  }

  void sendZoomInput(WebBrowser& browser, bool wheel, bool increase) {
    auto* text = dynamic_cast<TextBrowserViewer*>(browser.viewer());
    if (wheel) {
      // Exercise the text viewport's native wheel dispatch, including its default Qt zoom fallback.
      QWidget* receiver = text != nullptr ? text->viewport() : dynamic_cast<QWidget*>(browser.viewer())->focusProxy();
      QWheelEvent event(QPointF(5, 5),
                        QPointF(receiver->mapToGlobal(QPoint(5, 5))),
                        QPoint(),
                        QPoint(0, increase ? 120 : -120),
                        Qt::NoButton,
                        Qt::ControlModifier,
                        Qt::NoScrollPhase,
                        false);
      QCoreApplication::sendEvent(receiver, &event);
    }
    else {
      QWidget* receiver =
        text != nullptr ? static_cast<QWidget*>(text) : dynamic_cast<QWidget*>(browser.viewer())->focusProxy();
      QTest::keyClick(receiver, increase ? Qt::Key_Plus : Qt::Key_Minus, Qt::ControlModifier);
    }
  }

  bool waitForLoad(QSignalSpy& loaded) {
    return !loaded.isEmpty() || loaded.wait(10000);
  }

  bool initializeInputReceiver(WebBrowser& browser) {
    if (dynamic_cast<TextBrowserViewer*>(browser.viewer()) != nullptr) {
      return true;
    }
    QSignalSpy loaded(dynamic_cast<QObject*>(browser.viewer()), SIGNAL(loadingFinished(bool)));
    browser.resize(600, 300);
    browser.show();
    browser.setHtml(QSL("<p>Article for native zoom input</p>"));
    return waitForLoad(loaded) && loaded.last().at(0).toBool() &&
           dynamic_cast<QWidget*>(browser.viewer())->focusProxy() != nullptr;
  }

  void finishViewerTeardown() {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
    // Renderer cleanup is asynchronous; finish it before QApplication shuts Chromium down.
    if (usesWebEngine()) {
      QTest::qWait(100);
      QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
#endif
  }

  int probeRestoredZoom(const QString& profile, qreal expected, const QString& test_name) {
    QProcess process;
    process.start(QCoreApplication::applicationFilePath(),
                  {QSL("--zoom-restore-probe"), profile, QString::number(expected, 'g', 17)});
    if (!process.waitForStarted(10000) || !process.waitForFinished(30000)) {
      process.kill();
      process.waitForFinished();
      qWarning().noquote() << test_name << "zoom restoration subprocess did not finish:" << process.errorString();
      return -1;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
      qWarning().noquote() << test_name << process.readAllStandardError();
      return -1;
    }
    return 0;
  }
} // namespace

class TestZoom : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void restoresInitialPreference();
    void clampsFinalStepAndConsumesLimits_data();
    void clampsFinalStepAndConsumesLimits();
    void normalizesLiveChanges_data();
    void normalizesLiveChanges();
    void synchronizesExistingAndNewBrowsers();
    void preservesZoomAcrossDocumentLoads();
    void reappliesTextFontWithoutChangingPreference();
    void keyboardRoutingWithViewportFilter();
    void persistsAcrossRestart();
    void validatesStoredPreference_data();
    void validatesStoredPreference();
    void observesEngineZoomChanges();
};

void TestZoom::initTestCase() {
  QCOMPARE(qApp->web()->zoomFactor(), initial_zoom);
}

void TestZoom::init() {
  qApp->web()->setZoomFactor(initial_zoom);
}

void TestZoom::restoresInitialPreference() {
  QSignalSpy changed(qApp->web(), &WebFactory::zoomFactorChanged);
  auto browser = createBrowser();
  QCOMPARE(browser->viewer()->zoomFactor(), initial_zoom);
  QCOMPARE(qApp->settings()->value(GROUP(Messages), SETTING(Messages::Zoom)).toDouble(), initial_zoom);
  QCOMPARE(changed.count(), 0); // Initialization must not save the viewer's default 100%.
}

void TestZoom::clampsFinalStepAndConsumesLimits_data() {
  QTest::addColumn<bool>("wheel");
  QTest::addColumn<bool>("increase");
  QTest::newRow("wheel-upper") << true << true;
  QTest::newRow("wheel-lower") << true << false;
  QTest::newRow("keyboard-upper") << false << true;
  QTest::newRow("keyboard-lower") << false << false;
}

void TestZoom::clampsFinalStepAndConsumesLimits() {
  QFETCH(bool, wheel);
  QFETCH(bool, increase);
  auto browser = createBrowser();
  QVERIFY(initializeInputReceiver(*browser));
  const qreal limit = increase ? MAX_ZOOM_FACTOR : MIN_ZOOM_FACTOR;
  browser->viewer()->setZoomFactor(limit + (increase ? -0.001 : 0.001));

  sendZoomInput(*browser, wheel, increase);
  QCOMPARE(browser->viewer()->zoomFactor(), limit);
  QCOMPARE(qApp->web()->zoomFactor(), limit);
  QVERIFY(increase ? !browser->viewer()->canZoomIn() : !browser->viewer()->canZoomOut());

  auto* widget = dynamic_cast<QWidget*>(browser->viewer());
  const QFont font_at_limit = widget->font();
  QSignalSpy changed(qApp->web(), &WebFactory::zoomFactorChanged);
  for (int i = 0; i < 3; ++i) {
    sendZoomInput(*browser, wheel, increase);
  }
  QCOMPARE(browser->viewer()->zoomFactor(), limit);
  QCOMPARE(widget->font(), font_at_limit); // Native QTextEdit zoom must not bypass the rejected command.
  QCOMPARE(changed.count(), 0);
}

void TestZoom::synchronizesExistingAndNewBrowsers() {
  auto first = createBrowser();
  auto second = createBrowser();
  QSignalSpy changed(qApp->web(), &WebFactory::zoomFactorChanged);

  first->viewer()->setZoomFactor(1.75);
  QCOMPARE(second->viewer()->zoomFactor(), qreal(1.75));
  QCOMPARE(qApp->web()->zoomFactor(), qreal(1.75));
  QCOMPARE(changed.count(), 1);

  second->viewer()->zoomIn();
  QCOMPARE(first->viewer()->zoomFactor(), second->viewer()->zoomFactor());
  QCOMPARE(qApp->web()->zoomFactor(), second->viewer()->zoomFactor());
  QCOMPARE(changed.count(), 2);

  auto third = createBrowser();
  QCOMPARE(third->viewer()->zoomFactor(), first->viewer()->zoomFactor());
  QCOMPARE(changed.count(), 2);
  first.reset(); // Destroyed subscribers must not obstruct the next shared update.
  third->viewer()->setZoomFactor(2.0);
  QCOMPARE(second->viewer()->zoomFactor(), qreal(2.0));
  QCOMPARE(changed.count(), 3);
  QCOMPARE(qApp->settings()->value(GROUP(Messages), SETTING(Messages::Zoom)).toDouble(), qreal(2.0));
}

void TestZoom::normalizesLiveChanges_data() {
  QTest::addColumn<qreal>("requested");
  QTest::addColumn<qreal>("expected");
  QTest::newRow("nan") << std::numeric_limits<qreal>::quiet_NaN() << qreal(DEFAULT_ZOOM_FACTOR);
  QTest::newRow("infinity") << std::numeric_limits<qreal>::infinity() << qreal(DEFAULT_ZOOM_FACTOR);
  QTest::newRow("below-minimum") << qreal(-1.0) << qreal(MIN_ZOOM_FACTOR);
  QTest::newRow("above-maximum") << qreal(25.0) << qreal(MAX_ZOOM_FACTOR);
}

void TestZoom::normalizesLiveChanges() {
  QFETCH(qreal, requested);
  QFETCH(qreal, expected);
  auto browser = createBrowser();
  QSignalSpy changed(qApp->web(), &WebFactory::zoomFactorChanged);
  browser->viewer()->setZoomFactor(requested);
  QCOMPARE(browser->viewer()->zoomFactor(), expected);
  QCOMPARE(qApp->web()->zoomFactor(), expected);
  QCOMPARE(changed.count(), 1);
  browser->viewer()->setZoomFactor(requested);
  QCOMPARE(changed.count(), 1);
}

void TestZoom::preservesZoomAcrossDocumentLoads() {
  auto browser = createBrowser();
  QSignalSpy loaded(dynamic_cast<QObject*>(browser->viewer()), SIGNAL(loadingFinished(bool)));
  browser->viewer()->setZoomFactor(1.75);
  QSignalSpy changed(qApp->web(), &WebFactory::zoomFactorChanged);

  browser->setHtml(QSL("<p>First article</p>"));
  QVERIFY(waitForLoad(loaded));
  QVERIFY(loaded.last().at(0).toBool());
  QCOMPARE(browser->viewer()->zoomFactor(), qreal(1.75));

  loaded.clear();
  Message message;
  message.m_title = QSL("Another article");
  message.m_contents = QSL("<p>Article content</p>");
  browser->loadMessage(message, nullptr, nullptr);
  QVERIFY(waitForLoad(loaded));
  QVERIFY(loaded.last().at(0).toBool());
  QCOMPARE(browser->viewer()->zoomFactor(), qreal(1.75));

  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  QFile article(directory.filePath(QSL("article.html")));
  QVERIFY(article.open(QIODevice::WriteOnly));
  article.write("<!doctype html><html><body>Direct local article</body></html>");
  article.close();
  loaded.clear();
  browser->loadArticleUrl(QUrl::fromLocalFile(article.fileName()));
  QVERIFY(waitForLoad(loaded));
  QVERIFY(loaded.last().at(0).toBool());
  QCOMPARE(browser->viewer()->zoomFactor(), qreal(1.75));
  QCOMPARE(changed.count(), 0); // Navigation must not reinterpret restoration as a user preference change.
}

void TestZoom::reappliesTextFontWithoutChangingPreference() {
  auto browser = createBrowser();
  auto* text = dynamic_cast<TextBrowserViewer*>(browser->viewer());
  if (text == nullptr) {
    QSKIP("Text document rendering is tested in the text variant.");
  }
  QFont base_font = text->font();
  base_font.setPointSizeF(12.0);
  text->applyFont(base_font);
  text->setZoomFactor(1.75);
  QSignalSpy changed(qApp->web(), &WebFactory::zoomFactorChanged);
  QSignalSpy viewer_changed(text, SIGNAL(viewerZoomFactorChanged(qreal)));
  QVERIFY(viewer_changed.isValid());

  // A document rebuild must apply the factor even when the tracked value did not change.
  text->QTextBrowser::setFont(base_font);
  browser->setHtml(QSL("<p>Rebuilt article</p>"));
  QCOMPARE(text->font().pointSizeF(), qreal(21.0));
  text->setLoadExternalResources(!text->loadExternalResources());
  QCOMPARE(text->font().pointSizeF(), qreal(21.0));
  base_font.setPointSizeF(14.0);
  text->applyFont(base_font);
  QCOMPARE(text->font().pointSizeF(), qreal(24.5));
  QCOMPARE(text->zoomFactor(), qreal(1.75));
  QCOMPARE(viewer_changed.count(), 0);
  QCOMPARE(changed.count(), 0);
}

void TestZoom::keyboardRoutingWithViewportFilter() {
  auto browser = createBrowser();
  auto* text = dynamic_cast<TextBrowserViewer*>(browser->viewer());
  if (text == nullptr) {
    QSKIP("Viewport keyboard propagation applies to QTextBrowser.");
  }
  browser->resize(600, 300);
  browser->show();
  auto* search = browser->findChild<QWidget*>(QSL("SearchTextWidget"));
  QVERIFY(search != nullptr);
  QVERIFY(!search->isVisible());
  QTest::keyClick(text, Qt::Key_F, Qt::ControlModifier);
  QVERIFY(search->isVisible());
  QTest::keyClick(search->focusProxy(), Qt::Key_Escape);
  QVERIFY(!search->isVisible());
  QTest::keyClick(text, Qt::Key_Plus, Qt::ControlModifier);
  QVERIFY(text->zoomFactor() > initial_zoom);
  QTest::keyClick(text, Qt::Key_0, Qt::ControlModifier);
  QCOMPARE(text->zoomFactor(), qreal(DEFAULT_ZOOM_FACTOR));
  QCOMPARE(qApp->web()->zoomFactor(), qreal(DEFAULT_ZOOM_FACTOR));
}

void TestZoom::persistsAcrossRestart() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  auto browser = createBrowser();
  browser->viewer()->setZoomFactor(1.75);
  qApp->settings()->sync();
  QVERIFY(QDir().mkpath(directory.path() + QDir::separator() + QSL(APP_CFG_PATH)));
  QVERIFY(QFile::copy(qApp->settings()->fileName(), settingsPath(directory.path())));
  QCOMPARE(probeRestoredZoom(directory.path(), 1.75, QSL("saved preference")), 0);
}

void TestZoom::validatesStoredPreference_data() {
  QTest::addColumn<QVariant>("stored");
  QTest::addColumn<qreal>("expected");
  QTest::newRow("invalid-text") << QVariant(QSL("not-a-number")) << qreal(DEFAULT_ZOOM_FACTOR);
  QTest::newRow("nan") << QVariant(std::numeric_limits<qreal>::quiet_NaN()) << qreal(DEFAULT_ZOOM_FACTOR);
  QTest::newRow("positive-infinity") << QVariant(std::numeric_limits<qreal>::infinity()) << qreal(DEFAULT_ZOOM_FACTOR);
  QTest::newRow("below-minimum") << QVariant(-1.0) << qreal(MIN_ZOOM_FACTOR);
  QTest::newRow("above-maximum") << QVariant(25.0) << qreal(MAX_ZOOM_FACTOR);
}

void TestZoom::validatesStoredPreference() {
  QFETCH(QVariant, stored);
  QFETCH(qreal, expected);
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  prepareProfile(directory.path(), stored);
  QCOMPARE(probeRestoredZoom(directory.path(), expected, QSL("validated preference")), 0);
}

void TestZoom::observesEngineZoomChanges() {
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
  auto browser = createBrowser();
  auto other = createBrowser();
  auto* engine = dynamic_cast<WebEngineViewer*>(browser->viewer());
  if (engine == nullptr) {
    QSKIP("Engine-originated zoom is tested in the web variant.");
  }
  QSignalSpy loaded(engine, SIGNAL(loadingFinished(bool)));
  QVERIFY(loaded.isValid());
  browser->setHtml(QSL("<p>Initialized engine page</p>"));
  QVERIFY(waitForLoad(loaded));
  QVERIFY(loaded.last().at(0).toBool());
  QSignalSpy changed(qApp->web(), &WebFactory::zoomFactorChanged);
  engine->QWebEngineView::page()->setZoomFactor(2.0);
  QCOMPARE(qApp->web()->zoomFactor(), qreal(2.0));
  QCOMPARE(other->viewer()->zoomFactor(), qreal(2.0));
  QCOMPARE(changed.count(), 1);
#else
  QSKIP("Qt before 6.8 has no engine zoom-factor notification; application setter coverage runs above.");
#endif
}

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  const bool restoring = argc == 4 && QByteArray(argv[1]) == "--zoom-restore-probe";
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  const QString profile = restoring ? QString::fromLocal8Bit(argv[2]) : directory.path();
  if (!restoring) {
    prepareProfile(profile, initial_zoom);
  }
  const qreal expected = restoring ? QString::fromLocal8Bit(argv[3]).toDouble() : initial_zoom;
  QStringList arguments{QString::fromLocal8Bit(argv[0]), QSL("--data"), profile, QSL("--no-single-instance")};
  if (!usesWebEngine()) {
    arguments.append(QSL("--force-text-browser"));
  }
  Application application(QSL("rssguard-zoom-test"), argc, argv, arguments);
  int result;
  if (restoring) {
    auto browser = createBrowser();
    QSignalSpy loaded(dynamic_cast<QObject*>(browser->viewer()), SIGNAL(loadingFinished(bool)));
    browser->setHtml(QSL("<p>Article after restart</p>"));
    result = waitForLoad(loaded) && loaded.last().at(0).toBool() &&
                 qFuzzyCompare(application.web()->zoomFactor(), expected) &&
                 qFuzzyCompare(browser->viewer()->zoomFactor(), expected)
               ? 0
               : 1;
  }
  else {
    TestZoom tests;
    result = QTest::qExec(&tests, argc, argv);
  }
  finishViewerTeardown();
  return result;
}

#include "test_zoom.moc"
