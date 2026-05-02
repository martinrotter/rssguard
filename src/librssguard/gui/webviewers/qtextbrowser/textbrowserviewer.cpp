// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"

#include "3rd-party/gumbo/src/gumbo.h"
#include "definitions/definitions.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"

#include <QBuffer>
#include <QContextMenuEvent>
#include <QDomDocument>
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

static bool isGumboVoidTag(GumboTag tag) {
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

static void processGumboNode(GumboNode* node, QString& out) {
  if (!node) {
    return;
  }

  switch (node->type) {
    case GUMBO_NODE_TEXT:
      out += QString::fromUtf8(node->v.text.text).toHtmlEscaped();
      break;

    case GUMBO_NODE_ELEMENT: {
      GumboElement& el = node->v.element;

      if (el.tag == GUMBO_TAG_IMG) {
        GumboAttribute* attr_src = gumbo_get_attribute(&el.attributes, "src");
        GumboAttribute* attr_alt = gumbo_get_attribute(&el.attributes, "alt");
        GumboAttribute* attr_title = gumbo_get_attribute(&el.attributes, "title");

        if (attr_src && attr_src->value) {
          QString src = QString::fromUtf8(attr_src->value);
          QString href = src.toHtmlEscaped();
          QString link_text;

          if (attr_alt && attr_alt->value && QString::fromUtf8(attr_alt->value).trimmed().size() > 0) {
            link_text = QString::fromUtf8(attr_alt->value).toHtmlEscaped();
          }
          else if (attr_title && attr_title->value && QString::fromUtf8(attr_title->value).trimmed().size() > 0) {
            link_text = QString::fromUtf8(attr_title->value).toHtmlEscaped();
          }
          else {
            QUrl url(src);

            if (url.isValid() && !url.fileName().trimmed().isEmpty()) {
              link_text = url.fileName().toHtmlEscaped();
            }
            else if (url.isValid() && !url.host().isEmpty()) {
              link_text = QObject::tr("image") + QSL(" - ") + url.host().toHtmlEscaped();
            }
            else {
              link_text = QObject::tr("image");
            }
          }

          out += QSL("<br/><a href=\"%1\">🖼️ %2</a><br/>").arg(href, link_text);
        }

        return;
      }

      const char* tag_name = gumbo_normalized_tagname(el.tag);

      if (tag_name && *tag_name) {
        out += QSL("<");
        out += tag_name;

        GumboVector* attrs = &el.attributes;

        for (unsigned int i = 0; i < attrs->length; ++i) {
          auto* attr = static_cast<GumboAttribute*>(attrs->data[i]);
          out += QSL(" ");
          out += attr->name;
          out += QSL("=\"");
          out += QString::fromUtf8(attr->value).toHtmlEscaped();
          out += QSL("\"");
        }

        out += QSL(">");
      }

      GumboVector* children = &el.children;

      for (unsigned int i = 0; i < children->length; ++i) {
        processGumboNode(static_cast<GumboNode*>(children->data[i]), out);
      }

      if (tag_name && *tag_name && !isGumboVoidTag(el.tag)) {
        out += QSL("</");
        out += tag_name;
        out += QSL(">");
      }

      break;
    }

    default:
      break;
  }
}

QString TextBrowserViewer::convertToHtmlWithoutImages(const QString& html) const {
  if (!TextFactory::couldBeHtml(html)) {
    return html;
  }

  QByteArray utf8 = html.toUtf8();
  GumboOutput* output = gumbo_parse(utf8.constData());
  QString result;
  GumboNode* root = output->root;

  processGumboNode(root, result);

  gumbo_destroy_output(&kGumboDefaultOptions, output);

  // IOFactory::debugWriteFile("orig.html", html.toUtf8());
  // IOFactory::debugWriteFile("new.html", result.toUtf8());

  return result;
}

QString TextBrowserViewer::htmlForMessage(const Message& message, RootItem* root) const {
  auto editable_message = message;

  editable_message.m_contents = convertToHtmlWithoutImages(editable_message.m_contents);

  auto html_message = qApp->skins()->generateHtmlOfArticle(editable_message, root);

  // html_message = QTextDocumentFragment().fromHtml(html_message).toPlainText();

  // Remove other characters which cannot be displayed properly.
  static QRegularExpression exp_symbols("&#x1F[0-9A-F]{3};");

  html_message = html_message.replace(exp_symbols, QString());

  return html_message;
}

void TextBrowserViewer::printToPrinter(QPrinter* printer) {
  QTextBrowser::print(printer);
  onPrintingFinished(true);
}

void TextBrowserViewer::cleanupCache() {}

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

bool TextBrowserViewer::supportsNavigation() const {
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

  justSetHtml(html, url, root);

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

void TextBrowserViewer::displayDownloadedPage(const QUrl& url, const QByteArray& data, const NetworkResult& res) {
  if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
    if (res.m_contentType.startsWith(QSL("image"))) {
      emit openUrlInNewTab(true, url);
    }
    else if (res.m_contentType.contains(QSL("xml"))) {
      QDomDocument dom;

      if (dom.setContent(data)) {
        setHtml(Qt::convertFromPlainText(dom.toString(2)), url);
      }
      else {
        setHtml(QString::fromUtf8(data).toHtmlEscaped(), url);
      }
    }
    else {
      setHtml(QString::fromUtf8(data), url);

      if (url.hasFragment()) {
        scrollToAnchor(url.fragment());
      }
    }
  }
  else {
    QString error = tr("The page cannot be loaded with HTTP/%1 error.").arg(QString::number(res.m_httpCode));

    if (!data.isEmpty()) {
      error += QSL("\n\n");
      error += QString::fromUtf8(data);
    }

    setHtml(Qt::convertFromPlainText(error), url);
  }

  emit loadingFinished(res.m_networkError == QNetworkReply::NetworkError::NoError);
}

void TextBrowserViewer::loadUrl(const QUrl& url) {
  if (url.isRelative() && url.scheme().isEmpty() && url.path().isEmpty() && url.hasFragment()) {
    // NOTE: We only scroll to anchor.
    scrollToAnchor(url.fragment());

    auto new_url = this->url();
    new_url.setFragment(url.fragment());

    emit pageUrlChanged(new_url);
    return;
  }

  emit loadingStarted();

  QByteArray output;
  auto download_res = NetworkFactory::performNetworkOperation(url.toString(),
                                                              5000,
                                                              {},
                                                              output,
                                                              QNetworkAccessManager::Operation::GetOperation);

  displayDownloadedPage(url, output, download_res);
}

void TextBrowserViewer::setHtml(const QString& html, const QUrl& url, RootItem* root) {
  justSetHtml(convertToHtmlWithoutImages(html), url, root);
}

void TextBrowserViewer::justSetHtml(const QString& html, const QUrl& url, RootItem* root) {
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
