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

#include <QWebEngineProfile>
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
  Skin skin = qApp->skins()->currentSkin();
  QString messages_layout;
  QString single_message_layout = skin.m_layoutMarkup;

  for (const Message& message : messages) {
    QString enclosures;
    QString enclosure_images;

    for (const Enclosure& enclosure : message.m_enclosures) {
      QString enc_url = QUrl::fromPercentEncoding(enclosure.m_url.toUtf8());

      enclosures += skin.m_enclosureMarkup.arg(enc_url,
                                               QSL("&#129527;"),
                                               enclosure.m_mimeType);

      if (enclosure.m_mimeType.startsWith(QSL("image/")) &&
          qApp->settings()->value(GROUP(Messages), SETTING(Messages::DisplayEnclosuresInMessage)).toBool()) {
        // Add thumbnail image.
        enclosure_images += skin.m_enclosureImageMarkup.arg(
          enclosure.m_url,
          enclosure.m_mimeType,
          qApp->settings()->value(GROUP(Messages), SETTING(Messages::MessageHeadImageHeight)).toString());
      }
    }

    QString msg_date = qApp->settings()->value(GROUP(Messages), SETTING(Messages::UseCustomDate)).toBool()
                       ? message.m_created.toLocalTime().toString(qApp->settings()->value(GROUP(Messages),
                                                                                          SETTING(Messages::CustomDateFormat)).toString())
                       : qApp->localization()->loadedLocale().toString(message.m_created.toLocalTime(),
                                                                       QLocale::FormatType::ShortFormat);

    messages_layout.append(single_message_layout
                           .arg(message.m_title,
                                tr("Written by ") + (message.m_author.isEmpty() ?
                                                     tr("unknown author") :
                                                     message.m_author),
                                message.m_url,
                                message.m_contents,
                                msg_date,
                                enclosures,
                                enclosure_images,
                                QString::number(message.m_id)));
  }

  m_messageContents = skin.m_layoutMarkupWrapper.arg(messages.size() == 1
                                                     ? messages.at(0).m_title
                                                     : tr("Newspaper view"),
                                                     messages_layout);

  m_root = root;

  auto* feed = root->getParentServiceRoot()->getItemFromSubTree([messages](const RootItem* it) {
    return it->kind() == RootItem::Kind::Feed && it->customId() == messages.at(0).m_feedId;
  })->toFeed();

  m_messageBaseUrl = QString();

  if (feed != nullptr) {
    QUrl url(NetworkFactory::sanitizeUrl(feed->source()));

    if (url.isValid()) {
      m_messageBaseUrl = url.scheme() + QSL("://") + url.host();
    }
  }

  bool previously_enabled = isEnabled();

  setEnabled(false);
  setHtml(m_messageContents, m_messageBaseUrl /*, QUrl::fromUserInput(INTERNAL_URL_MESSAGE)*/);
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

      if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally)).toBool()) {
        QTimer::singleShot(1000, qApp, []() {
          qApp->mainForm()->display();
        });
      }
    });
  }

  if (menu_data.mediaUrl().isValid() || menu_data.linkUrl().isValid()) {
    QString media_link = menu_data.mediaUrl().isValid() ? menu_data.mediaUrl().toString() : menu_data.linkUrl().toString();
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

  connect(this, &WebEngineViewer::loadStarted, browser, &WebBrowser::onLoadingStarted);
  connect(this, &WebEngineViewer::loadProgress, browser, &WebBrowser::onLoadingProgress);
  connect(this, &WebEngineViewer::loadFinished, browser, &WebBrowser::onLoadingFinished);
  connect(this, &WebEngineViewer::titleChanged, browser, &WebBrowser::onTitleChanged);
  connect(this, &WebEngineViewer::iconChanged, browser, &WebBrowser::onIconChanged);
  connect(this, &WebEngineViewer::urlChanged, browser, &WebBrowser::updateUrl);
  connect(this, &WebEngineViewer::newWindowRequested, browser, &WebBrowser::newWindowRequested);

  connect(page(), &WebEnginePage::windowCloseRequested, browser, &WebBrowser::windowCloseRequested);
  connect(page(), &WebEnginePage::linkHovered, browser, &WebBrowser::onLinkHovered);
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

  QWebEngineProfile::defaultProfile()->settings()->setFontFamily(QWebEngineSettings::FontFamily::StandardFont, fon.family());
  QWebEngineProfile::defaultProfile()->settings()->setFontFamily(QWebEngineSettings::FontFamily::SerifFont, fon.family());
  QWebEngineProfile::defaultProfile()->settings()->setFontFamily(QWebEngineSettings::FontFamily::SansSerifFont, fon.family());
  QWebEngineProfile::defaultProfile()->settings()->setFontSize(QWebEngineSettings::DefaultFontSize, pixel_size);
}

bool WebEngineViewer::canZoomIn() const {
  return zoomFactor() <= double(MAX_ZOOM_FACTOR) - double(ZOOM_FACTOR_STEP);
}

bool WebEngineViewer::canZoomOut() const {
  return zoomFactor() >= double(MIN_ZOOM_FACTOR) + double(ZOOM_FACTOR_STEP);
}

qreal WebEngineViewer::zoomFactor() const {
  return QWebEngineView::zoomFactor();
}

void WebEngineViewer::zoomIn() {
  setZoomFactor(zoomFactor() + double(ZOOM_FACTOR_STEP));
}

void WebEngineViewer::zoomOut() {
  setZoomFactor(zoomFactor() - double(ZOOM_FACTOR_STEP));
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
