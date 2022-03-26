// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/litehtml/litehtmlviewer.h"

#include "core/message.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/skinfactory.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"

#include <QAction>
#include <QClipboard>
#include <QScrollBar>
#include <QWheelEvent>

LiteHtmlViewer::LiteHtmlViewer(QWidget* parent) : QLiteHtmlWidget(parent) {
  setResourceHandler([this](const QUrl& url) {
    emit loadProgress(-1);
    return handleResource(url);
  });

  connect(this, &LiteHtmlViewer::linkClicked, this, &LiteHtmlViewer::setUrl);

  connect(this, &LiteHtmlViewer::copyAvailable, this, [this](bool available) {
    if (!available) {
      return;
    }

    QString sel_text = QLiteHtmlWidget::selectedText();

    if (!sel_text.isEmpty()) {
      QGuiApplication::clipboard()->setText(sel_text, QClipboard::Mode::Selection);
    }
  });
}

void LiteHtmlViewer::bindToBrowser(WebBrowser* browser) {
  browser->m_actionBack = new QAction(this);
  browser->m_actionForward = new QAction(this);
  browser->m_actionReload = new QAction(this);
  browser->m_actionStop = new QAction(this);

  browser->m_actionBack->setEnabled(false);
  browser->m_actionForward->setEnabled(false);
  browser->m_actionReload->setEnabled(false);

  // TODO: When clicked "Stop", save the "Stop" state and return {} from "handleResource(...)"
  // right away.
  browser->m_actionStop->setEnabled(false);

  connect(this, &LiteHtmlViewer::zoomFactorChanged, browser, &WebBrowser::onZoomFactorChanged);
  connect(this, &LiteHtmlViewer::linkHighlighted, browser, [browser](const QUrl& url) {
    browser->onLinkHovered(url.toString());
  });
  connect(this, &LiteHtmlViewer::titleChanged, browser, &WebBrowser::onTitleChanged);
  connect(this, &LiteHtmlViewer::urlChanged, browser, &WebBrowser::updateUrl);
  connect(this, &LiteHtmlViewer::loadStarted, browser, &WebBrowser::onLoadingStarted);
  connect(this, &LiteHtmlViewer::loadProgress, browser, &WebBrowser::onLoadingProgress);
  connect(this, &LiteHtmlViewer::loadFinished, browser, &WebBrowser::onLoadingFinished);
}

void LiteHtmlViewer::findText(const QString& text, bool backwards) {
  QLiteHtmlWidget::findText(text, backwards
                            ? QTextDocument::FindFlag::FindBackward
                            : QTextDocument::FindFlag(0x0), false);
}

void LiteHtmlViewer::setUrl(const QUrl& url) {
  emit loadStarted();
  AdblockRequestInfo block_request(url);

  if (url.path().endsWith(QSL("css"))) {
    block_request.setResourceType(QSL("stylesheet"));
  }
  else {
    block_request.setResourceType(QSL("image"));
  }

  if (qApp->web()->adBlock()->block(block_request).m_blocked) {
    qWarningNN << LOGSEC_ADBLOCK << "Blocked request:" << QUOTE_W_SPACE_DOT(block_request.requestUrl().toString());

    // TODO: Display "site blocked" error.
    return;
  }

  QByteArray output;
  auto net_res = NetworkFactory::performNetworkOperation(
    url.toString(),
    5000,
    {},
    output,
    QNetworkAccessManager::Operation::GetOperation);
  QString html_str;

  if (net_res.m_networkError != QNetworkReply::NetworkError::NoError) {
    html_str = "Error!";
  }
  else {
    html_str = QString::fromUtf8(output);
  }

  setHtml(html_str, url);

  emit loadFinished(net_res.m_networkError == QNetworkReply::NetworkError::NoError);
}

void LiteHtmlViewer::setHtml(const QString& html, const QUrl& base_url) {
  QLiteHtmlWidget::setUrl(base_url);
  QLiteHtmlWidget::setHtml(html);

  emit titleChanged(title());
  emit urlChanged(base_url);
}

QString LiteHtmlViewer::html() const {
  return QLiteHtmlWidget::html();
}

QUrl LiteHtmlViewer::url() const {
  return QLiteHtmlWidget::url();
}

void LiteHtmlViewer::clear() {
  setHtml({});
}

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
  emit loadFinished(true);
}

double LiteHtmlViewer::verticalScrollBarPosition() const {
  return verticalScrollBar()->value();
}

void LiteHtmlViewer::setVerticalScrollBarPosition(double pos) {
  verticalScrollBar()->setValue(pos);
}

void LiteHtmlViewer::reloadFontSettings(const QFont& fon) {
  QLiteHtmlWidget::setDefaultFont(fon);
}

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

QByteArray LiteHtmlViewer::handleResource(const QUrl& url) {
  AdblockRequestInfo block_request(url);

  if (url.path().endsWith(QSL("css"))) {
    block_request.setResourceType(QSL("stylesheet"));
  }
  else {
    block_request.setResourceType(QSL("image"));
  }

  if (qApp->web()->adBlock()->block(block_request).m_blocked) {
    qWarningNN << LOGSEC_ADBLOCK << "Blocked request:" << QUOTE_W_SPACE_DOT(block_request.requestUrl().toString());
    return {};
  }
  else {
    QByteArray output;

    NetworkFactory::performNetworkOperation(
      url.toString(),
      5000,
      {},
      output,
      QNetworkAccessManager::Operation::GetOperation);

    return output;
  }
}

void LiteHtmlViewer::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::StandardKey::Copy)) {
    QString sel_text = QLiteHtmlWidget::selectedText();

    if (!sel_text.isEmpty()) {
      QGuiApplication::clipboard()->setText(sel_text, QClipboard::Mode::Clipboard);
    }
  }

  QLiteHtmlWidget::keyPressEvent(event);
}
