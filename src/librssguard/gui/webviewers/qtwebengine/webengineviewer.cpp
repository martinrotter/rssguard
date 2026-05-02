// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtwebengine/webengineviewer.h"

#include "definitions/definitions.h"
#include "gui/dialogs/filedialog.h"
#include "gui/reusable/scrollablemenu.h"
#include "gui/webbrowser.h"
#include "gui/webviewers/qtwebengine/webenginepage.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/skinfactory.h"
#include "network-web/webfactory.h"

#include <QAction>
#include <QFileIconProvider>
#include <QGraphicsView>
#include <QTimer>
#include <QToolTip>
#include <QWheelEvent>

#if QT_VERSION_MAJOR >= 6
#include <QWebEngineContextMenuRequest>
#else
#include <QWebEngineContextMenuData>
#endif

#include <QWebEngineProfile>
#include <QWebEngineSettings>

WebEngineViewer::WebEngineViewer(QWidget* parent)
  : QWebEngineView(parent), m_browser(nullptr),
    m_actionPrintToPdf(new QAction(qApp->icons()->fromTheme(QSL("document-print")), tr("Print to PDF"), this)),
    m_actionSaveFullPage(new QAction(qApp->icons()->fromTheme(QSL("document-save-as"), QSL("download")),
                                     tr("Save complete webpage"),
                                     this)) {
  WebEnginePage* page = new WebEnginePage(this);

  setPage(page);
  connect(this, &WebEngineViewer::loadFinished, this, [=]() {
    page->toHtml([&](const QString& htm) {
      m_html = htm;
    });
    page->toPlainText([&](const QString& txt) {
      m_plainText = txt;
    });
  });

  connect(m_actionPrintToPdf.data(), &QAction::triggered, this, &WebEngineViewer::printToPdf);
  connect(m_actionSaveFullPage.data(), &QAction::triggered, this, &WebEngineViewer::saveCompleteWebPage);

#if QT_VERSION_MAJOR >= 6
  connect(this, &WebEngineViewer::printFinished, this, [this](bool success) {
    onPrintingFinished(success);
  });
#endif

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

void WebEngineViewer::loadUrl(const QUrl& url) {
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

void WebEngineViewer::cleanupCache() {
  page()->profile()->clearAllVisitedLinks();
  page()->profile()->clearHttpCache();
}

void WebEngineViewer::printToPrinter(QPrinter* printer) {
#if QT_VERSION_MAJOR < 6
  page()->print(printer, [this](bool success) {
    if (success) {
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           GuiMessage(tr("Done"), tr("Printing is finished.")),
                           GuiMessageDestination(true, true, true));
    }
    else {
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           GuiMessage(tr("Error"), tr("Printing failed."), QSystemTrayIcon::MessageIcon::Critical),
                           GuiMessageDestination(true, true, true));
    }

    onPrintingFinished(success);
  });
#else
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
  auto* viewer = new WebEngineViewer(this);
  emit openViewerInNewTab(viewer);
  return viewer;
}

void WebEngineViewer::bindToBrowser(WebBrowser* browser) {
  m_browser = browser;

  // NOTE: Just forward QtWebEngine signals, it's all there.
  connect(this, &QWebEngineView::loadStarted, this, &WebEngineViewer::loadingStarted);
  connect(this, &QWebEngineView::loadProgress, this, &WebEngineViewer::loadingProgress);
  connect(this, &QWebEngineView::loadFinished, this, &WebEngineViewer::loadingFinished);
  connect(this, &QWebEngineView::titleChanged, this, &WebEngineViewer::pageTitleChanged);
  connect(this, &QWebEngineView::iconChanged, this, &WebEngineViewer::pageIconChanged);
  connect(this, &QWebEngineView::urlChanged, this, &WebEngineViewer::pageUrlChanged);

  connect(page(), &WebEnginePage::linkMouseClicked, this, &WebEngineViewer::linkMouseClicked);
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

  page()->printToPdf(selected_file);
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
  specific_menu->addMenu(qApp->icons()->fromTheme(QSL("list-add")), tr("Extra actions"))->addActions(advancedActions());

  auto* page_actions_menu = new ScrollableMenu(tr("Page actions"), specific_menu);
  page_actions_menu->setActions(page()->allPageActions(), true);
  page_actions_menu->setIcon(qApp->icons()->fromTheme(QSL("application-x-executable"), QSL("tools")));

  specific_menu->addMenu(page_actions_menu);

  specific_menu->addMenu(qApp->icons()->fromTheme(QSL("applications-internet")), tr("Web attributes"))
    ->addActions(qApp->web()->webEngineAttributeActions());
}
