// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"

#include "gui/webbrowser.h"
#include "miscellaneous/application.h"

#include <QBuffer>
#include <QContextMenuEvent>
#include <QFileIconProvider>
#include <QScrollBar>
#include <QTextCodec>
#include <QTimer>
#include <QtConcurrent>

TextBrowserViewer::TextBrowserViewer(QWidget* parent)
  : QTextBrowser(parent), m_document(new TextBrowserDocument(this)) {
  setAutoFillBackground(false);
  setFrameShape(QFrame::Shape::NoFrame);
  setFrameShadow(QFrame::Shadow::Plain);
  setTabChangesFocus(true);
  setOpenLinks(false);
  setWordWrapMode(QTextOption::WrapMode::WordWrap);

  viewport()->setAutoFillBackground(false);
  setDocument(m_document.data());
  setReadOnly(true);

  connect(this, QOverload<const QUrl&>::of(&QTextBrowser::highlighted), this, &TextBrowserViewer::linkMouseHighlighted);
}

TextBrowserViewer::~TextBrowserViewer() {}

QSize TextBrowserViewer::sizeHint() const {
  auto doc_size = document()->size().toSize();

  doc_size.setHeight(doc_size.height() + contentsMargins().top() + contentsMargins().bottom());
  return doc_size;
}

void TextBrowserViewer::reloadNetworkSettings() {}

void TextBrowserViewer::bindToBrowser(WebBrowser* browser) {
  m_browser = browser;

  installEventFilter(browser);
}

void TextBrowserViewer::findText(const QString& text, bool backwards) {
  if (!text.isEmpty()) {
    bool found =
      QTextBrowser::find(text, backwards ? QTextDocument::FindFlag::FindBackward : QTextDocument::FindFlag(0));

    if (!found) {
      textCursor().clearSelection();
      moveCursor(QTextCursor::MoveOperation::Start);

      QTextBrowser::find(text, backwards ? QTextDocument::FindFlag::FindBackward : QTextDocument::FindFlag(0));
    }
  }
  else {
    textCursor().clearSelection();
    moveCursor(QTextCursor::MoveOperation::Start);
  }
}

QString TextBrowserViewer::html() const {
  return QTextBrowser::toHtml();
}

QString TextBrowserViewer::plainText() const {
  return QTextBrowser::toPlainText();
}

QUrl TextBrowserViewer::url() const {
  return QTextBrowser::source();
}

void TextBrowserViewer::clear() {
  setHtml({});
}

QString TextBrowserViewer::htmlForMessage(const Message& messages, RootItem* root) const {
  auto html_message = qApp->skins()->generateHtmlOfArticle(messages, root);

  // Remove other characters which cannot be displayed properly.
  static QRegularExpression exp_symbols("&#x1F[0-9A-F]{3};");

  html_message = html_message.replace(exp_symbols, QString());

  return html_message;
}

void TextBrowserViewer::printToPrinter(QPrinter* printer) {
  QTextBrowser::print(printer);
}

void TextBrowserViewer::cleanupCache() {
  // TODO: TODO
}

double TextBrowserViewer::verticalScrollBarPosition() const {
  return verticalScrollBar()->value();
}

void TextBrowserViewer::setVerticalScrollBarPosition(double pos) {
  verticalScrollBar()->setValue(int(pos));
}

void TextBrowserViewer::applyFont(const QFont& fon) {
  setFont(fon);
  setZoomFactor(zoomFactor());
}

void TextBrowserViewer::reloadPage() {
  QTextBrowser::reload();
}

void TextBrowserViewer::goBack() {
  QTextBrowser::backward();
}

void TextBrowserViewer::goForward() {
  QTextBrowser::forward();
}

qreal TextBrowserViewer::zoomFactor() const {
  return m_zoomFactor;
}

void TextBrowserViewer::setZoomFactor(qreal zoom_factor) {
  m_zoomFactor = zoom_factor;

  auto fon = font();

  fon.setPointSizeF(fon.pointSizeF() * zoom_factor);
  setFont(fon);
}

void TextBrowserViewer::contextMenuEvent(QContextMenuEvent* event) {
  event->accept();

  auto* menu = new QMenu(this);
  menu->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);

  processContextMenu(menu, event);
  menu->popup(event->globalPos());
}

void TextBrowserViewer::loadMessage(const Message& message, RootItem* root) {
  emit loadingStarted();

  auto url = urlForMessage(message, root);
  auto html = htmlForMessage(message, root);

  setHtml(html, url, root);

  emit loadingProgress(50);

  QTextOption op;
  op.setTextDirection((message.m_rtlBehavior == RtlBehavior::Everywhere ||
                       message.m_rtlBehavior == RtlBehavior::EverywhereExceptFeedList ||
                       message.m_rtlBehavior == RtlBehavior::OnlyViewer)
                        ? Qt::LayoutDirection::RightToLeft
                        : Qt::LayoutDirection::LeftToRight);
  document()->setDefaultTextOption(op);

  emit loadingFinished(true);
}

void TextBrowserViewer::loadUrl(const QUrl& url) {
  emit loadingStarted();

  setSource(url, QTextDocument::ResourceType::HtmlResource);

  setZoomFactor(m_zoomFactor);
  setVerticalScrollBarPosition(0.0);

  emit pageTitleChanged(documentTitle());
  emit pageUrlChanged(url);
  emit loadingFinished(true);
}

void TextBrowserViewer::setHtml(const QString& html, const QUrl& url, RootItem* root) {
  QTextBrowser::setHtml(html);

  setZoomFactor(m_zoomFactor);
  setVerticalScrollBarPosition(0.0);

  emit pageTitleChanged(documentTitle());
  emit pageUrlChanged(url);
}

TextBrowserDocument::TextBrowserDocument(TextBrowserViewer* parent) : QTextDocument(parent) {
  m_viewer = parent;
}

QVariant TextBrowserDocument::loadResource(int type, const QUrl& name) {
  return {};
}

ContextMenuData TextBrowserViewer::provideContextMenuData(QContextMenuEvent* event) {
  ContextMenuData c;

  QString anchor = anchorAt(event->pos());

  if (!anchor.isEmpty()) {
    c.m_linkUrl = anchor;
  }

  return c;
}
