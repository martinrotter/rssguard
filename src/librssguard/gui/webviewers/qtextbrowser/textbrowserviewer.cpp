// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"

#include "3rd-party/gumbo/src/gumbo.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"

#include <QBuffer>
#include <QContextMenuEvent>
#include <QFileIconProvider>
#include <QScrollBar>
#include <QTextDocumentFragment>
#include <QTimer>
#include <QtConcurrent>

TextBrowserViewer::TextBrowserViewer(QWidget* parent)
  : QTextBrowser(parent), m_document(new TextBrowserDocument(this)) {
  setAutoFillBackground(false);
  setFrameShape(QFrame::Shape::NoFrame);
  setFrameShadow(QFrame::Shadow::Plain);
  setTabChangesFocus(true);
  setOpenLinks(false);
  setOpenExternalLinks(false);
  setWordWrapMode(QTextOption::WrapMode::WordWrap);

  viewport()->setAutoFillBackground(false);
  setDocument(m_document.data());
  setReadOnly(true);

  connect(this, &TextBrowserViewer::anchorClicked, this, &TextBrowserViewer::linkMouseClicked);
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
  return m_currentUrl;
}

void TextBrowserViewer::clear() {
  setHtml({});
}

QString TextBrowserViewer::htmlForMessage(const Message& message, RootItem* root) const {
  auto html_message = qApp->skins()->generateHtmlOfArticle(message, root);

  // html_message = QTextDocumentFragment().fromHtml(html_message).toPlainText();

  // Remove other characters which cannot be displayed properly.
  static QRegularExpression exp_symbols("&#x1F[0-9A-F]{3};");

  html_message = html_message.replace(exp_symbols, QString());

  return html_message;
}

void TextBrowserViewer::printToPrinter(QPrinter* printer) {
  QTextBrowser::print(printer);
}

void TextBrowserViewer::cleanupCache() {
  // IOFactory::removeFolder(qApp->web()->webCacheFolder());
}

double TextBrowserViewer::verticalScrollBarPosition() const {
  return verticalScrollBar()->value();
}

void TextBrowserViewer::setVerticalScrollBarPosition(double pos) {
  verticalScrollBar()->setValue(int(pos));
}

void TextBrowserViewer::applyFont(const QFont& fon) {
  m_baseFont = fon;

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

bool TextBrowserViewer::supportImagesLoading() const {
  return false;
}

qreal TextBrowserViewer::zoomFactor() const {
  return m_zoomFactor;
}

void TextBrowserViewer::setZoomFactor(qreal zoom_factor) {
  m_zoomFactor = zoom_factor;

  auto fon = font();

  fon.setPointSizeF(m_baseFont.pointSizeF() * zoom_factor);
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

  QByteArray output;
  auto download_res = NetworkFactory::performNetworkOperation(url.toString(),
                                                              5000,
                                                              {},
                                                              output,
                                                              QNetworkAccessManager::Operation::GetOperation);

  setHtml(QString::fromUtf8(output), url);

  emit loadingFinished(download_res.m_networkError == QNetworkReply::NetworkError::NoError);
}

// --- helper: HTML escape ---
static QString escapeHtml(const QString& input) {
  QString s = input;
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  return s;
}

// --- helper: void elements ---
static bool isVoidTag(GumboTag tag) {
  switch (tag) {
    case GUMBO_TAG_AREA:
    case GUMBO_TAG_BASE:
    case GUMBO_TAG_BR:
    case GUMBO_TAG_COL:
    case GUMBO_TAG_EMBED:
    case GUMBO_TAG_HR:
    case GUMBO_TAG_IMG:
    case GUMBO_TAG_INPUT:
    case GUMBO_TAG_LINK:
    case GUMBO_TAG_META:
    case GUMBO_TAG_PARAM:
    case GUMBO_TAG_SOURCE:
    case GUMBO_TAG_TRACK:
    case GUMBO_TAG_WBR:
      return true;
    default:
      return false;
  }
}

// --- recursive processing ---
static void processNode(GumboNode* node, QString& out) {
  if (!node) {
    return;
  }

  switch (node->type) {
    case GUMBO_NODE_TEXT:
      out += escapeHtml(QString::fromUtf8(node->v.text.text));
      break;

    case GUMBO_NODE_ELEMENT: {
      GumboElement& el = node->v.element;

      // 🔴 replace <img>
      if (el.tag == GUMBO_TAG_IMG) {
        GumboAttribute* src = gumbo_get_attribute(&el.attributes, "src");

        if (src && src->value) {
          QString url = escapeHtml(QString::fromUtf8(src->value));
          out += "<a href=\"" + url + "\">" + url + "</a>";
        }
        return;
      }

      const char* tagName = gumbo_normalized_tagname(el.tag);

      if (tagName && *tagName) {
        out += "<";
        out += tagName;

        // attributes
        GumboVector* attrs = &el.attributes;
        for (unsigned int i = 0; i < attrs->length; ++i) {
          auto* attr = static_cast<GumboAttribute*>(attrs->data[i]);
          out += " ";
          out += attr->name;
          out += "=\"";
          out += escapeHtml(QString::fromUtf8(attr->value));
          out += "\"";
        }

        out += ">";
      }

      // children
      GumboVector* children = &el.children;
      for (unsigned int i = 0; i < children->length; ++i) {
        processNode(static_cast<GumboNode*>(children->data[i]), out);
      }

      // closing tag
      if (tagName && *tagName && !isVoidTag(el.tag)) {
        out += "</";
        out += tagName;
        out += ">";
      }

      break;
    }

    default:
      break;
  }
}

// --- main function ---
QString replaceImagesWithLinks(const QString& html) {
  QByteArray utf8 = html.toUtf8();

  GumboOutput* output = gumbo_parse(utf8.constData());
  QString result;

  // gumbo wraps fragment into html > body
  GumboNode* root = output->root;

  if (root->type == GUMBO_NODE_ELEMENT) {
    GumboVector* rootChildren = &root->v.element.children;

    // najdi <body>
    for (unsigned int i = 0; i < rootChildren->length; ++i) {
      GumboNode* node = static_cast<GumboNode*>(rootChildren->data[i]);

      if (node->type == GUMBO_NODE_ELEMENT && node->v.element.tag == GUMBO_TAG_BODY) {
        GumboVector* bodyChildren = &node->v.element.children;

        for (unsigned int j = 0; j < bodyChildren->length; ++j) {
          processNode(static_cast<GumboNode*>(bodyChildren->data[j]), result);
        }
        break;
      }
    }
  }

  gumbo_destroy_output(&kGumboDefaultOptions, output);
  return result;
}

void TextBrowserViewer::setHtml(const QString& html, const QUrl& url, RootItem* root) {
  m_currentUrl = url;

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
  if (QTextDocument::ResourceType(type) = QTextDocument::ResourceType::ImageResource) {
    return m_viewer->m_placeholderImage;
  }
  else {
    return {};
  }
}

ContextMenuData TextBrowserViewer::provideContextMenuData(QContextMenuEvent* event) {
  ContextMenuData c;

  c.m_linkUrl = anchorAt(event->pos());
  c.m_selectedText = textCursor().selectedText();

  return c;
}
