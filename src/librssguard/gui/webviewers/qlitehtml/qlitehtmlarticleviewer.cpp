// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qlitehtml/qlitehtmlarticleviewer.h"

#include "definitions/definitions.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "services/abstract/serviceroot.h"

#include <QBuffer>
#include <QClipboard>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QScrollBar>

QLiteHtmlArticleViewer::QLiteHtmlArticleViewer(QWidget* parent) : QLiteHtmlWidget(parent), m_root(nullptr) {
  setAutoFillBackground(false);
  viewport()->setAutoFillBackground(false);
  setFrameShape(QFrame::Shape::NoFrame);
  setFrameShadow(QFrame::Shadow::Plain);

  setLoadExternalResources(qApp->settings()->value(GROUP(Browser), SETTING(Browser::LoadExternalResources)).toBool());

  connect(this, &QLiteHtmlWidget::linkHighlighted, this, &QLiteHtmlArticleViewer::linkMouseHighlighted);
  connect(this, &QLiteHtmlWidget::linkClicked, this, &QLiteHtmlArticleViewer::linkMouseClicked);
}

QLiteHtmlArticleViewer::~QLiteHtmlArticleViewer() {}

void QLiteHtmlArticleViewer::bindToBrowser(WebBrowser* browser) {
  viewport()->installEventFilter(browser);
}

void QLiteHtmlArticleViewer::findText(const QString& text, bool backwards) {
  QLiteHtmlWidget::findText(text,
                            backwards ? QTextDocument::FindFlag::FindBackward : QTextDocument::FindFlag(0),
                            false);
}

void QLiteHtmlArticleViewer::reloadNetworkSettings() {
  documentContainer()->downloader()->reloadSettings();
}

QString QLiteHtmlArticleViewer::html() const {
  return QLiteHtmlWidget::html();
}

QUrl QLiteHtmlArticleViewer::url() const {
  return QLiteHtmlWidget::url();
}

void QLiteHtmlArticleViewer::clear() {
  setHtml({});
}

void QLiteHtmlArticleViewer::loadMessage(const Message& message, RootItem* root) {
  emit loadingStarted();

  auto url = urlForMessage(message, root);
  auto html = htmlForMessage(message, root);

  // NOTE: Sadly, litehtml does not really support RTL, therefore RTL is not supported here either.
  setHtml(html, url, root);

  emit loadingFinished(true);
}

QString QLiteHtmlArticleViewer::htmlForMessage(const Message& message, RootItem* root) const {
  auto html_message = qApp->skins()->generateHtmlOfArticle(message, root);

  // Remove other characters which cannot be displayed properly.
  static QRegularExpression exp_symbols("&#x1F[0-9A-F]{3};");

  html_message = html_message.replace(exp_symbols, QString());

  return html_message;
}

bool QLiteHtmlArticleViewer::loadExternalResources() const {
  return WebViewer::loadExternalResources();
}

void QLiteHtmlArticleViewer::setLoadExternalResources(bool load_resources) {
  WebViewer::setLoadExternalResources(load_resources);
  documentContainer()->setLoadExternalResources(load_resources);
}

double QLiteHtmlArticleViewer::verticalScrollBarPosition() const {
  return verticalScrollBar()->value();
}

void QLiteHtmlArticleViewer::setVerticalScrollBarPosition(double pos) {
  verticalScrollBar()->setValue(int(pos));
}

void QLiteHtmlArticleViewer::applyFont(const QFont& fon) {
  if (defaultFont() == fon) {
    return;
  }

  setDefaultFont(fon);
  setZoomFactor(zoomFactor());
}

qreal QLiteHtmlArticleViewer::zoomFactor() const {
  return QLiteHtmlWidget::zoomFactor();
}

void QLiteHtmlArticleViewer::setZoomFactor(qreal zoom_factor) {
  if (zoomFactor() == zoom_factor) {
    return;
  }

  QLiteHtmlWidget::setZoomFactor(zoom_factor);
}

void QLiteHtmlArticleViewer::setHtml(const QString& html, const QUrl& url, RootItem* root) {
  m_root = root;

  documentContainer()->setNetworkProxy(m_root == nullptr ? QNetworkProxy()
                                                         : m_root->account()->networkProxyForItem(root));

  QLiteHtmlWidget::setUrl(url);
  QLiteHtmlWidget::setHtml(html);

  emit pageTitleChanged(QLiteHtmlWidget::title());
  emit pageUrlChanged(url);
}

ContextMenuData QLiteHtmlArticleViewer::provideContextMenuData(QContextMenuEvent* event) {
  ContextMenuData c;
  QPointF viewport_pos;
  QPointF pos;

  htmlPos(event->pos(), &viewport_pos, &pos);

  QString anchor = documentContainer()->linkAt(pos, viewport_pos).toString();

  if (!anchor.isEmpty()) {
    c.m_linkUrl = anchor;
  }

  QString img_anchor = documentContainer()->imgLinkAt(pos, viewport_pos).toString();

  if (!img_anchor.isEmpty()) {
    c.m_imgLinkUrl = img_anchor;

    auto img = documentContainer()
                 ->handleExternalResource(DocumentContainer::RequestType::ImageDisplay, c.m_imgLinkUrl)
                 .value<QPixmap>();

    if (!img.isNull()) {
      c.m_img = img;
    }
  }

  c.m_selectedText = selectedText();

  return c;
}

void QLiteHtmlArticleViewer::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::StandardKey::Copy)) {
    auto sel_text = selectedText();
    auto* clip = QGuiApplication::clipboard();

    if (!sel_text.isEmpty() && clip != nullptr) {
      clip->setText(selectedText());
    }
  }

  QLiteHtmlWidget::keyPressEvent(event);
}

void QLiteHtmlArticleViewer::contextMenuEvent(QContextMenuEvent* event) {
  event->accept();

  auto* menu = new QMenu(tr("Context menu for article viewer"), this);

  menu->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);

  WebViewer::processContextMenu(menu, event);
  QLiteHtmlWidget::processContextMenu(menu, event);

  menu->popup(event->globalPos());
}
