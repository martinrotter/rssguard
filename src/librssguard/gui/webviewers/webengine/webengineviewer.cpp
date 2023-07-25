// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/webengine/webengineviewer.h"

#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/tabwidget.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/skinfactory.h"
#include "network-web/adblock/adblockicon.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/networkfactory.h"
#include "network-web/webengine/webenginepage.h"
#include "network-web/webfactory.h"

#include <QFileIconProvider>
#include <QTimer>
#include <QToolTip>

#if QT_VERSION_MAJOR == 6
#include <QWebEngineContextMenuRequest>
#else
#include <QOpenGLWidget>
#include <QWebEngineContextMenuData>
#endif

#include <QWheelEvent>

WebEngineViewer::WebEngineViewer(QWidget* parent) : QWebEngineView(parent), m_browser(nullptr), m_root(nullptr) {
  WebEnginePage* page = new WebEnginePage(this);

  setPage(page);
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

void WebEngineViewer::loadMessages(const QList<Message>& messages, RootItem* root) {
  auto html_messages = qApp->skins()->generateHtmlOfArticles(messages, root);

  m_root = root;
  m_messageContents = html_messages.m_html;
  m_messageBaseUrl = html_messages.m_baseUrl;

  bool previously_enabled = isEnabled();

  setEnabled(false);
  setHtml(m_messageContents, m_messageBaseUrl);
  setEnabled(previously_enabled);

  page()->runJavaScript(QSL("window.scrollTo(0, 0);"));
}

void WebEngineViewer::clear() {
  bool previously_enabled = isEnabled();

  setEnabled(false);
  setHtml(QSL("<!DOCTYPE html><html><body</body></html>"), QUrl(QSL(INTERNAL_URL_BLANK)));
  setEnabled(previously_enabled);
}

void WebEngineViewer::contextMenuEvent(QContextMenuEvent* event) {
  event->accept();

#if QT_VERSION_MAJOR == 6
  QMenu* menu = createStandardContextMenu();
  auto* menu_pointer = lastContextMenuRequest();
  QWebEngineContextMenuRequest& menu_data = *menu_pointer;
#else
  QMenu* menu = page()->createStandardContextMenu();
  QWebEngineContextMenuData menu_data = page()->contextMenuData();
#endif

  if (menu_data.linkUrl().isValid()) {
    QString link_url = menu_data.linkUrl().toString();

    // Add option to open link in external viewe
    menu->addAction(qApp->icons()->fromTheme(QSL("document-open")), tr("Open link in external browser"), [link_url]() {
      qApp->web()->openUrlInExternalBrowser(link_url);

      if (qApp->settings()
            ->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally))
            .toBool()) {
        QTimer::singleShot(1000, qApp, []() {
          qApp->mainForm()->display();
        });
      }
    });
  }

  if (menu_data.mediaUrl().isValid() || menu_data.linkUrl().isValid()) {
    QString media_link =
      menu_data.mediaUrl().isValid() ? menu_data.mediaUrl().toString() : menu_data.linkUrl().toString();
    QFileIconProvider icon_provider;
    QMenu* menu_ext_tools = new QMenu(tr("Open with external tool"), menu);
    auto tools = ExternalTool::toolsFromSettings();

    menu_ext_tools->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

    for (const ExternalTool& tool : qAsConst(tools)) {
      QAction* act_tool = new QAction(QFileInfo(tool.executable()).fileName(), menu_ext_tools);

      act_tool->setIcon(icon_provider.icon(QFileInfo(tool.executable())));
      act_tool->setToolTip(tool.executable());
      act_tool->setData(QVariant::fromValue(tool));
      menu_ext_tools->addAction(act_tool);

      connect(act_tool, &QAction::triggered, this, [this, act_tool, media_link]() {
        openUrlWithExternalTool(act_tool->data().value<ExternalTool>(), media_link);
      });
    }

    if (menu_ext_tools->actions().isEmpty()) {
      QAction* act_not_tools = new QAction(tr("No external tools activated"));

      act_not_tools->setEnabled(false);
      menu_ext_tools->addAction(act_not_tools);
    }

    menu->addMenu(menu_ext_tools);
  }

  menu->addAction(qApp->web()->adBlock()->adBlockIcon());
  menu->addAction(qApp->web()->engineSettingsAction());

  const QPoint pos = event->globalPos();
  QPoint p(pos.x(), pos.y() + 1);

  menu->popup(p);
}

QWebEngineView* WebEngineViewer::createWindow(QWebEnginePage::WebWindowType type) {
  auto* viewer = new WebEngineViewer(this);
  emit newWindowRequested(viewer);

  return viewer;
}

void WebEngineViewer::openUrlWithExternalTool(ExternalTool tool, const QString& target_url) {
  tool.run(target_url);
}

RootItem* WebEngineViewer::root() const {
  return m_root;
}

void WebEngineViewer::bindToBrowser(WebBrowser* browser) {
  m_browser = browser;

  browser->m_actionBack = pageAction(QWebEnginePage::WebAction::Back);
  browser->m_actionForward = pageAction(QWebEnginePage::WebAction::Forward);
  browser->m_actionReload = pageAction(QWebEnginePage::WebAction::Reload);
  browser->m_actionStop = pageAction(QWebEnginePage::WebAction::Stop);

  // NOTE: Just forward QtWebEngine signals, it's all there.
  connect(this, &QWebEngineView::loadStarted, this, &WebEngineViewer::loadingStarted);
  connect(this, &QWebEngineView::loadProgress, this, &WebEngineViewer::loadingProgress);
  connect(this, &QWebEngineView::loadFinished, this, &WebEngineViewer::loadingFinished);
  connect(this, &QWebEngineView::titleChanged, this, &WebEngineViewer::pageTitleChanged);
  connect(this, &QWebEngineView::iconChanged, this, &WebEngineViewer::pageIconChanged);
  connect(this, &QWebEngineView::urlChanged, this, &WebEngineViewer::pageUrlChanged);

  connect(page(), &QWebEnginePage::windowCloseRequested, this, &WebEngineViewer::closeWindowRequested);
  connect(page(), &QWebEnginePage::linkHovered, this, &WebEngineViewer::linkMouseHighlighted);
}

void WebEngineViewer::findText(const QString& text, bool backwards) {
  if (backwards) {
    QWebEngineView::findText(text, QWebEnginePage::FindFlag::FindBackward);
  }
  else {
    QWebEngineView::findText(text);
  }
}

void WebEngineViewer::setUrl(const QUrl& url) {
  QWebEngineView::setUrl(url);
}

void WebEngineViewer::setHtml(const QString& html, const QUrl& base_url) {
  QWebEngineView::setHtml(html, base_url);
}

void WebEngineViewer::setReadabledHtml(const QString& html, const QUrl& base_url) {
  auto better_html = qApp->skins()->prepareHtml(html, base_url);

  setHtml(better_html.m_html, better_html.m_baseUrl);
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

  qApp->web()->engineProfile()->settings()->setFontFamily(QWebEngineSettings::FontFamily::StandardFont, fon.family());
  qApp->web()->engineProfile()->settings()->setFontFamily(QWebEngineSettings::FontFamily::SerifFont, fon.family());
  qApp->web()->engineProfile()->settings()->setFontFamily(QWebEngineSettings::FontFamily::SansSerifFont, fon.family());
  qApp->web()->engineProfile()->settings()->setFontSize(QWebEngineSettings::DefaultFontSize, pixel_size);
}

qreal WebEngineViewer::zoomFactor() const {
  return QWebEngineView::zoomFactor();
}

void WebEngineViewer::setZoomFactor(qreal zoom_factor) {
  QWebEngineView::setZoomFactor(zoom_factor);
}

QString WebEngineViewer::html() const {
  QEventLoop loop;
  QString htmll;

  page()->toHtml([&](const QString& htm) {
    htmll = htm;
    loop.exit();
  });

  loop.exec();

  return htmll;
}

QUrl WebEngineViewer::url() const {
  return QWebEngineView::url();
}
