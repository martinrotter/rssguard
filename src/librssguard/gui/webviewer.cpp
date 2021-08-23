// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewer.h"

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
#include "network-web/webfactory.h"
#include "network-web/webpage.h"

#include <QFileIconProvider>
#include <QOpenGLWidget>
#include <QTimer>
#include <QWebEngineContextMenuData>
#include <QWheelEvent>

WebViewer::WebViewer(QWidget* parent) : QWebEngineView(parent), m_root(nullptr) {
  WebPage* page = new WebPage(this);

  setPage(page);
  resetWebPageZoom();
}

bool WebViewer::canIncreaseZoom() {
  return zoomFactor() <= double(MAX_ZOOM_FACTOR) - double(ZOOM_FACTOR_STEP);
}

bool WebViewer::canDecreaseZoom() {
  return zoomFactor() >= double(MIN_ZOOM_FACTOR) + double(ZOOM_FACTOR_STEP);
}

bool WebViewer::event(QEvent* event) {
  if (event->type() == QEvent::Type::ChildAdded) {
    QChildEvent* child_ev = static_cast<QChildEvent*>(event);
    QWidget* w = dynamic_cast<QWidget*>(child_ev->child());

    if (w != nullptr) {
      w->installEventFilter(this);
    }
  }

  return QWebEngineView::event(event);
}

WebPage* WebViewer::page() const {
  return qobject_cast<WebPage*>(QWebEngineView::page());
}

void WebViewer::displayMessage() {
  setHtml(m_messageContents, m_messageBaseUrl /*, QUrl::fromUserInput(INTERNAL_URL_MESSAGE)*/);
}

bool WebViewer::increaseWebPageZoom() {
  if (canIncreaseZoom()) {
    setZoomFactor(zoomFactor() + double(ZOOM_FACTOR_STEP));
    qApp->settings()->setValue(GROUP(Messages), Messages::Zoom, zoomFactor());
    return true;
  }
  else {
    return false;
  }
}

bool WebViewer::decreaseWebPageZoom() {
  if (canDecreaseZoom()) {
    setZoomFactor(zoomFactor() - double(ZOOM_FACTOR_STEP));
    qApp->settings()->setValue(GROUP(Messages), Messages::Zoom, zoomFactor());
    return true;
  }
  else {
    return false;
  }
}

bool WebViewer::resetWebPageZoom(bool to_factory_default) {
  const qreal new_factor = to_factory_default ? 1.0 : qApp->settings()->value(GROUP(Messages),
                                                                              SETTING(Messages::Zoom)).toReal();

  if (to_factory_default) {
    qApp->settings()->setValue(GROUP(Messages), Messages::Zoom, new_factor);
  }

  if (new_factor != zoomFactor()) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}

void WebViewer::loadMessages(const QList<Message>& messages, RootItem* root) {
  Skin skin = qApp->skins()->currentSkin();
  QString messages_layout;
  QString single_message_layout = skin.m_layoutMarkup;

  for (const Message& message : messages) {
    QString enclosures;
    QString enclosure_images;

    for (const Enclosure& enclosure : message.m_enclosures) {
      QString enc_url;

      if (!enclosure.m_url.contains(QRegularExpression(QSL("^(http|ftp|\\/)")))) {
        enc_url = QString(INTERNAL_URL_PASSATTACHMENT) + QL1S("/?") + enclosure.m_url;
      }
      else {
        enc_url = enclosure.m_url;
      }

      enc_url = QUrl::fromPercentEncoding(enc_url.toUtf8());

      enclosures += skin.m_enclosureMarkup.arg(enc_url,
                                               QSL("&#129527;"), enclosure.m_mimeType);

      if (enclosure.m_mimeType.startsWith(QSL("image/")) &&
          qApp->settings()->value(GROUP(Messages), SETTING(Messages::DisplayEnclosuresInMessage)).toBool()) {
        // Add thumbnail image.
        enclosure_images += skin.m_enclosureImageMarkup.arg(
          enclosure.m_url,
          enclosure.m_mimeType,
          qApp->settings()->value(GROUP(Messages), SETTING(Messages::MessageHeadImageHeight)).toString());
      }
    }

    messages_layout.append(single_message_layout
                           .arg(message.m_title,
                                tr("Written by ") + (message.m_author.isEmpty() ?
                                                     tr("unknown author") :
                                                     message.m_author),
                                message.m_url,
                                message.m_contents,
                                QLocale().toString(message.m_created.toLocalTime(), QLocale::FormatType::ShortFormat),
                                enclosures,
                                enclosure_images));
  }

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

  m_messageContents = skin.m_layoutMarkupWrapper.arg(messages.size() == 1 ? messages.at(0).m_title : tr("Newspaper view"),
                                                     messages_layout);

  bool previously_enabled = isEnabled();

  setEnabled(false);
  displayMessage();
  setEnabled(previously_enabled);

  page()->runJavaScript(QSL("window.scrollTo(0, 0);"));
}

void WebViewer::clear() {
  bool previously_enabled = isEnabled();

  setEnabled(false);
  setHtml("<!DOCTYPE html><html><body</body></html>", QUrl(INTERNAL_URL_BLANK));
  setEnabled(previously_enabled);
}

void WebViewer::contextMenuEvent(QContextMenuEvent* event) {
  event->accept();
  QMenu* menu = page()->createStandardContextMenu();
  QWebEngineContextMenuData menu_data = page()->contextMenuData();

  if (menu_data.linkUrl().isValid()) {
    // Add option to open link in external viewe
    menu->addAction(qApp->icons()->fromTheme(QSL("document-open")), tr("Open link in external browser"), [menu_data]() {
      qApp->web()->openUrlInExternalBrowser(menu_data.linkUrl().toString());

      if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally)).toBool()) {
        QTimer::singleShot(1000, qApp, []() {
          qApp->mainForm()->display();
        });
      }
    });
  }

  if (menu_data.mediaUrl().isValid() || menu_data.linkUrl().isValid()) {
    QFileIconProvider icon_provider;
    QMenu* menu_ext_tools = new QMenu(tr("Open with external tool"), menu);
    auto tools = ExternalTool::toolsFromSettings();

    menu_ext_tools->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

    for (const ExternalTool& tool : qAsConst(tools)) {
      QAction* act_tool = new QAction(QFileInfo(tool.executable()).fileName(), menu_ext_tools);

      act_tool->setIcon(icon_provider.icon(tool.executable()));
      act_tool->setToolTip(tool.executable());
      act_tool->setData(QVariant::fromValue(tool));
      menu_ext_tools->addAction(act_tool);

      connect(act_tool, &QAction::triggered, this, [this, act_tool, menu_data]() {
        openUrlWithExternalTool(act_tool->data().value<ExternalTool>(), menu_data);
      });
    }

    if (menu_ext_tools->actions().isEmpty()) {
      QAction* act_not_tools = new QAction("No external tools activated");

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

QWebEngineView* WebViewer::createWindow(QWebEnginePage::WebWindowType type) {
  Q_UNUSED(type)
  int index = qApp->mainForm()->tabWidget()->addBrowser(false, false);

  if (index >= 0) {
    return qApp->mainForm()->tabWidget()->widget(index)->webBrowser()->viewer();
  }
  else {
    return nullptr;
  }
}

void WebViewer::wheelEvent(QWheelEvent* event) {
  QWebEngineView::wheelEvent(event);
}

bool WebViewer::eventFilter(QObject* object, QEvent* event) {
  Q_UNUSED(object)

  if (event->type() == QEvent::Type::Wheel) {
    QWheelEvent* wh_event = static_cast<QWheelEvent*>(event);

    if ((wh_event->modifiers() & Qt::KeyboardModifier::ControlModifier) > 0) {
      if (wh_event->angleDelta().y() > 0) {
        increaseWebPageZoom();
        return true;
      }
      else if (wh_event->angleDelta().y() < 0) {
        decreaseWebPageZoom();
        return true;
      }
    }
  }
  else if (event->type() == QEvent::Type::KeyPress) {
    QKeyEvent* key_event = static_cast<QKeyEvent*>(event);

    if ((key_event->modifiers() & Qt::KeyboardModifier::ControlModifier) > 0) {
      if (key_event->key() == Qt::Key::Key_Plus) {
        increaseWebPageZoom();
        return true;
      }
      else if (key_event->key() == Qt::Key::Key_Minus) {
        decreaseWebPageZoom();
        return true;
      }
      else if (key_event->key() == Qt::Key::Key_0) {
        resetWebPageZoom(true);
        return true;
      }
    }
  }

  return false;
}

void WebViewer::openUrlWithExternalTool(ExternalTool tool, const QWebEngineContextMenuData& target) {
  tool.run(target.mediaUrl().isValid() ? target.mediaUrl().toString() : target.linkUrl().toString());
}

RootItem* WebViewer::root() const {
  return m_root;
}
