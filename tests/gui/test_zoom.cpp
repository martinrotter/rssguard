// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webbrowser.h"
#include "gui/webviewers/qtextbrowser/textbrowserimagehandler.h"
#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "network-web/webfactory.h"

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
#include "gui/webviewers/qtwebengine/webengineviewer.h"

#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineScript>
#include <QWebEngineSettings>
#endif

#include <algorithm>
#include <limits>
#include <memory>

#include <QAbstractTextDocumentLayout>
#include <QBuffer>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QImage>
#include <QKeyEvent>
#include <QPdfWriter>
#include <QProcess>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBlock>
#include <QTextImageFormat>
#include <QTimer>
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

  QString imageData(int width, int height) {
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::red);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return QSL("data:image/png;base64,") + QString::fromLatin1(bytes.toBase64());
  }

  struct ImageOccurrence {
      int position;
      QTextImageFormat format;
  };

  QList<ImageOccurrence> imageOccurrences(QTextDocument* document) {
    QList<ImageOccurrence> images;
    for (auto block = document->begin(); block.isValid(); block = block.next()) {
      for (auto fragment = block.begin(); !fragment.atEnd(); ++fragment) {
        const auto image = fragment.fragment();
        if (image.charFormat().isImageFormat()) {
          for (int i = 0; i < image.length(); ++i) {
            images.append({image.position() + i, image.charFormat().toImageFormat()});
          }
        }
      }
    }
    return images;
  }

  QSizeF imageGeometry(TextBrowserViewer* viewer, const ImageOccurrence& image) {
    auto* handler = viewer->document()->documentLayout()->handlerForObject(QTextFormat::ImageObject);
    return handler == nullptr ? QSizeF() : handler->intrinsicSize(viewer->document(), image.position, image.format);
  }

  bool nearSize(const QSizeF& actual, const QSizeF& expected, qreal tolerance = 1.0) {
    return qAbs(actual.width() - expected.width()) <= tolerance &&
           qAbs(actual.height() - expected.height()) <= tolerance;
  }

  void serveDelayedImage(QTcpServer& server, const QByteArray& png) {
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&server, png]() {
      while (server.hasPendingConnections()) {
        auto* socket = server.nextPendingConnection();
        const auto request = QSharedPointer<QByteArray>::create();
        QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket, request, png]() {
          request->append(socket->readAll());
          if (request->contains("\r\n\r\n")) {
            QObject::disconnect(socket, &QTcpSocket::readyRead, nullptr, nullptr);
            QTimer::singleShot(150, socket, [socket, png]() {
              socket->write("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: " +
                            QByteArray::number(png.size()) + "\r\nConnection: close\r\n\r\n" + png);
              socket->disconnectFromHost();
            });
          }
        });
        QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
      }
    });
  }

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  QVariant applicationScript(WebEngineViewer* viewer, const QString& script) {
    QEventLoop loop;
    const QPointer<QEventLoop> guard(&loop);
    const auto result = QSharedPointer<QVariant>::create();
    viewer->QWebEngineView::page()->runJavaScript(script,
                                                  QWebEngineScript::ApplicationWorld,
                                                  [guard, result](const QVariant& value) {
                                                    if (!guard.isNull()) {
                                                      *result = value;
                                                      guard->quit();
                                                    }
                                                  });
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    loop.exec();
    return *result;
  }

  QVariantMap webImageGeometries(WebEngineViewer* viewer) {
    return applicationScript(viewer,
                             QSL("(function(){var result={};Array.from(document.images).forEach(function(image){"
                                 "var rect=image.getBoundingClientRect();result[image.id]=[rect.width,rect.height,"
                                 "image.complete&&image.naturalWidth>0,image.getAttribute('data-rssguard-image-ready'),"
                                 "getComputedStyle(image).display];});return result;})()"))
      .toMap();
  }

  QSizeF webImageSize(const QVariantMap& images, const QString& id) {
    const auto geometry = images.value(id).toList();
    return geometry.size() < 2 ? QSizeF() : QSizeF(geometry.at(0).toDouble(), geometry.at(1).toDouble());
  }
#endif
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
    void preservesImageAttributesInSharedLimiter();
    void preservesDistinctImageLimitsForRepeatedResources();
    void zoomsAndCapsTextImages_data();
    void zoomsAndCapsTextImages();
    void preservesTextImageMetadataAndPrinterState();
    void restoresAdjacentTextImageMetadata();
    void preservesTextImageZoomDuringDelayedDownloads();
    void respectsHighDpiImageResources();
    void fitsZoomedTextImagesToPercentageMaximumWidth();
    void capsWebImagesWithoutPublisherJavascript_data();
    void capsWebImagesWithoutPublisherJavascript();
    void updatesWebImageCapsWithoutCompounding();
    void capsWebPrintImagesWithoutPublisherJavascript();
    void capsImagesAfterWebHistoryNavigation();
};

void TestZoom::initTestCase() {
  QCOMPARE(qApp->web()->zoomFactor(), initial_zoom);
}

void TestZoom::init() {
  qApp->web()->setZoomFactor(initial_zoom);
  qApp->settings()->setValue(GROUP(Messages), Messages::LimitArticleImagesHeight, 0);
  qApp->settings()->setValue(GROUP(Messages), Messages::LimitEnclosureImagesHeight, 72);
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

void TestZoom::preservesImageAttributesInSharedLimiter() {
  auto browser = createBrowser();
  qApp->settings()->setValue(GROUP(Messages), Messages::LimitArticleImagesHeight, 50);
  const QString html =
    QSL("<img src='same.png' width='40' height='30' alt='original alt' title='original title' "
        "style='display:none; width:40px; max-height:20px !important' data-rssguard-max-height='999'>");
  const QString limited = browser->viewer()->convertToHtmlWithLimitedImages(html);
  QVERIFY(limited.contains(QSL("width=\"40\"")));
  QVERIFY(limited.contains(QSL("height=\"30\"")));
  QVERIFY(limited.contains(QSL("style=\"display:none; width:40px; max-height:20px !important\"")));
  QVERIFY(limited.contains(QSL("alt=\"original alt\"")));
  QVERIFY(limited.contains(QSL("title=\"original title\"")));
  QCOMPARE(limited.count(QSL("data-rssguard-max-height=")), 1);
  QVERIFY(limited.contains(browser->viewer()->imageCssMaxHeight(50)));
  QVERIFY(browser->viewer()->imageCssMaxHeight(0).isEmpty());
  qApp->settings()->setValue(GROUP(Messages), Messages::LimitArticleImagesHeight, 0);
  QCOMPARE(browser->viewer()->convertToHtmlWithLimitedImages(html), html);
}

void TestZoom::preservesDistinctImageLimitsForRepeatedResources() {
  auto browser = createBrowser();
  browser->resize(800, 600);
  browser->show();
  QCoreApplication::processEvents();
  browser->viewer()->setLoadExternalResources(true);
  browser->viewer()->setZoomFactor(1.0);
  qApp->settings()->setValue(GROUP(Messages), Messages::LimitArticleImagesHeight, 80);
  qApp->settings()->setValue(GROUP(Messages), Messages::LimitEnclosureImagesHeight, 25);
  qApp->settings()->setValue(GROUP(Messages), Messages::DisplayEnclosuresInMessage, true);
  const QString resource = imageData(400, 200);
  Message message;
  message.m_contents = QSL("<p><img src='%1' alt='article image'></p>").arg(resource);
  message.m_enclosures.append(QSharedPointer<MessageEnclosure>::create(resource, QSL("image/png")));
  const QString html = browser->viewer()->htmlForMessage(message, nullptr, nullptr);
  const QRegularExpression image_tag(QSL("<img\\b[^>]*>"));
  auto tags = image_tag.globalMatch(html);
  QList<int> limits;
  while (tags.hasNext()) {
    const QString tag = tags.next().captured();
    if (tag.contains(resource)) {
      const auto limit = QRegularExpression(QSL("data-rssguard-max-height=\"([0-9]+)\"")).match(tag);
      QVERIFY(limit.hasMatch());
      limits.append(limit.captured(1).toInt());
    }
  }
  std::sort(limits.begin(), limits.end());
  QCOMPARE(limits, QList<int>({25, 80}));

  QSignalSpy loaded(dynamic_cast<QObject*>(browser->viewer()), SIGNAL(loadingFinished(bool)));
  browser->setHtml(html);
  QVERIFY(waitForLoad(loaded));
  if (auto* text = dynamic_cast<TextBrowserViewer*>(browser->viewer())) {
    const auto images = imageOccurrences(text->document());
    QCOMPARE(images.size(), 2);
    for (const auto& image : images) {
      const int limit = image.format.intProperty(TextBrowserImageHandler::MaximumHeightProperty);
      QVERIFY(limit == 25 || limit == 80);
      QVERIFY(nearSize(imageGeometry(text, image), QSizeF(limit * 2, limit)));
    }
  }
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  else if (auto* engine = dynamic_cast<WebEngineViewer*>(browser->viewer())) {
    const auto sizes = applicationScript(engine,
                                         QSL("Array.from(document.images).map(function(image){"
                                             "var rect=image.getBoundingClientRect();return ["
                                             "Number(image.getAttribute('data-rssguard-max-height')),"
                                             "rect.width,rect.height];})"))
                         .toList();
    QCOMPARE(sizes.size(), 2);
    for (const auto& value : sizes) {
      const auto size = value.toList();
      const int limit = size.at(0).toInt();
      QVERIFY(limit == 25 || limit == 80);
      const QSizeF actual(size.at(1).toDouble(), size.at(2).toDouble());
      QVERIFY2(nearSize(actual, QSizeF(limit * 2, limit)),
               qPrintable(QSL("Limit %1, geometry %2×%3").arg(limit).arg(actual.width()).arg(actual.height())));
    }
  }
#endif
}

void TestZoom::zoomsAndCapsTextImages_data() {
  QTest::addColumn<int>("source_width");
  QTest::addColumn<int>("source_height");
  QTest::addColumn<QString>("dimensions");
  QTest::addColumn<int>("maximum_height");
  QTest::addColumn<QSizeF>("expected");
  QTest::newRow("natural") << 400 << 200 << QString() << 0 << QSizeF(400, 200);
  QTest::newRow("small-image-is-not-enlarged") << 20 << 10 << QString() << 50 << QSizeF(20, 10);
  QTest::newRow("natural-height-cap") << 400 << 200 << QString() << 50 << QSizeF(100, 50);
  QTest::newRow("width-only") << 400 << 200 << QSL("width='120'") << 0 << QSizeF(120, 60);
  QTest::newRow("height-only") << 400 << 200 << QSL("height='60'") << 0 << QSizeF(120, 60);
  QTest::newRow("authored-two-axes") << 400 << 200 << QSL("width='80' height='30'") << 0 << QSizeF(80, 30);
  QTest::newRow("width-and-cap") << 400 << 200 << QSL("width='120'") << 50 << QSizeF(100, 50);
  QTest::newRow("two-axes-and-cap") << 400 << 200 << QSL("width='80' height='30'") << 20 << QSizeF(80.0 * 2 / 3, 20);
}

void TestZoom::zoomsAndCapsTextImages() {
  auto browser = createBrowser();
  auto* text = dynamic_cast<TextBrowserViewer*>(browser->viewer());
  if (text == nullptr) {
    QSKIP("Native text-image geometry is tested in the text variant.");
  }
  QFETCH(int, source_width);
  QFETCH(int, source_height);
  QFETCH(QString, dimensions);
  QFETCH(int, maximum_height);
  QFETCH(QSizeF, expected);
  text->setLoadExternalResources(true);
  text->setZoomFactor(1.0);
  const QString html =
    QSL("<p><img src='%1' %2 %3></p>")
      .arg(imageData(source_width, source_height), dimensions, text->imageCssMaxHeight(maximum_height));
  browser->setHtml(html);
  auto images = imageOccurrences(text->document());
  QCOMPARE(images.size(), 1);
  QVERIFY(nearSize(imageGeometry(text, images.first()), expected));
  const auto original_format = images.first().format;
  text->setZoomFactor(2.0);
  QVERIFY(nearSize(imageGeometry(text, images.first()), expected * 2));
  QCOMPARE(imageOccurrences(text->document()).first().format, original_format);
  text->setZoomFactor(0.5);
  QVERIFY(nearSize(imageGeometry(text, images.first()), expected * 0.5));
  browser->setHtml(html); // Rebuilding the same document must not compound either zoom or the height cap.
  images = imageOccurrences(text->document());
  QCOMPARE(images.size(), 1);
  QVERIFY(nearSize(imageGeometry(text, images.first()), expected * 0.5));
  text->setLoadExternalResources(false);
  text->setLoadExternalResources(true);
  images = imageOccurrences(text->document());
  QCOMPARE(images.size(), 1);
  QVERIFY(nearSize(imageGeometry(text, images.first()), expected * 0.5));
}

void TestZoom::preservesTextImageMetadataAndPrinterState() {
  auto browser = createBrowser();
  auto* text = dynamic_cast<TextBrowserViewer*>(browser->viewer());
  if (text == nullptr) {
    QSKIP("Native text-image metadata and printing are tested in the text variant.");
  }
  text->setLoadExternalResources(true);
  text->setZoomFactor(1.75);
  const QString resource = imageData(400, 200);
  browser->setHtml(QSL("<a href='https://example.test/original'><img src='%1' width='40' height='30' "
                       "alt='original alt' title='original title' %2></a>")
                     .arg(resource, text->imageCssMaxHeight(20)));
  const auto images = imageOccurrences(text->document());
  QCOMPARE(images.size(), 1);
  const auto image = images.first();
  QCOMPARE(image.format.name(), resource);
  QCOMPARE(image.format.width(), qreal(40));
  QCOMPARE(image.format.height(), qreal(30));
  QCOMPARE(image.format.stringProperty(QTextFormat::ImageAltText), QSL("original alt"));
  QCOMPARE(image.format.stringProperty(QTextFormat::ImageTitle), QSL("original title"));
  QCOMPARE(image.format.anchorHref(), QSL("https://example.test/original"));
  QCOMPARE(image.format.intProperty(TextBrowserImageHandler::MaximumHeightProperty), 20);
  const QString exported = text->html();
  const QSizeF before_print = imageGeometry(text, image);
  auto* handler = text->document()->documentLayout()->handlerForObject(QTextFormat::ImageObject);
  QSignalSpy changed(qApp->web(), &WebFactory::zoomFactorChanged);
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString printed = directory.filePath(QSL("article.pdf"));
  {
    QPdfWriter writer(printed);
    writer.setResolution(96);
    text->QTextBrowser::print(&writer); // Exercise Qt's cloned print document without the application's printer UI.
  }
  QVERIFY(QFileInfo(printed).size() > 500);
  QCOMPARE(text->html(), exported);
  QCOMPARE(text->zoomFactor(), qreal(1.75));
  QCOMPARE(imageGeometry(text, image), before_print);
  QCOMPARE(text->document()->documentLayout()->handlerForObject(QTextFormat::ImageObject), handler);
  QCOMPARE(changed.count(), 0);
}

void TestZoom::restoresAdjacentTextImageMetadata() {
  auto browser = createBrowser();
  auto* text = dynamic_cast<TextBrowserViewer*>(browser->viewer());
  if (text == nullptr) {
    QSKIP("Adjacent QTextFragment merging is tested in the text variant.");
  }
  text->setLoadExternalResources(true);
  text->setZoomFactor(1.0);
  const QString image =
    QSL("<img src='%1' alt='same original alt' %2>").arg(imageData(400, 200), text->imageCssMaxHeight(50));
  browser->setHtml(QSL("<p>Příliš žluťoučký kůň %1%1%1</p>").arg(image));
  const auto images = imageOccurrences(text->document());
  QCOMPARE(images.size(), 3);
  for (const auto& occurrence : images) {
    QCOMPARE(occurrence.format.stringProperty(QTextFormat::ImageAltText), QSL("same original alt"));
    QCOMPARE(occurrence.format.intProperty(TextBrowserImageHandler::MaximumHeightProperty), 50);
    QVERIFY(nearSize(imageGeometry(text, occurrence), QSizeF(100, 50)));
  }
  QVERIFY(text->plainText().contains(QSL("Příliš žluťoučký kůň")));
  QVERIFY(!text->html().contains(QSL("rssguard-image-cap")));
}

void TestZoom::preservesTextImageZoomDuringDelayedDownloads() {
  auto browser = createBrowser();
  auto* text = dynamic_cast<TextBrowserViewer*>(browser->viewer());
  if (text == nullptr) {
    QSKIP("QTextBrowser's image downloader is tested in the text variant.");
  }
  QImage image(400, 200, QImage::Format_RGB32);
  image.fill(Qt::green);
  QByteArray png;
  QBuffer image_buffer(&png);
  QVERIFY(image_buffer.open(QIODevice::WriteOnly));
  QVERIFY(image.save(&image_buffer, "PNG"));
  QTcpServer server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  serveDelayedImage(server, png);
  text->setLoadExternalResources(true);
  text->setZoomFactor(1.0);
  QSignalSpy requested(&server, &QTcpServer::newConnection);
  QSignalSpy loaded(text, SIGNAL(loadingFinished(bool)));
  QVERIFY(loaded.isValid());
  browser->setHtml(QSL("<img src='http://127.0.0.1:%1/image.png' alt='original delayed image' %2>")
                     .arg(server.serverPort())
                     .arg(text->imageCssMaxHeight(50)));
  text->setZoomFactor(1.75);
  QVERIFY(waitForLoad(loaded));
  QVERIFY(loaded.last().at(0).toBool());
  QCOMPARE(requested.count(), 1);
  auto images = imageOccurrences(text->document());
  QCOMPARE(images.size(), 1);
  QCOMPARE(images.first().format.stringProperty(QTextFormat::ImageAltText), QSL("original delayed image"));
  QVERIFY(nearSize(imageGeometry(text, images.first()), QSizeF(175, 87.5)));
  QCOMPARE(text->zoomFactor(), qreal(1.75));
  text->setLoadExternalResources(false);
  QVERIFY(imageOccurrences(text->document()).isEmpty());
  QVERIFY(text->plainText().contains(QSL("original delayed image")));
  loaded.clear();
  text->setLoadExternalResources(true);
  QVERIFY(waitForLoad(loaded));
  images = imageOccurrences(text->document());
  QCOMPARE(images.size(), 1);
  QCOMPARE(images.first().format.stringProperty(QTextFormat::ImageAltText), QSL("original delayed image"));
  QVERIFY(nearSize(imageGeometry(text, images.first()), QSizeF(175, 87.5)));
  QCOMPARE(text->zoomFactor(), qreal(1.75));
  QCOMPARE(requested.count(), 1); // The downloaded cache remains usable after toggling resource visibility.
}

void TestZoom::respectsHighDpiImageResources() {
  auto browser = createBrowser();
  auto* text = dynamic_cast<TextBrowserViewer*>(browser->viewer());
  if (text == nullptr) {
    QSKIP("QImage device-pixel-ratio handling is tested in the text variant.");
  }
  text->setLoadExternalResources(true);
  text->setZoomFactor(1.0);
  const QUrl resource(QSL("qrc:/zoom-test-dpr-image"));
  browser->setHtml(QSL("<img src='%1' %2>").arg(resource.toString(), text->imageCssMaxHeight(75)));
  QImage image(400, 200, QImage::Format_RGB32);
  image.setDevicePixelRatio(2.0);
  image.fill(Qt::blue);
  text->document()->addResource(QTextDocument::ImageResource, resource, image);
  text->document()->markContentsDirty(0, text->document()->characterCount());
  const auto images = imageOccurrences(text->document());
  QCOMPARE(images.size(), 1);
  QVERIFY(nearSize(imageGeometry(text, images.first()), QSizeF(150, 75)));
  QCOMPARE(text->document()->resource(QTextDocument::ImageResource, resource).value<QImage>().devicePixelRatio(),
           qreal(2));
  text->setZoomFactor(2.0);
  QVERIFY(nearSize(imageGeometry(text, images.first()), QSizeF(300, 150)));
}

void TestZoom::fitsZoomedTextImagesToPercentageMaximumWidth() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
  auto browser = createBrowser();
  auto* text = dynamic_cast<TextBrowserViewer*>(browser->viewer());
  if (text == nullptr) {
    QSKIP("QTextImageFormat percentage widths are tested in the text variant.");
  }
  browser->resize(520, 400);
  browser->show();
  QCoreApplication::processEvents();
  text->setLoadExternalResources(true);
  text->setZoomFactor(1.0);
  browser->setHtml(QSL("<style>img{max-width:50%;}</style><img src='%1'>").arg(imageData(400, 200)));
  const auto images = imageOccurrences(text->document());
  QCOMPARE(images.size(), 1);
  QVERIFY(images.first().format.hasProperty(QTextFormat::ImageMaxWidth));
  const QSizeF fitted = imageGeometry(text, images.first());
  const qreal available_width = text->document()->pageSize().width() - 2 * text->document()->documentMargin();
  QVERIFY(fitted.width() > 0 && fitted.width() <= available_width / 2 + 1);
  QVERIFY(nearSize(fitted, QSizeF(fitted.width(), fitted.width() / 2)));
  text->setZoomFactor(2.0);
  QVERIFY(nearSize(imageGeometry(text, images.first()), fitted));
  text->setZoomFactor(0.5);
  QVERIFY(nearSize(imageGeometry(text, images.first()), QSizeF(200, 100)));
#else
  QSKIP("Qt 5's native rich-text image format does not support percentage maximum widths.");
#endif
}

void TestZoom::capsWebImagesWithoutPublisherJavascript_data() {
  QTest::addColumn<bool>("javascript_enabled");
  QTest::newRow("publisher-javascript-enabled") << true;
  QTest::newRow("publisher-javascript-disabled") << false;
}

void TestZoom::capsWebImagesWithoutPublisherJavascript() {
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  auto browser = createBrowser();
  auto* engine = dynamic_cast<WebEngineViewer*>(browser->viewer());
  if (engine == nullptr) {
    QSKIP("Web DOM image sizing is tested in the web variant.");
  }
  QFETCH(bool, javascript_enabled);
  engine->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, javascript_enabled);
  engine->setLoadExternalResources(true);
  engine->setZoomFactor(1.0);
  browser->resize(800, 600);
  browser->show();
  const QString large = imageData(400, 200);
  const QString small = imageData(20, 10);
  const QString html =
    QSL("<!doctype html><html><body style='margin:0'>"
        "<img id='natural' src='%1' data-rssguard-max-height='40'>"
        "<img id='small' src='%2' data-rssguard-max-height='40'>"
        "<img id='both' src='%1' width='40' height='30' data-rssguard-max-height='20'>"
        "<img id='width' src='%1' width='120' data-rssguard-max-height='40'>"
        "<img id='height' src='%1' height='120' data-rssguard-max-height='40'>"
        "<img id='stricter' src='%1' style='width:120px;height:80px;max-height:15px !important' "
        "data-rssguard-max-height='40'>"
        "<img id='important' src='%1' style='width:120px !important;height:80px !important;"
        "border:2px solid red;float:left;opacity:0.4' "
        "data-rssguard-max-height='40'>"
        "<img id='minimum' src='%1' style='width:120px;height:80px;min-width:240px;min-height:160px;"
        "max-height:30px' data-rssguard-max-height='40'>"
        "<img id='hidden' src='%1' style='display:none;width:120px;height:80px' data-rssguard-max-height='40'>"
        "<script>document.body.setAttribute('data-publisher-ran','yes');</script></body></html>")
      .arg(large, small);
  QSignalSpy loaded(engine, SIGNAL(loadingFinished(bool)));
  browser->setHtml(html);
  QVERIFY(waitForLoad(loaded));
  QTRY_VERIFY_WITH_TIMEOUT(webImageGeometries(engine).value(QSL("natural")).toList().value(3).isValid(), 10000);
  const auto images = webImageGeometries(engine);
  QCOMPARE(images.size(), 9);
  QVERIFY(nearSize(webImageSize(images, QSL("natural")), QSizeF(80, 40)));
  QVERIFY(nearSize(webImageSize(images, QSL("small")), QSizeF(20, 10)));
  QVERIFY(nearSize(webImageSize(images, QSL("both")), QSizeF(40.0 * 2 / 3, 20)));
  QVERIFY(nearSize(webImageSize(images, QSL("width")), QSizeF(80, 40)));
  QVERIFY(nearSize(webImageSize(images, QSL("height")), QSizeF(80, 40)));
  QVERIFY(nearSize(webImageSize(images, QSL("stricter")), QSizeF(120, 15)));
  QVERIFY(nearSize(webImageSize(images, QSL("important")), QSizeF(64, 44))); // Include the unchanged 2px border.
  QVERIFY(nearSize(webImageSize(images, QSL("minimum")), QSizeF(45, 30)));
  QCOMPARE(applicationScript(engine,
                             QSL("(function(){var style=getComputedStyle(document.getElementById('important'));"
                                 "return [style.borderTopWidth,style.cssFloat,style.opacity];})()"))
             .toList(),
           QVariantList({QSL("2px"), QSL("left"), QSL("0.4")}));
  QCOMPARE(images.value(QSL("hidden")).toList().value(4).toString(), QSL("none"));
  QCOMPARE(webImageSize(images, QSL("hidden")), QSizeF(0, 0));
  QCOMPARE(applicationScript(engine, QSL("document.body.getAttribute('data-publisher-ran')")).toString(),
           javascript_enabled ? QSL("yes") : QString());
  const auto metadata =
    applicationScript(engine,
                      QSL("[document.getElementById('both').getAttribute('width'),"
                          "document.getElementById('both').getAttribute('height'),"
                          "document.getElementById('stricter').style.getPropertyValue('max-height'),"
                          "document.getElementById('stricter').style.getPropertyPriority('max-height')]"))
      .toList();
  QCOMPARE(metadata, QVariantList({QSL("40"), QSL("30"), QSL("15px"), QSL("important")}));
  engine->setZoomFactor(2.0);
  QTRY_VERIFY_WITH_TIMEOUT(nearSize(webImageSize(webImageGeometries(engine), QSL("natural")), QSizeF(80, 40)), 10000);
  QCOMPARE(engine->zoomFactor(), qreal(2.0)); // Chromium supplies the physical image enlargement through page zoom.
#else
  QSKIP("WebEngine is not available in this build.");
#endif
}

void TestZoom::updatesWebImageCapsWithoutCompounding() {
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  auto browser = createBrowser();
  auto* engine = dynamic_cast<WebEngineViewer*>(browser->viewer());
  if (engine == nullptr) {
    QSKIP("Responsive web image sizing is tested in the web variant.");
  }
  engine->setLoadExternalResources(true);
  engine->setZoomFactor(1.0);
  browser->resize(800, 600);
  browser->show();
  QSignalSpy loaded(engine, SIGNAL(loadingFinished(bool)));
  browser
    ->setHtml(QSL("<!doctype html><html><body style='margin:0'>"
                  "<img id='dynamic' src='%1' style='width:240px !important;height:180px !important' "
                  "data-rssguard-max-height='60'>"
                  "<img id='revealed' src='%1' style='display:none;width:120px;height:80px' "
                  "data-rssguard-max-height='40'>"
                  "<div id='container' style='width:50%'><img id='responsive' src='%1' style='width:100%;height:auto' "
                  "data-rssguard-max-height='60'></div></body></html>")
                .arg(imageData(400, 200)));
  QVERIFY(waitForLoad(loaded));
  QTRY_VERIFY_WITH_TIMEOUT(nearSize(webImageSize(webImageGeometries(engine), QSL("dynamic")), QSizeF(80, 60)), 10000);
  applicationScript(engine,
                    QSL("document.getElementById('dynamic').setAttribute('data-rssguard-max-height','90');"
                        "window.__rssguardImageLimits.update();"));
  QVERIFY(nearSize(webImageSize(webImageGeometries(engine), QSL("dynamic")), QSizeF(120, 90)));
  applicationScript(engine,
                    QSL("document.getElementById('dynamic').removeAttribute('data-rssguard-max-height');"
                        "window.__rssguardImageLimits.update();"));
  QVERIFY(nearSize(webImageSize(webImageGeometries(engine), QSL("dynamic")), QSizeF(240, 180)));
  const auto restored =
    applicationScript(engine,
                      QSL("[document.getElementById('dynamic').style.getPropertyValue('width'),"
                          "document.getElementById('dynamic').style.getPropertyPriority('width'),"
                          "document.getElementById('dynamic').style.getPropertyValue('height'),"
                          "document.getElementById('dynamic').style.getPropertyPriority('height')]"))
      .toList();
  QCOMPARE(restored, QVariantList({QSL("240px"), QSL("important"), QSL("180px"), QSL("important")}));
  applicationScript(engine,
                    QSL("document.getElementById('dynamic').setAttribute('data-rssguard-max-height','60');"
                        "window.__rssguardImageLimits.update();"
                        "document.getElementById('dynamic').style.width='300px';"
                        "document.getElementById('dynamic').style.height='150px';"));
  QTRY_VERIFY_WITH_TIMEOUT(nearSize(webImageSize(webImageGeometries(engine), QSL("dynamic")), QSizeF(120, 60)), 10000);
  applicationScript(engine,
                    QSL("document.getElementById('dynamic').removeAttribute('data-rssguard-max-height');"
                        "window.__rssguardImageLimits.update();"));
  QVERIFY(nearSize(webImageSize(webImageGeometries(engine), QSL("dynamic")), QSizeF(300, 150)));
  applicationScript(engine, QSL("document.getElementById('revealed').style.display='block';"));
  QTRY_VERIFY_WITH_TIMEOUT(nearSize(webImageSize(webImageGeometries(engine), QSL("revealed")), QSizeF(60, 40)), 10000);
  const QSizeF large_viewport = webImageSize(webImageGeometries(engine), QSL("responsive"));
  QVERIFY(nearSize(large_viewport, QSizeF(120, 60)));
  applicationScript(engine, QSL("document.getElementById('container').style.width='80px';"));
  QTRY_VERIFY_WITH_TIMEOUT(nearSize(webImageSize(webImageGeometries(engine), QSL("responsive")), QSizeF(80, 40)),
                           10000);
  applicationScript(engine, QSL("document.getElementById('container').style.width='50%';"));
  QTRY_VERIFY_WITH_TIMEOUT(nearSize(webImageSize(webImageGeometries(engine), QSL("responsive")), large_viewport),
                           10000);
  browser->resize(200, 600);
  QTRY_VERIFY_WITH_TIMEOUT(webImageSize(webImageGeometries(engine), QSL("responsive")).height() < 59, 10000);
  const QSizeF small_viewport = webImageSize(webImageGeometries(engine), QSL("responsive"));
  QVERIFY(nearSize(small_viewport, QSizeF(small_viewport.width(), small_viewport.width() / 2)));
  browser->resize(800, 600);
  QTRY_VERIFY_WITH_TIMEOUT(nearSize(webImageSize(webImageGeometries(engine), QSL("responsive")), large_viewport),
                           10000);
#else
  QSKIP("WebEngine is not available in this build.");
#endif
}

void TestZoom::capsWebPrintImagesWithoutPublisherJavascript() {
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  auto browser = createBrowser();
  auto* engine = dynamic_cast<WebEngineViewer*>(browser->viewer());
  if (engine == nullptr) {
    QSKIP("Web printing is tested in the web variant.");
  }
  engine->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
  engine->setLoadExternalResources(true);
  engine->setZoomFactor(1.0);
  browser->resize(800, 600);
  browser->show();
  QSignalSpy loaded(engine, SIGNAL(loadingFinished(bool)));
  browser->setHtml(QSL("<!doctype html><html><head><style>@media print{"
                       "#printed{width:240px !important;height:240px !important}}"
                       "</style></head><body><img id='printed' src='%1' style='width:120px;height:80px' "
                       "data-rssguard-max-height='40'></body></html>")
                     .arg(imageData(400, 200)));
  QVERIFY(waitForLoad(loaded));
  QVERIFY(nearSize(webImageSize(webImageGeometries(engine), QSL("printed")), QSizeF(60, 40)));
  applicationScript(engine,
                    QSL("window.__rssguardPrintTrace=[];"
                        "function recordImagePrint(stage){var image=document.getElementById('printed');"
                        "var rect=image.getBoundingClientRect(),style=getComputedStyle(image);"
                        "window.__rssguardPrintTrace.push([stage,matchMedia('print').matches,rect.width,rect.height,"
                        "style.width,style.height]);"
                        "if(matchMedia('print').matches)window.__rssguardPrintSize=[rect.width,rect.height];}"
                        "window.addEventListener('beforeprint',function(){"
                        "recordImagePrint('beforeprint');});"
                        "matchMedia('print').addListener(function(){recordImagePrint('media');});"));
  QEventLoop loop;
  const QPointer<QEventLoop> guard(&loop);
  const auto pdf = QSharedPointer<QByteArray>::create();
  engine->QWebEngineView::page()->printToPdf([guard, pdf](const QByteArray& bytes) {
    if (!guard.isNull()) {
      *pdf = bytes;
      guard->quit();
    }
  });
  QTimer::singleShot(30000, &loop, &QEventLoop::quit);
  loop.exec();
  QVERIFY(pdf->startsWith("%PDF"));
  QFile output(QCoreApplication::applicationDirPath() + QSL("/image-height-print.pdf"));
  QVERIFY(output.open(QIODevice::WriteOnly));
  QCOMPARE(output.write(*pdf), qint64(pdf->size()));
  output.close();
  const auto printed = applicationScript(engine, QSL("window.__rssguardPrintSize")).toList();
  QCOMPARE(printed.size(), 2);
  const QSizeF actual(printed.at(0).toDouble(), printed.at(1).toDouble());
  QVERIFY2(nearSize(actual, QSizeF(40, 40)),
           qPrintable(QSL("Print geometry %1×%2, trace %3")
                        .arg(actual.width())
                        .arg(actual.height())
                        .arg(applicationScript(engine, QSL("JSON.stringify(window.__rssguardPrintTrace)"))
                               .toString())));
  QTRY_VERIFY_WITH_TIMEOUT(nearSize(webImageSize(webImageGeometries(engine), QSL("printed")), QSizeF(60, 40)), 10000);
#else
  QSKIP("WebEngine is not available in this build.");
#endif
}

void TestZoom::capsImagesAfterWebHistoryNavigation() {
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  auto browser = createBrowser();
  auto* engine = dynamic_cast<WebEngineViewer*>(browser->viewer());
  if (engine == nullptr) {
    QSKIP("Web history navigation is tested in the web variant.");
  }
  engine->setLoadExternalResources(true);
  engine->setZoomFactor(1.0);
  browser->resize(800, 600);
  browser->show();
  QSignalSpy loaded(engine, SIGNAL(loadingFinished(bool)));
  browser->setHtml(QSL("<!doctype html><html><body><img id='history' src='%1' "
                       "data-rssguard-max-height='40'></body></html>")
                     .arg(imageData(400, 200)));
  QVERIFY(waitForLoad(loaded));
  QVERIFY(nearSize(webImageSize(webImageGeometries(engine), QSL("history")), QSizeF(80, 40)));
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  QFile other(directory.filePath(QSL("unmarked.html")));
  QVERIFY(other.open(QIODevice::WriteOnly));
  other.write("<!doctype html><html><body>Unmarked document</body></html>");
  other.close();
  loaded.clear();
  browser->loadUrl(QUrl::fromLocalFile(other.fileName()));
  QVERIFY(waitForLoad(loaded));
  QVERIFY(engine->history()->canGoBack());
  loaded.clear();
  browser->goBack();
  QVERIFY(waitForLoad(loaded));
  QVERIFY(nearSize(webImageSize(webImageGeometries(engine), QSL("history")), QSizeF(80, 40)));
  QVERIFY(applicationScript(engine, QSL("!!window.__rssguardImageLimits")).toBool());
  QCOMPARE(applicationScript(engine, QSL("getComputedStyle(document.getElementById('history')).visibility")).toString(),
           QSL("visible"));
  loaded.clear();
  browser->reloadPage();
  QVERIFY(waitForLoad(loaded));
  QVERIFY(nearSize(webImageSize(webImageGeometries(engine), QSL("history")), QSizeF(80, 40)));
  QVERIFY(applicationScript(engine, QSL("!!window.__rssguardImageLimits")).toBool());
#else
  QSKIP("WebEngine is not available in this build.");
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
