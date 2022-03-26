// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/litehtml/litehtmlviewer.h"

#include "core/message.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/skinfactory.h"
#include "network-web/networkfactory.h"

#include <QAction>
#include <QWheelEvent>

LiteHtmlViewer::LiteHtmlViewer(QWidget* parent) : QLiteHtmlWidget(parent) {
  setResourceHandler([this](const QUrl& url) {
    QByteArray output;

    NetworkFactory::performNetworkOperation(
      url.toString(),
      5000,
      {},
      output,
      QNetworkAccessManager::Operation::GetOperation);

    return output;
  });
}

void LiteHtmlViewer::bindToBrowser(WebBrowser* browser) {
  browser->m_actionBack = new QAction(this);
  browser->m_actionForward = new QAction(this);
  browser->m_actionReload = new QAction(this);
  browser->m_actionStop = new QAction(this);

  connect(this, &LiteHtmlViewer::zoomFactorChanged, browser, &WebBrowser::onZoomFactorChanged);

  // TODO: změna ikon, změna stavu akcí.

  /*
     connect(this, &WebEngineViewer::urlChanged, browser, &WebBrowser::updateUrl);
     connect(this, &WebEngineViewer::loadStarted, browser, &WebBrowser::onLoadingStarted);
     connect(this, &WebEngineViewer::loadProgress, browser, &WebBrowser::onLoadingProgress);
     connect(this, &WebEngineViewer::loadFinished, browser, &WebBrowser::onLoadingFinished);
     connect(this, &WebEngineViewer::titleChanged, browser, &WebBrowser::onTitleChanged);
     connect(this, &WebEngineViewer::iconChanged, browser, &WebBrowser::onIconChanged);

     connect(page(), &WebEnginePage::windowCloseRequested, browser, &WebBrowser::closeRequested);
     connect(page(), &WebEnginePage::linkHovered, browser, &WebBrowser::onLinkHovered);
   */
}

void LiteHtmlViewer::findText(const QString& text, bool backwards) {}

void LiteHtmlViewer::setUrl(const QUrl& url) {
  QByteArray output;

  NetworkFactory::performNetworkOperation(
    url.toString(),
    5000,
    {},
    output,
    QNetworkAccessManager::Operation::GetOperation);

  setHtml(QString::fromUtf8(output), url);
}

void LiteHtmlViewer::setHtml(const QString& html, const QUrl& base_url) {
  QLiteHtmlWidget::setUrl(base_url);
  QLiteHtmlWidget::setHtml(html);
}

QString LiteHtmlViewer::html() const {
  return {};
}

QUrl LiteHtmlViewer::url() const {
  return {};
}

void LiteHtmlViewer::clear() {}

void LiteHtmlViewer::loadMessages(const QList<Message>& messages, RootItem* root) {
  Skin skin = qApp->skins()->currentSkin();
  QString messages_layout;
  QString single_message_layout = skin.m_layoutMarkup;

  for (const Message& message : messages) {
    QString enclosures;
    QString enclosure_images;

    for (const Enclosure& enclosure : message.m_enclosures) {
      QString enc_url;

      if (!enclosure.m_url.contains(QRegularExpression(QSL("^(http|ftp|\\/)")))) {
        enc_url = QSL(INTERNAL_URL_PASSATTACHMENT) + QL1S("/?") + enclosure.m_url;
      }
      else {
        enc_url = enclosure.m_url;
      }

      enc_url = QUrl::fromPercentEncoding(enc_url.toUtf8());

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

  QString msg_contents = skin.m_layoutMarkupWrapper.arg(messages.size() == 1
                                                     ? messages.at(0).m_title
                                                     : tr("Newspaper view"),
                                                        messages_layout);
  auto* feed = root->getParentServiceRoot()->getItemFromSubTree([messages](const RootItem* it) {
    return it->kind() == RootItem::Kind::Feed && it->customId() == messages.at(0).m_feedId;
  })->toFeed();
  QString base_url;

  if (feed != nullptr) {
    QUrl url(NetworkFactory::sanitizeUrl(feed->source()));

    if (url.isValid()) {
      base_url = url.scheme() + QSL("://") + url.host();
    }
  }

  setHtml(msg_contents, QUrl::fromUserInput(base_url));
}

double LiteHtmlViewer::verticalScrollBarPosition() const {
  return {};
}

void LiteHtmlViewer::setVerticalScrollBarPosition(double pos) {}

void LiteHtmlViewer::reloadFontSettings(const QFont& fon) {}

bool LiteHtmlViewer::canZoomIn() const {
  return zoomFactor() <= double(MAX_ZOOM_FACTOR) - double(ZOOM_FACTOR_STEP);
}

bool LiteHtmlViewer::canZoomOut() const {
  return zoomFactor() >= double(MIN_ZOOM_FACTOR) + double(ZOOM_FACTOR_STEP);
}

qreal LiteHtmlViewer::zoomFactor() const {
  return QLiteHtmlWidget::zoomFactor();
}

void LiteHtmlViewer::zoomIn() {
  setZoomFactor(zoomFactor() + double(ZOOM_FACTOR_STEP));
}

void LiteHtmlViewer::zoomOut() {
  setZoomFactor(zoomFactor() - double(ZOOM_FACTOR_STEP));
}

void LiteHtmlViewer::setZoomFactor(qreal zoom_factor) {
  if (zoom_factor == 0.0) {
    QLiteHtmlWidget::setZoomFactor(0.1);
  }
  else {
    QLiteHtmlWidget::setZoomFactor(zoom_factor);
  }
}

void LiteHtmlViewer::wheelEvent(QWheelEvent* event) {
  if ((event->modifiers() & Qt::KeyboardModifier::ControlModifier) > 0) {
    if (event->angleDelta().y() > 0 && canZoomIn()) {
      zoomIn();
      emit zoomFactorChanged();
    }
    else if (event->angleDelta().y() < 0 && canZoomOut()) {
      zoomOut();
      emit zoomFactorChanged();
    }
  }

  QLiteHtmlWidget::wheelEvent(event);
}
