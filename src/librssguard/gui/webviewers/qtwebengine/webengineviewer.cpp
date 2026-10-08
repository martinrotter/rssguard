// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtwebengine/webengineviewer.h"

#include "definitions/definitions.h"
#include "dynamic-shortcuts/shortcutpriority.h"
#include "gui/dialogs/filedialog.h"
#include "gui/reusable/scrollablemenu.h"
#include "gui/webbrowser.h"
#include "gui/webviewers/qtwebengine/webenginepage.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/memorydiagnostics.h"
#include "miscellaneous/skinfactory.h"
#include "network-web/webfactory.h"

#include <cmath>
#include <utility>

#include <QAction>
#include <QFile>
#include <QFileIconProvider>
#include <QGraphicsView>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>
#include <QToolTip>
#include <QWheelEvent>

#if QT_VERSION_MAJOR >= 6
#include <QWebEngineContextMenuRequest>
#else
#include <QWebEngineContextMenuData>
#endif

#include <QWebEngineHistory>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineSettings>

WebEngineViewer::WebEngineViewer(QWidget* parent)
  : QWebEngineView(parent), m_browser(nullptr),
    m_actionPrintToPdf(new QAction(qApp->icons()->fromTheme(QSL("document-print")), tr("Print to PDF"), this)),
    m_actionSaveFullPage(new QAction(qApp->icons()->fromTheme(QSL("document-save-as"), QSL("download")),
                                     tr("Save complete webpage"),
                                     this)),
    m_actionDiagGpu(new QAction(qApp->icons()->fromTheme(QSL("video-display"), QSL("dialog-information")),
                                tr("GPU"),
                                this)) {
  // Chromium's focused child reports whether an HTML text input is editable.
  ShortcutPriority::registerInputMethodEditor(this);
  WebEnginePage* page = new WebEnginePage(false, this);

  setPage(page);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
  connect(page, &QWebEnginePage::zoomFactorChanged, this, [this](qreal) {
    notifyZoomFactorChanged();
  });
#endif
  connect(this, &WebEngineViewer::loadStarted, this, [this]() {
    MemoryDiagnostics::webLoadStarted();
    ++m_contentGeneration;
    m_imageSizeLimitsInitialized = false;
    m_html.clear();
    m_plainText.clear();
  });
  connect(this, &WebEngineViewer::loadFinished, this, [this](bool success) {
    MemoryDiagnostics::webLoadFinished(success);

    applyImageSizeLimits(
      [this, success]() {
        if (success) {
          cachePageContents();
        }
        emit loadingFinished(success);
      },
      true);
  });

  connect(m_actionPrintToPdf.data(), &QAction::triggered, this, &WebEngineViewer::printToPdf);
  connect(m_actionSaveFullPage.data(), &QAction::triggered, this, &WebEngineViewer::saveCompleteWebPage);
  connect(m_actionDiagGpu.data(), &QAction::triggered, this, [this]() {
    emit openUrlInNewTab(false, QUrl(QSL("chrome://gpu")));
  });

  WebEngineViewer::setLoadExternalResources(WebViewer::loadExternalResources());
}

bool WebEngineViewer::event(QEvent* event) {
  if (event->type() == QEvent::Type::ChildAdded) {
    QChildEvent* child_ev = static_cast<QChildEvent*>(event);
    QWidget* w = qobject_cast<QWidget*>(child_ev->child());

    if (w != nullptr && m_browser != nullptr) {
      w->installEventFilter(m_browser);
    }
  }

  return QWebEngineView::event(event);
}

QList<QAction*> WebEngineViewer::advancedActions() const {
  auto* act_rel = page()->action(QWebEnginePage::WebAction::ReloadAndBypassCache);
  auto* act_src = page()->action(QWebEnginePage::WebAction::ViewSource);

  act_rel->setText(tr("Reload (bypass cache)"));
  act_src->setText(tr("View source"));

  return QList<QAction*>{m_actionPrintToPdf.data(), m_actionSaveFullPage.data(), act_rel, act_src};
}

QList<QAction*> WebEngineViewer::diagActions() const {
  return QList<QAction*>{m_actionDiagGpu.data()};
}

WebEnginePage* WebEngineViewer::page() const {
  return qobject_cast<WebEnginePage*>(QWebEngineView::page());
}

void WebEngineViewer::loadMessage(const Message& message, RootItem* root, Feed* feed) {
  m_selectedItem = root;
  m_feed = feed;

  auto url = urlForMessage(message, root);
  auto html = htmlForMessage(message, root, feed);

  setHtml(html, url, root, feed);

  setVerticalScrollBarPosition(0.0);
}

void WebEngineViewer::loadUrl(const QUrl& url) {
  m_hasImageSizeLimits = false;
  if (url.isValid()) {
    QWebEngineView::load(url);
  }
  else {
    clear();
  }
}

void WebEngineViewer::clear() {
  bool previously_enabled = isEnabled();

  setEnabled(false);
  setHtml(QSL("<!DOCTYPE html><html><body</body></html>"));
  setEnabled(previously_enabled);
}

void WebEngineViewer::reloadPage() {
  QWebEngineView::reload();
}

void WebEngineViewer::goBack() {
  QWebEngineView::back();
}

void WebEngineViewer::goForward() {
  QWebEngineView::forward();
}

void WebEngineViewer::clearNavigationHistory() {
  history()->clear();
}

void WebEngineViewer::cleanupCache() {
  page()->profile()->clearAllVisitedLinks();
  page()->profile()->clearHttpCache();
}

void WebEngineViewer::cachePageContents() {
  const quint64 content_generation = m_contentGeneration;
  const QWeakPointer<bool> lifetime_guard = m_lifetimeGuard.toWeakRef();

  MemoryDiagnostics::webContentRequestStarted();
  page()->toHtml([this, lifetime_guard, content_generation](const QString& html) {
    const QSharedPointer<bool> guard = lifetime_guard.toStrongRef();
    const bool apply_result = !guard.isNull() && m_contentGeneration == content_generation;

    if (apply_result) {
      m_html = html;
    }

    MemoryDiagnostics::webContentRequestFinished(apply_result);
  });

  MemoryDiagnostics::webContentRequestStarted();
  page()->toPlainText([this, lifetime_guard, content_generation](const QString& text) {
    const QSharedPointer<bool> guard = lifetime_guard.toStrongRef();
    const bool apply_result = !guard.isNull() && m_contentGeneration == content_generation;

    if (apply_result) {
      m_plainText = text;
    }

    MemoryDiagnostics::webContentRequestFinished(apply_result);
  });
}

void WebEngineViewer::printToPrinter(QPrinter* printer) {
  const QSharedPointer<QPrinter> guarded_printer = currentPrinter();
  applyImageSizeLimits(
    [this, printer, guarded_printer]() {
      Q_UNUSED(guarded_printer)
      printPreparedPage(printer);
    },
    false,
    [this]() {
      onPrintingFinished(false);
    });
}

void WebEngineViewer::printPreparedPage(QPrinter* printer) {
#if QT_VERSION_MAJOR < 6
  const QWeakPointer<bool> lifetime_guard = m_lifetimeGuard.toWeakRef();
  const QSharedPointer<QPrinter> guarded_printer = currentPrinter();

  page()->print(printer, [this, lifetime_guard, guarded_printer](bool success) {
    Q_UNUSED(guarded_printer)

    const QSharedPointer<bool> guard = lifetime_guard.toStrongRef();

    if (!guard.isNull()) {
      scheduleImageSizeLimits();
      onPrintingFinished(success);
    }
  });
#else
  disconnect(m_printFinishedConnection);

  const QWeakPointer<bool> lifetime_guard = m_lifetimeGuard.toWeakRef();
  const QSharedPointer<QPrinter> guarded_printer = currentPrinter();

  m_printFinishedConnection =
    connect(this, &WebEngineViewer::printFinished, this, [this, lifetime_guard, guarded_printer](bool success) {
      Q_UNUSED(guarded_printer)

      const QSharedPointer<bool> guard = lifetime_guard.toStrongRef();

      if (!guard.isNull()) {
        disconnect(m_printFinishedConnection);
        m_printFinishedConnection = {};
        scheduleImageSizeLimits();
        onPrintingFinished(success);
      }
    });

  QWebEngineView::print(printer);
#endif
}

/*
bool WebEngineViewer::loadExternalResources() const {
  return page()->settings()->testAttribute(QWebEngineSettings::WebAttribute::AutoLoadImages);
}
*/

void WebEngineViewer::setLoadExternalResources(bool load_resources) {
  WebViewer::setLoadExternalResources(load_resources);

  page()->settings()->setAttribute(QWebEngineSettings::WebAttribute::AutoLoadImages, load_resources);
}

bool WebEngineViewer::supportsNavigation() const {
  return true;
}

bool WebEngineViewer::supportImagesLoading() const {
  return true;
}

void WebEngineViewer::contextMenuEvent(QContextMenuEvent* event) {
  event->accept();

  auto* menu = new QMenu(this);
  menu->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);

  QPoint pos = event->globalPos();
  pos = QPoint(pos.x(), pos.y() + 1);

  processContextMenu(menu, event);
  menu->popup(pos);
}

QWebEngineView* WebEngineViewer::createWindow(QWebEnginePage::WebWindowType type) {
  return nullptr;
}

void WebEngineViewer::bindToBrowser(WebBrowser* browser) {
  m_browser = browser;

  // NOTE: Just forward QtWebEngine signals, it's all there.
  connect(this, &QWebEngineView::loadStarted, this, &WebEngineViewer::loadingStarted);
  connect(this, &QWebEngineView::loadProgress, this, &WebEngineViewer::loadingProgress);
  connect(this, &QWebEngineView::titleChanged, this, &WebEngineViewer::pageTitleChanged);
  connect(this, &QWebEngineView::iconChanged, this, &WebEngineViewer::pageIconChanged);
  connect(this, &QWebEngineView::urlChanged, this, &WebEngineViewer::pageUrlChanged);

  connect(page(), &WebEnginePage::linkMouseClicked, this, [this](const QUrl& url) {
    emit linkMouseClicked(url);
  });
  connect(page(), &WebEnginePage::linkHovered, this, &WebEngineViewer::linkMouseHighlighted);

  m_actionWatcherGoBack.setAction(page()->action(QWebEnginePage::WebAction::Back));
  m_actionWatcherGoForward.setAction(page()->action(QWebEnginePage::WebAction::Forward));
  m_actionWatcherReloadPage.setAction(page()->action(QWebEnginePage::WebAction::Reload));

  connect(&m_actionWatcherGoBack, &ActionWatcher::enabledChanged, this, &WebEngineViewer::goBackEnabledChanged);
  connect(&m_actionWatcherGoForward, &ActionWatcher::enabledChanged, this, &WebEngineViewer::goForwardEnabledChanged);
  connect(&m_actionWatcherReloadPage, &ActionWatcher::enabledChanged, this, &WebEngineViewer::reloadPageEnabledChanged);
}

void WebEngineViewer::findText(const QString& text, bool backwards) {
  QWebEngineView::findText(text, backwards ? QWebEnginePage::FindFlag::FindBackward : QWebEnginePage::FindFlag{});
}

void WebEngineViewer::reloadNetworkSettings() {}

void WebEngineViewer::setHtml(const QString& html, const QUrl& url, RootItem* root, Feed* feed) {
  m_selectedItem = root;
  m_feed = feed;

  QString display_html = htmlToDisplay(html);
  const QString marker = QLatin1String(ImageMaximumHeightAttribute);
  const QString width_marker = QLatin1String(ImageFitWidthAttribute);
  const QRegularExpression maximum_height(QSL("\\b%1\\s*=\\s*[\"']([0-9]+)[\"']").arg(marker),
                                          QRegularExpression::CaseInsensitiveOption);
  QSet<int> heights;
  auto matches = maximum_height.globalMatch(display_html);
  while (matches.hasNext()) {
    const int height = matches.next().captured(1).toInt();
    if (height > 0) {
      heights.insert(height);
    }
  }
  const QRegularExpression fit_width(QSL("\\b%1\\s*=\\s*[\"']([0-9]+)[\"']").arg(width_marker),
                                     QRegularExpression::CaseInsensitiveOption);
  bool has_width_fitting = false;
  auto width_matches = fit_width.globalMatch(display_html);
  while (width_matches.hasNext()) {
    const int percentage = width_matches.next().captured(1).toInt();
    if (percentage > 0 && percentage <= 100) {
      has_width_fitting = true;
      break;
    }
  }
  m_hasImageSizeLimits = !heights.isEmpty() || has_width_fitting;

  if (m_hasImageSizeLimits) {
    // Bound the pending layout before the first paint. The helper suspends
    // this gate while measuring and reveals each image after applying its cap.
    QString pending_style = QSL("<style id=\"rssguard-image-limit-pending\">");
    if (has_width_fitting) {
      pending_style += QSL("img[%1]:not([data-rssguard-image-ready])"
                           "{visibility:hidden!important;max-width:98%!important;min-width:0!important;}")
                         .arg(width_marker);
    }
    for (int height : std::as_const(heights)) {
      pending_style += QSL("img[%1=\"%2\"]:not([data-rssguard-image-ready])"
                           "{visibility:hidden!important;max-height:%2px!important;min-height:0!important;}")
                         .arg(marker)
                         .arg(height);
    }
    pending_style += QSL("</style>");
    const QRegularExpression head(QSL("<head(?:\\s[^>]*)?>"), QRegularExpression::CaseInsensitiveOption);
    const auto head_match = head.match(display_html);
    if (head_match.hasMatch()) {
      display_html.insert(head_match.capturedEnd(), pending_style);
    }
    else {
      const QRegularExpression doctype(QSL("<!doctype[^>]*>"), QRegularExpression::CaseInsensitiveOption);
      const auto doctype_match = doctype.match(display_html);
      display_html.insert(doctype_match.hasMatch() ? doctype_match.capturedEnd() : 0, pending_style);
    }
  }

  QWebEngineView::setHtml(display_html, url);
}

void WebEngineViewer::applyImageSizeLimits(const std::function<void()>& finished,
                                           bool inspect_document,
                                           const std::function<void()>& cancelled) {
  if (!m_hasImageSizeLimits && !inspect_document) {
    if (finished) {
      finished();
    }
    return;
  }

  static const QString script = []() {
    QFile source(QSL(":/scripts/webengine/image-height-limits.js"));
    if (!source.open(QIODevice::ReadOnly)) {
      return QString();
    }
    return QString::fromUtf8(source.readAll())
      .replace(QSL("@IMAGE_MAXIMUM_HEIGHT_ATTRIBUTE@"), QLatin1String(ImageMaximumHeightAttribute))
      .replace(QSL("@IMAGE_FIT_WIDTH_ATTRIBUTE@"), QLatin1String(ImageFitWidthAttribute))
      .replace(QSL("@IMAGE_WIDTH_FIT_STYLE_ID@"), QLatin1String(ImageWidthFitStyleId));
  }();
  const quint64 content_generation = m_contentGeneration;
  const QWeakPointer<bool> lifetime_guard = m_lifetimeGuard.toWeakRef();

  // Explicit execution also works when publisher JavaScript is disabled;
  // automatic QWebEngineScript injection does not on supported Qt versions.
  page()->runJavaScript(script,
                        QWebEngineScript::ApplicationWorld,
                        [this, lifetime_guard, content_generation, finished, cancelled](const QVariant& result) {
                          const QSharedPointer<bool> guard = lifetime_guard.toStrongRef();
                          if (guard.isNull()) {
                            return;
                          }
                          if (m_contentGeneration != content_generation) {
                            if (cancelled) {
                              cancelled();
                            }
                            return;
                          }
                          m_hasImageSizeLimits = result.toBool();
                          m_imageSizeLimitsInitialized = m_hasImageSizeLimits;
                          if (finished) {
                            finished();
                          }
                        });
}

void WebEngineViewer::scheduleImageSizeLimits() {
  if (m_hasImageSizeLimits && m_imageSizeLimitsInitialized) {
    page()->runJavaScript(QSL("if(window.__rssguardImageLimits){window.__rssguardImageLimits.schedule();}"),
                          QWebEngineScript::ApplicationWorld);
  }
}

double WebEngineViewer::verticalScrollBarPosition() const {
  return page()->scrollPosition().y();
}

void WebEngineViewer::setVerticalScrollBarPosition(double pos) {
  if (std::isfinite(pos)) {
    page()->runJavaScript(QSL("window.scrollTo({left: 0, top: %1, behavior: 'instant'});")
                            .arg(QString::number(pos, 'g', 17)),
                          QWebEngineScript::ApplicationWorld);
  }
}

void WebEngineViewer::scrollVerticallyBy(double delta) {
  if (std::isfinite(delta)) {
    page()->runJavaScript(QSL("window.scrollBy({left: 0, top: %1, behavior: 'instant'});")
                            .arg(QString::number(delta, 'g', 17)),
                          QWebEngineScript::ApplicationWorld);
  }
}

void WebEngineViewer::scrollVerticallyByPage(bool down) {
  // Measure in CSS pixels inside the page, so resize and zoom are respected.
  page()->runJavaScript(QSL("window.scrollBy({left: 0, top: %1 * window.innerHeight, behavior: 'instant'});")
                          .arg(down ? 1 : -1),
                        QWebEngineScript::ApplicationWorld);
}

void WebEngineViewer::applyFont(const QFont& fon) {
  auto pixel_size = QFontMetrics(fon).ascent();

  page()->profile()->settings()->setFontFamily(QWebEngineSettings::FontFamily::StandardFont, fon.family());
  page()->profile()->settings()->setFontFamily(QWebEngineSettings::FontFamily::SerifFont, fon.family());
  page()->profile()->settings()->setFontFamily(QWebEngineSettings::FontFamily::SansSerifFont, fon.family());
  page()->profile()->settings()->setFontSize(QWebEngineSettings::DefaultFontSize, pixel_size);
}

qreal WebEngineViewer::zoomFactor() const {
  return QWebEngineView::zoomFactor();
}

void WebEngineViewer::setZoomFactor(qreal zoom_factor) {
  QWebEngineView::setZoomFactor(WebViewer::normalizedZoomFactor(zoom_factor));

  // Older Qt has no notification, and newer Qt does not emit one until the page is initialized.
  notifyZoomFactorChanged();
}

void WebEngineViewer::notifyZoomFactorChanged() {
  const qreal zoom_factor = zoomFactor();

  if (!qFuzzyCompare(m_lastZoomFactor, zoom_factor)) {
    m_lastZoomFactor = zoom_factor;
    emit viewerZoomFactorChanged(zoom_factor);
    scheduleImageSizeLimits();
  }
}

void WebEngineViewer::printToPdf() {
  QString the_file = QSL("%1.pdf").arg(title());
  QString selected_file = FileDialog::saveFileName(nullptr,
                                                   tr("Save page to PDF file"),
                                                   qApp->documentsFolder(),
                                                   the_file,
                                                   tr("PDF files (*.pdf)"),
                                                   nullptr,
                                                   GENERAL_REMEMBERED_PATH);

  if (selected_file.isEmpty()) {
    return;
  }

  applyImageSizeLimits([this, selected_file]() {
    page()->printToPdf(selected_file);
  });
}

void WebEngineViewer::saveCompleteWebPage() {
  QString the_file = QSL("%1.mhtml").arg(title());
  QString selected_file = FileDialog::saveFileName(nullptr,
                                                   tr("Save complete page to file"),
                                                   qApp->documentsFolder(),
                                                   the_file,
                                                   tr("Mime HTML files (*.mhtml)"),
                                                   nullptr,
                                                   GENERAL_REMEMBERED_PATH);

  if (selected_file.isEmpty()) {
    return;
  }

#if QT_VERSION_MAJOR < 6
  page()->save(selected_file, QWebEngineDownloadItem::SavePageFormat::MimeHtmlSaveFormat);
#else
  page()->save(selected_file, QWebEngineDownloadRequest::SavePageFormat::MimeHtmlSaveFormat);
#endif
}

QString WebEngineViewer::html() const {
  return m_html;
}

QString WebEngineViewer::plainText() const {
  return m_plainText;
}

QUrl WebEngineViewer::url() const {
  return QWebEngineView::url();
}

ContextMenuData WebEngineViewer::provideContextMenuData(QContextMenuEvent* event) {
#if QT_VERSION_MAJOR == 6
  auto* menu_pointer = lastContextMenuRequest();
  QWebEngineContextMenuRequest& menu_data = *menu_pointer;
#else
  QWebEngineContextMenuData menu_data = page()->contextMenuData();
#endif

  ContextMenuData c;

  if (menu_data.mediaUrl().isValid()) {
    c.m_imgLinkUrl = menu_data.mediaUrl();
  }

  if (menu_data.linkUrl().isValid()) {
    c.m_linkUrl = menu_data.linkUrl();
  }

  c.m_selectedText = selectedText();

  return c;
}

void WebEngineViewer::processContextMenu(QMenu* specific_menu, QContextMenuEvent* event) {
  WebViewer::processContextMenu(specific_menu, event);

  specific_menu->addSection(tr("Advanced"));

  // Extra actions.
  specific_menu->addMenu(qApp->icons()->fromTheme(QSL("list-add")), tr("Extra actions"))->addActions(advancedActions());

  // Page actions.
  auto* page_actions_menu = new ScrollableMenu(tr("Page actions"), specific_menu);
  page_actions_menu->setActions(page()->allPageActions(), true);
  page_actions_menu->setIcon(qApp->icons()->fromTheme(QSL("application-x-executable"), QSL("tools")));

  specific_menu->addMenu(page_actions_menu);

  // Web attributes.
  specific_menu->addMenu(qApp->icons()->fromTheme(QSL("applications-internet")), tr("Web attributes"))
    ->addActions(qApp->web()->webEngineAttributeActions());

  // Diagnostics.
  specific_menu->addMenu(qApp->icons()->fromTheme(QSL("redeyes")), tr("Diagnostics"))->addActions(diagActions());
}
