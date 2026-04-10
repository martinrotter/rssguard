// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtwebengine/webengineviewer.h"

#include "definitions/definitions.h"
#include "gui/webbrowser.h"
#include "gui/webviewers/qtwebengine/webenginepage.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/skinfactory.h"
#include "network-web/webfactory.h"

#include <QFileIconProvider>
#include <QGraphicsView>
#include <QTimer>
#include <QToolTip>
#include <QWheelEvent>

#if QT_VERSION_MAJOR == 6
#include <QWebEngineContextMenuRequest>
#else
#include <QWebEngineContextMenuData>
#endif

#include <QWebEngineProfile>
#include <QWebEngineSettings>

WebEngineViewer::WebEngineViewer(QWidget* parent) : QWebEngineView(parent), m_browser(nullptr) {
  WebEnginePage* page = new WebEnginePage(this);

  setPage(page);
  connect(this, &WebEngineViewer::loadFinished, this, [=]() {
    page->toHtml([&](const QString& htm) {
      m_html = htm;
    });
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

WebEnginePage* WebEngineViewer::page() const {
  return qobject_cast<WebEnginePage*>(QWebEngineView::page());
}

void WebEngineViewer::loadMessage(const Message& message, RootItem* root) {
  auto url = urlForMessage(message, root);
  auto html = htmlForMessage(message, root);

  setHtml(html, url, root);

  setVerticalScrollBarPosition(0.0);
}

QString WebEngineViewer::htmlForMessage(const Message& message, RootItem* root) const {
  auto html_message = qApp->skins()->generateHtmlOfArticle(message, root);
  return html_message;
}

void WebEngineViewer::loadUrl(const QUrl& url) {}

void WebEngineViewer::clear() {
  bool previously_enabled = isEnabled();

  setEnabled(false);
  setHtml(QSL("<!DOCTYPE html><html><body</body></html>"));
  setEnabled(previously_enabled);
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

void WebEngineViewer::contextMenuEvent(QContextMenuEvent* event) {
  event->accept();

  /*
  #if QT_VERSION_MAJOR == 6
    QMenu* menu = createStandardContextMenu();
  #else
    QMenu* menu = page()->createStandardContextMenu();
  #endif
    */

  auto* menu = new QMenu(this);
  menu->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);

  QPoint pos = event->globalPos();
  pos = QPoint(pos.x(), pos.y() + 1);

  processContextMenu(menu, event);
  menu->popup(pos);
}

/*
QWebEngineView* WebEngineViewer::createWindow(QWebEnginePage::WebWindowType type) {
  auto* viewer = new WebEngineViewer(this);
  // emit newWindowRequested(viewer);

  return viewer;
}
*/

void WebEngineViewer::bindToBrowser(WebBrowser* browser) {
  m_browser = browser;

  /*
  browser->m_actionBack = pageAction(QWebEnginePage::WebAction::Back);
  browser->m_actionForward = pageAction(QWebEnginePage::WebAction::Forward);
  browser->m_actionReload = pageAction(QWebEnginePage::WebAction::Reload);
  browser->m_actionStop = pageAction(QWebEnginePage::WebAction::Stop);
*/

  // NOTE: Just forward QtWebEngine signals, it's all there.
  connect(this, &QWebEngineView::loadStarted, this, &WebEngineViewer::loadingStarted);
  connect(this, &QWebEngineView::loadProgress, this, &WebEngineViewer::loadingProgress);
  connect(this, &QWebEngineView::loadFinished, this, &WebEngineViewer::loadingFinished);
  connect(this, &QWebEngineView::titleChanged, this, &WebEngineViewer::pageTitleChanged);
  connect(this, &QWebEngineView::iconChanged, this, &WebEngineViewer::pageIconChanged);
  connect(this, &QWebEngineView::urlChanged, this, &WebEngineViewer::pageUrlChanged);

  connect(page(), &WebEnginePage::linkMouseClicked, this, &WebEngineViewer::linkMouseClicked);
  connect(page(), &WebEnginePage::linkHovered, this, &WebEngineViewer::linkMouseHighlighted);
}

void WebEngineViewer::findText(const QString& text, bool backwards) {
  QWebEngineView::findText(text, backwards ? QWebEnginePage::FindFlag::FindBackward : QWebEnginePage::FindFlag{});
}

void WebEngineViewer::reloadNetworkSettings() {}

void WebEngineViewer::setHtml(const QString& html, const QUrl& url, RootItem* root) {
  QWebEngineView::setHtml(html, url);

  // IOFactory::writeFile("a.html", html.toUtf8());
}

double WebEngineViewer::verticalScrollBarPosition() const {
  double position;
  QEventLoop loop;

  page()->runJavaScript(QSL("window.pageYOffset;"), [&position, &loop](const QVariant& val) {
    position = val.toDouble();
    loop.exit();
  });
  loop.exec();

  return position;
}

void WebEngineViewer::setVerticalScrollBarPosition(double pos) {
  page()->runJavaScript(QSL("window.scrollTo(0, %1);").arg(pos));
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
  QWebEngineView::setZoomFactor(zoom_factor);
}

QString WebEngineViewer::html() const {
  return m_html;
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

QAction* WebEngineViewer::createEngineSettingsAction(QObject* parent,
                                                     const QString& title,
                                                     QWebEngineSettings::WebAttribute web_attribute) {
  auto* act = new QAction(title, parent);

  act->setData(web_attribute);
  act->setCheckable(true);
  act->setChecked(qApp->settings()->value(WebEngineAttributes::ID, QString::number(int(web_attribute)), true).toBool());

  auto enabl = act->isChecked();

  page()->settings()->setAttribute(web_attribute, enabl);
  connect(act, &QAction::toggled, this, &WebEngineViewer::onWebEngineAttributeChanged);
  return act;
}

void WebEngineViewer::onWebEngineAttributeChanged(bool enabled) {
  const QAction* const act = qobject_cast<QAction*>(sender());

  QWebEngineSettings::WebAttribute attribute = act->data().value<QWebEngineSettings::WebAttribute>();

  qApp->settings()->setValue(WebEngineAttributes::ID, QString::number(static_cast<int>(attribute)), enabled);
  page()->settings()->setAttribute(attribute, act->isChecked());
}

void WebEngineViewer::processContextMenu(QMenu* specific_menu, QContextMenuEvent* event) {
  WebViewer::processContextMenu(specific_menu, event);

  specific_menu->addSection(tr("Advanced"));

  QMenu* menu_web_attrs =
    specific_menu->addMenu(qApp->icons()->fromTheme(QSL("applications-internet")), tr("Web attributes"));

  QList<QAction*> actions;

  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("JS enabled"),
                                        QWebEngineSettings::WebAttribute::JavascriptEnabled);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("JS can open popup windows"),
                                        QWebEngineSettings::WebAttribute::JavascriptCanOpenWindows);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("JS can access clipboard"),
                                        QWebEngineSettings::WebAttribute::JavascriptCanAccessClipboard);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Hyperlinks can get focus"),
                                        QWebEngineSettings::WebAttribute::LinksIncludedInFocusChain);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Local storage enabled"),
                                        QWebEngineSettings::WebAttribute::LocalStorageEnabled);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Local content can access remote URLs"),
                                        QWebEngineSettings::WebAttribute::LocalContentCanAccessRemoteUrls);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("XSS auditing enabled"),
                                        QWebEngineSettings::WebAttribute::XSSAuditingEnabled);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Spatial navigation enabled"),
                                        QWebEngineSettings::WebAttribute::SpatialNavigationEnabled);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Local content can access local files"),
                                        QWebEngineSettings::WebAttribute::LocalContentCanAccessFileUrls);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Hyperlink auditing enabled"),
                                        QWebEngineSettings::WebAttribute::HyperlinkAuditingEnabled);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Animate scrolling"),
                                        QWebEngineSettings::WebAttribute::ScrollAnimatorEnabled);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Error pages enabled"),
                                        QWebEngineSettings::WebAttribute::ErrorPageEnabled);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Plugins enabled"),
                                        QWebEngineSettings::WebAttribute::PluginsEnabled);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Fullscreen enabled"),
                                        QWebEngineSettings::WebAttribute::FullScreenSupportEnabled);

#if !defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Screen capture enabled"),
                                        QWebEngineSettings::WebAttribute::ScreenCaptureEnabled);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("WebGL enabled"),
                                        QWebEngineSettings::WebAttribute::WebGLEnabled);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Accelerate 2D canvas"),
                                        QWebEngineSettings::WebAttribute::Accelerated2dCanvasEnabled);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Print element backgrounds"),
                                        QWebEngineSettings::WebAttribute::PrintElementBackgrounds);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Allow running insecure content"),
                                        QWebEngineSettings::WebAttribute::AllowRunningInsecureContent);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Allow geolocation on insecure origins"),
                                        QWebEngineSettings::WebAttribute::AllowGeolocationOnInsecureOrigins);
#endif

  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("JS can activate windows"),
                                        QWebEngineSettings::WebAttribute::AllowWindowActivationFromJavaScript);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Show scrollbars"),
                                        QWebEngineSettings::WebAttribute::ShowScrollBars);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Media playback with gestures"),
                                        QWebEngineSettings::WebAttribute::PlaybackRequiresUserGesture);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("WebRTC uses only public interfaces"),
                                        QWebEngineSettings::WebAttribute::WebRTCPublicInterfacesOnly);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("JS can paste from clipboard"),
                                        QWebEngineSettings::WebAttribute::JavascriptCanPaste);
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("DNS prefetch enabled"),
                                        QWebEngineSettings::WebAttribute::DnsPrefetchEnabled);

#if QT_VERSION >= 0x050D00 // Qt >= 5.13.0
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("PDF viewer enabled"),
                                        QWebEngineSettings::WebAttribute::PdfViewerEnabled);
#endif

#if QT_VERSION >= 0x060700 // Qt >= 6.7.0
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Force dark mode"),
                                        QWebEngineSettings::WebAttribute::ForceDarkMode);
#endif

#if QT_VERSION >= 0x060900 // Qt >= 6.9.0
  actions << createEngineSettingsAction(menu_web_attrs,
                                        tr("Printing - print headers/footers."),
                                        QWebEngineSettings::WebAttribute::PrintHeaderAndFooter);
#endif

  menu_web_attrs->addActions(actions);
}
