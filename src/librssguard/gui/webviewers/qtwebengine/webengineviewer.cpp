// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtwebengine/webengineviewer.h"

#include "definitions/definitions.h"
#include "gui/webbrowser.h"
#include "gui/webviewers/qtwebengine/webenginepage.h"
#include "miscellaneous/application.h"
#include "miscellaneous/externaltool.h"
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

void WebEngineViewer::contextMenuEvent(QContextMenuEvent* event) {
  event->accept();

  /*
  #if QT_VERSION_MAJOR == 6
    QMenu* menu = createStandardContextMenu();
  #else
    QMenu* menu = page()->createStandardContextMenu();
  #endif
    */

  auto* menu = new QMenu(tr("Context menu for article viewer"), this);

  menu->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);

  const QPoint pos = event->globalPos();
  QPoint p(pos.x(), pos.y() + 1);

  processContextMenu(menu, event);

  menu->popup(p);
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
    c.m_imgLinkUrl = menu_data.linkUrl();
  }

  if (menu_data.linkUrl().isValid()) {
    c.m_linkUrl = menu_data.linkUrl();
  }

  return c;
}
