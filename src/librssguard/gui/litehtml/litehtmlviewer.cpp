// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/litehtml/litehtmlviewer.h"

#include "gui/webbrowser.h"
#include "network-web/networkfactory.h"

#include <QAction>

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
  setHtml(messages.at(0).m_contents);
}

double LiteHtmlViewer::verticalScrollBarPosition() const {
  return {};
}

void LiteHtmlViewer::setVerticalScrollBarPosition(double pos) {}

void LiteHtmlViewer::reloadFontSettings(const QFont& fon) {}

bool LiteHtmlViewer::canZoomIn() const {
  return {};
}

bool LiteHtmlViewer::canZoomOut() const {
  return {};
}

qreal LiteHtmlViewer::zoomFactor() const {
  return {};
}

void LiteHtmlViewer::zoomIn() {}

void LiteHtmlViewer::zoomOut() {}

void LiteHtmlViewer::setZoomFactor(qreal zoom_factor) {}
