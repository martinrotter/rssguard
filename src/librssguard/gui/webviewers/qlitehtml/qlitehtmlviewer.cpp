// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qlitehtml/qlitehtmlviewer.h"

#include "definitions/definitions.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "services/abstract/serviceroot.h"

#include <QBuffer>
#include <QClipboard>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QScrollBar>

QLiteHtmlViewer::QLiteHtmlViewer(QWidget* parent)
  : QLiteHtmlWidget(parent), m_root(nullptr),
    m_placeholderImage(IconFactory::toByteArray(qApp->icons()->miscPixmap(QSL("image-placeholder")), QSL("PNG"))),
    m_placeholderImageError(IconFactory::toByteArray(qApp->icons()->miscPixmap(QSL("image-placeholder-error")),
                                                     QSL("PNG"))) {
  setAutoFillBackground(false);
  viewport()->setAutoFillBackground(false);
  setFrameShape(QFrame::Shape::NoFrame);
  setFrameShadow(QFrame::Shadow::Plain);
  setFontAntialiasing(true);

  horizontalScrollBar()->setSingleStep(5);
  verticalScrollBar()->setSingleStep(5);

  connect(this, &QLiteHtmlWidget::linkHighlighted, this, &QLiteHtmlViewer::linkMouseHighlighted);
  connect(this, &QLiteHtmlWidget::linkClicked, this, &QLiteHtmlViewer::linkClicked);

  setResourceHandler([this](const QUrl& url) {
    return handleExternalResource(url);
  });
}

QLiteHtmlViewer::~QLiteHtmlViewer() {}

void QLiteHtmlViewer::bindToBrowser(WebBrowser* browser) {
  viewport()->installEventFilter(browser);
}

void QLiteHtmlViewer::findText(const QString& text, bool backwards) {
  QLiteHtmlWidget::findText(text,
                            backwards ? QTextDocument::FindFlag::FindBackward : QTextDocument::FindFlag(0),
                            false);
}

void QLiteHtmlViewer::reloadNetworkSettings() {
  // m_network->loadSettings();
}

QString QLiteHtmlViewer::html() const {
  return QLiteHtmlWidget::html();
}

QUrl QLiteHtmlViewer::url() const {
  return QLiteHtmlWidget::url();
}

void QLiteHtmlViewer::clear() {
  setHtml({});
}

void QLiteHtmlViewer::loadMessage(const Message& message, RootItem* root) {
  emit loadingStarted();

  auto url = urlForMessage(message, root);
  auto html = htmlForMessage(message, root);

  setHtml(html, url, root);

  // TODO: RTL

  emit loadingFinished(true);
}

QString QLiteHtmlViewer::htmlForMessage(const Message& message, RootItem* root) const {
  auto html_message = qApp->skins()->generateHtmlOfArticle(message, root);

  // Remove other characters which cannot be displayed properly.
  static QRegularExpression exp_symbols("&#x1F[0-9A-F]{3};");

  html_message = html_message.replace(exp_symbols, QString());

  return html_message;
}

double QLiteHtmlViewer::verticalScrollBarPosition() const {
  return verticalScrollBar()->value();
}

void QLiteHtmlViewer::setVerticalScrollBarPosition(double pos) {
  verticalScrollBar()->setValue(int(pos));
}

void QLiteHtmlViewer::applyFont(const QFont& fon) {
  if (defaultFont() == fon) {
    return;
  }

  setDefaultFont(fon);
  setZoomFactor(zoomFactor());
}

qreal QLiteHtmlViewer::zoomFactor() const {
  return QLiteHtmlWidget::zoomFactor();
}

void QLiteHtmlViewer::setZoomFactor(qreal zoom_factor) {
  if (zoomFactor() == zoom_factor) {
    return;
  }

  QLiteHtmlWidget::setZoomFactor(zoom_factor);
}

QByteArray QLiteHtmlViewer::handleExternalResource(const QUrl& url) {
  // TODO: if image is NOT in cache, download it async and return placeholder
  // once image is downloaded, call render() to re-render the page.

  if (!loadExternalResources()) {
    return m_placeholderImage;
  }

  if (m_imageCache.contains(url)) {
    qDebugNN << LOGSEC_HTMLVIEWER << "Loading image" << QUOTE_W_SPACE(url.toString()) << "from cache.";
    return m_imageCache.value(url);
  }

  QEventLoop loop;
  QByteArray data;

  NetworkResult res =
    NetworkFactory::performNetworkOperation(url.toString(),
                                            5000,
                                            {},
                                            data,
                                            QNetworkAccessManager::Operation::GetOperation,
                                            {},
                                            false,
                                            {},
                                            {},
                                            m_root == nullptr ? QNetworkProxy()
                                                              : m_root->getParentServiceRoot()->networkProxy());

  if (res.m_networkError != QNetworkReply::NetworkError::NoError) {
    qWarningNN << LOGSEC_HTMLVIEWER << "Image" << QUOTE_W_SPACE(url.toString()) << "was not loaded due to error"
               << QUOTE_W_SPACE_DOT(res.m_networkError);
  }

  if (data.isEmpty()) {
    data = m_placeholderImageError;
  }
  else {
    qDebugNN << LOGSEC_HTMLVIEWER << "Inserting image" << QUOTE_W_SPACE(url.toString()) << "to cache.";
    m_imageCache.insert(url, data);
  }

  return data;
}

void QLiteHtmlViewer::setHtml(const QString& html, const QUrl& url, RootItem* root) {
  m_root = root;

  QLiteHtmlWidget::setUrl(url);
  QLiteHtmlWidget::setHtml(html);

  emit pageTitleChanged(QLiteHtmlWidget::title());
  emit pageUrlChanged(url);
}

ContextMenuData QLiteHtmlViewer::provideContextMenuData(QContextMenuEvent* event) const {
  ContextMenuData c;
  QPoint viewportPos;
  QPoint pos;

  htmlPos(event->pos(), &viewportPos, &pos);

  QString anchor = documentContainer()->linkAt(pos, viewportPos).toString();

  if (!anchor.isEmpty()) {
    c.m_linkUrl = anchor;
  }

  return c;
}

void QLiteHtmlViewer::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::StandardKey::Copy)) {
    auto sel_text = selectedText();
    auto* clip = QGuiApplication::clipboard();

    if (!sel_text.isEmpty() && clip != nullptr) {
      clip->setText(selectedText());
    }
  }

  QLiteHtmlWidget::keyPressEvent(event);
}

void QLiteHtmlViewer::contextMenuEvent(QContextMenuEvent* event) {
  event->accept();

  auto* menu = new QMenu(tr("Context menu for article viewer"), this);
  auto sel_text = selectedText();
  auto* act_copy = new QAction(qApp->icons()->fromTheme(QSL("edit-copy")), tr("Copy"), this);

  act_copy->setShortcut(QKeySequence(QKeySequence::StandardKey::Copy));
  act_copy->setEnabled(!sel_text.isEmpty());

  connect(act_copy, &QAction::triggered, this, [sel_text]() {
    auto* clip = QGuiApplication::clipboard();

    if (clip != nullptr) {
      clip->setText(sel_text);
    }
  });

  menu->addAction(act_copy);

  /*
  if (m_actionEnableResources.isNull()) {
    m_actionEnableResources.reset(new QAction(qApp->icons()->fromTheme(QSL("viewimage"), QSL("image-x-generic")),
                                              tr("Enable external resources"),
                                              this));

    m_actionEnableResources.data()->setCheckable(true);
    m_actionEnableResources.data()->setChecked(resourcesEnabled());

    connect(m_actionEnableResources.data(), &QAction::toggled, this, &TextBrowserViewer::enableResources);
  }

 menu->addAction(m_actionEnableResources.data());

  */

  processContextMenu(menu, event);
  menu->popup(event->globalPos());
}
