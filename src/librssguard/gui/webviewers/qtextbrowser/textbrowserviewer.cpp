// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"

#include "3rd-party/gumbo/src/gumbo.h"
#include "definitions/definitions.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"

#include <cstring>
#include <utility>

#include <QContextMenuEvent>
#include <QCryptographicHash>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QScrollBar>
#include <QTextImageFormat>
#include <QTimer>

QString TextBrowserImageCache::cacheRootFolder() {
  return qApp->web()->webCacheFolder() + QDir::separator() + QSL("images");
}

QString TextBrowserImageCache::cacheFilePath(const QUrl& image_url) {
  const QString host = image_url.host().isEmpty() ? QSL("_unknown") : image_url.host();
  const QByteArray hash =
    QCryptographicHash::hash(image_url.toString(QUrl::ComponentFormattingOption::FullyEncoded).toUtf8(),
                             QCryptographicHash::Algorithm::Md5)
      .toHex();

  return cacheRootFolder() + QDir::separator() + QString(QUrl::toPercentEncoding(host)) + QDir::separator() +
         QString::fromLatin1(hash);
}

bool TextBrowserImageCache::loadImage(const QUrl& image_url, QImage& image) {
  return image.load(cacheFilePath(image_url));
}

bool TextBrowserImageCache::saveImage(const QUrl& image_url, const QByteArray& image_data) {
  const QString file_path = cacheFilePath(image_url);
  const QFileInfo file_info(file_path);

  if (!QDir().mkpath(file_info.absolutePath())) {
    return false;
  }

  QFile file(file_path);

  if (!file.open(QFile::OpenModeFlag::WriteOnly)) {
    return false;
  }

  return file.write(image_data) == image_data.size();
}

void TextBrowserImageCache::cleanup() {
  IOFactory::removeFolder(cacheRootFolder());
}

TextBrowserImageDownloader::TextBrowserImageDownloader(QList<QUrl> image_urls,
                                                       QNetworkProxy custom_proxy,
                                                       QObject* parent)
  : QObject(parent), m_imageUrls(std::move(image_urls)), m_customProxy(std::move(custom_proxy)) {}

void TextBrowserImageDownloader::cancel() {
  m_cancelled = true;
}

void TextBrowserImageDownloader::downloadImages() {
  for (int i = 0; i < m_imageUrls.size(); ++i) {
    if (m_cancelled) {
      emit downloadFinished(false);
      return;
    }

    const QUrl image_url = m_imageUrls.at(i);
    QImage image;

    if (!TextBrowserImageCache::loadImage(image_url, image)) {
      QByteArray image_data;
      NetworkResult network_result =
        NetworkFactory::performNetworkOperation(image_url.toString(),
                                                10000,
                                                {},
                                                image_data,
                                                QNetworkAccessManager::Operation::GetOperation,
                                                {},
                                                false,
                                                {},
                                                {},
                                                m_customProxy);

      if (network_result.m_networkError == QNetworkReply::NetworkError::NoError) {
        image.loadFromData(image_data);

        if (!image.isNull()) {
          TextBrowserImageCache::saveImage(image_url, image_data);
        }
      }
    }

    if (!image.isNull()) {
      emit imageDownloaded(image_url, image);
    }

    emit downloadProgress(((i + 1) * 100) / m_imageUrls.size());
  }

  emit downloadFinished(true);
}

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

  connect(this, &TextBrowserViewer::anchorClicked, this, [this](const QUrl& url) {
    emit linkMouseClicked(url);
  });
  connect(this, QOverload<const QUrl&>::of(&QTextBrowser::highlighted), this, &TextBrowserViewer::linkMouseHighlighted);
}

TextBrowserViewer::~TextBrowserViewer() {
  abortImageDownloading();

  if (!m_imageDownloadThread.isNull()) {
    m_imageDownloadThread->wait();
  }
}

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

          if (attr_alt && attr_alt->value && strlen(attr_alt->value) > 0) {
            link_text = QString::fromUtf8(attr_alt->value).toHtmlEscaped();
          }
          else if (attr_title && attr_title->value && strlen(attr_title->value) > 0) {
            link_text = QString::fromUtf8(attr_title->value).toHtmlEscaped();
          }
          else {
            QUrl url(src);

            if (url.isValid() && !url.fileName().trimmed().isEmpty()) {
              link_text = url.fileName().toHtmlEscaped();
            }
            else if (url.isValid() && !url.host().isEmpty()) {
              link_text = url.host().toHtmlEscaped();
            }
          }

          out += QSL("<br/><a href=\"%1\">🖼️ %2 - %3</a><br/>").arg(href, QObject::tr("image"), link_text);
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

static void collectImageUrlsFromGumboNode(GumboNode* node, QList<QString>& image_urls) {
  if (node == nullptr || node->type != GUMBO_NODE_ELEMENT) {
    return;
  }

  GumboElement& el = node->v.element;

  if (el.tag == GUMBO_TAG_IMG) {
    GumboAttribute* attr_src = gumbo_get_attribute(&el.attributes, "src");

    if (attr_src != nullptr && attr_src->value != nullptr) {
      image_urls.append(QString::fromUtf8(attr_src->value));
    }
  }

  GumboVector* children = &el.children;

  for (unsigned int i = 0; i < children->length; ++i) {
    collectImageUrlsFromGumboNode(static_cast<GumboNode*>(children->data[i]), image_urls);
  }
}

QString TextBrowserViewer::htmlToDisplay(const QString& html) const {
  return loadExternalResources() ? html : convertToHtmlWithoutImages(html);
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

  return result;
}

QString TextBrowserViewer::htmlForMessage(const Message& message, RootItem* root) const {
  auto html_message = WebViewer::htmlForMessage(message, root);

  // Remove other characters which cannot be displayed properly.
  static QRegularExpression exp_symbols("&#x1F[0-9A-F]{3};");

  html_message = html_message.replace(exp_symbols, QString());

  return html_message;
}

void TextBrowserViewer::printToPrinter(QPrinter* printer) {
  QTextBrowser::print(printer);
  onPrintingFinished(true);
}

void TextBrowserViewer::cleanupCache() {
  abortImageDownloading();
  m_downloadedImages.clear();
  TextBrowserImageCache::cleanup();
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

QString TextBrowserViewer::imageCssMaxHeight(int height) const {
  return QSL("height=\"%1\"").arg(height);
}

void TextBrowserViewer::goBack() {
  QTextBrowser::backward();
}

void TextBrowserViewer::goForward() {
  QTextBrowser::forward();
}

bool TextBrowserViewer::supportImagesLoading() const {
  return true;
}

bool TextBrowserViewer::supportsNavigation() const {
  return false;
}

void TextBrowserViewer::setLoadExternalResources(bool load_resources) {
  WebViewer::setLoadExternalResources(load_resources);

  if (m_currentHtml.isEmpty()) {
    return;
  }

  abortImageDownloading();
  m_downloadedImages.clear();

  emit loadingStarted();
  justSetHtml(htmlToDisplay(m_currentHtml), m_currentUrl, m_root.data(), true);

  if (!load_resources || !startImageDownloading()) {
    emit loadingProgress(100);
    emit loadingFinished(true);
  }
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

  m_root = root;

  auto url = urlForMessage(message, root);
  auto html = htmlForMessage(message, root);

  bool downloads_started = loadStaticHtml(html, url, root);

  QTextOption op;
  op.setTextDirection((message.m_rtlBehavior == RtlBehavior::Everywhere ||
                       message.m_rtlBehavior == RtlBehavior::EverywhereExceptFeedList ||
                       message.m_rtlBehavior == RtlBehavior::OnlyViewer)
                        ? Qt::LayoutDirection::RightToLeft
                        : Qt::LayoutDirection::LeftToRight);
  document()->setDefaultTextOption(op);

  if (!downloads_started) {
    emit loadingProgress(100);
    emit loadingFinished(true);
  }
}

void TextBrowserViewer::displayDownloadedPage(const QUrl& url, const QByteArray& data, const NetworkResult& res) {
  if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
    if (res.m_contentType.startsWith(QSL("image"))) {
      if (!loadExternalResources()) {
        emit openUrlInNewTab(true, url);
      }
      else {
        const QString image_data = QString::fromLatin1(data.toBase64());
        const QString html =
          QSL("<html><body><img src=\"data:%1;base64,%2\"></body></html>").arg(res.m_contentType, image_data);

        loadStaticHtml(html, url);
      }
    }
    else if (res.m_contentType.contains(QSL("xml"))) {
      QDomDocument dom;

      if (dom.setContent(data)) {
        loadStaticHtml(Qt::convertFromPlainText(dom.toString(2)), url);
      }
      else {
        loadStaticHtml(QString::fromUtf8(data).toHtmlEscaped(), url);
      }
    }
    else if (res.m_contentType.contains(QSL("html"))) {
      bool no_images = loadStaticHtml(QString::fromUtf8(data), url);

      if (url.hasFragment()) {
        scrollToAnchor(url.fragment());
      }

      if (no_images) {
        return;
      }
    }
    else {
      loadStaticHtml(Qt::convertFromPlainText(QString::fromUtf8(data)), url);
    }
  }
  else {
    QString error = tr("The page cannot be loaded with HTTP/%1 error.").arg(QString::number(res.m_httpCode));

    if (!data.isEmpty()) {
      error += QSL("\n\n");
      error += QString::fromUtf8(data);
    }

    loadStaticHtml(Qt::convertFromPlainText(error), url);
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
  auto download_res =
    NetworkFactory::performNetworkOperation(url.toString(),
                                            5000,
                                            {},
                                            output,
                                            QNetworkAccessManager::Operation::GetOperation,
                                            {},
                                            {},
                                            {},
                                            {},
                                            m_root.isNull() ? QNetworkProxy::ProxyType::DefaultProxy
                                                            : m_root->account()->networkProxyForItem(m_root.data()));

  displayDownloadedPage(url, output, download_res);
}

void TextBrowserViewer::setHtml(const QString& html, const QUrl& url, RootItem* root) {
  emit loadingStarted();

  if (!loadStaticHtml(html, url, root)) {
    emit loadingProgress(100);
    emit loadingFinished(true);
  }
}

bool TextBrowserViewer::loadStaticHtml(const QString& html, const QUrl& url, RootItem* root) {
  abortImageDownloading();

  m_currentHtml = html;
  m_downloadedImages.clear();

  justSetHtml(htmlToDisplay(html), url, root);

  if (loadExternalResources()) {
    return startImageDownloading();
  }

  return false;
}

void TextBrowserViewer::justSetHtml(const QString& html, const QUrl& url, RootItem* root, bool keep_scroll) {
  const double scroll_position = verticalScrollBarPosition();

  m_currentUrl = url;
  m_root = root;

  document()->setBaseUrl(url);

  QTextBrowser::setHtml(html);

  setZoomFactor(m_zoomFactor);
  setVerticalScrollBarPosition(keep_scroll ? scroll_position : 0.0);

  emit pageTitleChanged(documentTitle());
  emit pageUrlChanged(url);
}

void TextBrowserViewer::abortImageDownloading() {
  if (!m_imageDownloader.isNull()) {
    m_imageDownloader->cancel();
    m_imageDownloader->disconnect(this);
  }

  if (!m_imageDownloadThread.isNull()) {
    m_imageDownloadThread->quit();
  }
}

bool TextBrowserViewer::startImageDownloading() {
  const QList<QUrl> image_urls = imageUrlsForHtml(m_currentHtml, m_currentUrl);

  if (image_urls.isEmpty()) {
    return false;
  }

  emit loadingProgress(0);

  auto* thread = new QThread(this);
  auto* downloader = new TextBrowserImageDownloader(image_urls, networkProxyForCurrentRoot());

  m_imageDownloadThread = thread;
  m_imageDownloader = downloader;

  downloader->moveToThread(thread);

  connect(thread, &QThread::started, downloader, &TextBrowserImageDownloader::downloadImages);
  connect(downloader, &TextBrowserImageDownloader::downloadFinished, thread, &QThread::quit);
  connect(downloader,
          &TextBrowserImageDownloader::imageDownloaded,
          this,
          [this, downloader](const QUrl& image_url, const QImage& image) {
            if (m_imageDownloader != downloader) {
              return;
            }

            m_downloadedImages.insert(image_url, image);
          });
  connect(downloader, &TextBrowserImageDownloader::downloadProgress, this, [this, downloader](int progress) {
    if (m_imageDownloader != downloader) {
      return;
    }

    emit loadingProgress(progress);
  });
  connect(downloader, &TextBrowserImageDownloader::downloadFinished, this, [this, downloader](bool success) {
    if (m_imageDownloader != downloader) {
      return;
    }

    reloadHtmlWithCachedImages();

    emit loadingProgress(100);
    emit loadingFinished(success);

    m_imageDownloader = nullptr;
    m_imageDownloadThread = nullptr;
  });
  connect(thread, &QThread::finished, downloader, &QObject::deleteLater);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);

  thread->start();
  return true;
}

void TextBrowserViewer::reloadHtmlWithCachedImages() {
  justSetHtml(htmlToDisplay(m_currentHtml), m_currentUrl, m_root.data(), true);
}

QList<QUrl> TextBrowserViewer::imageUrlsForHtml(const QString& html, const QUrl& base_url) const {
  QList<QString> raw_image_urls;
  QByteArray utf8 = html.toUtf8();
  GumboOutput* output = gumbo_parse(utf8.constData());

  collectImageUrlsFromGumboNode(output->root, raw_image_urls);
  gumbo_destroy_output(&kGumboDefaultOptions, output);

  QList<QUrl> image_urls;

  for (const QString& raw_image_url : std::as_const(raw_image_urls)) {
    const QUrl image_url(raw_image_url);
    const QUrl resolved_image_url = image_url.isRelative() ? base_url.resolved(image_url) : image_url;

    if (!resolved_image_url.isValid()) {
      continue;
    }

    const QString scheme = resolved_image_url.scheme().toLower();

    if (scheme != QSL("http") && scheme != QSL("https") && scheme != QSL("gemini")) {
      continue;
    }

    if (!image_urls.contains(resolved_image_url)) {
      image_urls.append(resolved_image_url);
    }
  }

  return image_urls;
}

QUrl TextBrowserViewer::resolvedResourceUrl(const QUrl& resource_url) const {
  if (resource_url.isRelative()) {
    return m_currentUrl.resolved(resource_url);
  }
  else {
    return resource_url;
  }
}

QUrl TextBrowserViewer::imageUrlAt(const QPoint& pos) const {
  QTextCursor cursor = cursorForPosition(pos);
  QUrl image_url = imageUrlFromCursor(cursor);

  if (image_url.isValid()) {
    return image_url;
  }

  if (cursor.position() > 0) {
    QTextCursor previous_cursor(cursor);

    previous_cursor.movePosition(QTextCursor::MoveOperation::PreviousCharacter);
    image_url = imageUrlFromCursor(previous_cursor);

    if (image_url.isValid()) {
      return image_url;
    }
  }

  QTextCursor next_cursor(cursor);

  if (next_cursor.movePosition(QTextCursor::MoveOperation::NextCharacter)) {
    image_url = imageUrlFromCursor(next_cursor);
  }

  return image_url;
}

QUrl TextBrowserViewer::imageUrlFromCursor(const QTextCursor& cursor) const {
  const QTextCharFormat format = cursor.charFormat();

  if (!format.isImageFormat()) {
    return {};
  }

  const QTextImageFormat image_format = format.toImageFormat();
  const QUrl image_url(image_format.name());

  return image_url.isValid() ? resolvedResourceUrl(image_url) : QUrl();
}

QNetworkProxy TextBrowserViewer::networkProxyForCurrentRoot() const {
  return m_root.isNull() ? QNetworkProxy::ProxyType::DefaultProxy
                         : m_root->account()->networkProxyForItem(m_root.data());
}

TextBrowserDocument::TextBrowserDocument(TextBrowserViewer* parent) : QTextDocument(parent) {
  m_viewer = parent;
}

QVariant TextBrowserDocument::loadResource(int type, const QUrl& name) {
  if (m_viewer.isNull()) {
    return QTextDocument::loadResource(type, name);
  }

  if (QTextDocument::ResourceType(type) != QTextDocument::ResourceType::ImageResource) {
    return QTextDocument::loadResource(type, name);
  }

  const QUrl image_url = m_viewer->resolvedResourceUrl(name);
  const auto image = m_viewer->m_downloadedImages.constFind(image_url);

  if (image != m_viewer->m_downloadedImages.constEnd()) {
    return image.value();
  }

  const QString scheme = image_url.scheme().toLower();

  if (scheme == QSL("http") || scheme == QSL("https") || scheme == QSL("gemini")) {
    if (m_viewer->loadExternalResources() && m_viewer->m_imageDownloader.isNull()) {
      return m_viewer->m_placeholderImageError;
    }
    else {
      return m_viewer->m_placeholderImage;
    }
  }

  return QTextDocument::loadResource(type, name);
}

ContextMenuData TextBrowserViewer::provideContextMenuData(QContextMenuEvent* event) {
  ContextMenuData c;

  c.m_linkUrl = anchorAt(event->pos());
  c.m_imgLinkUrl = imageUrlAt(event->pos());
  c.m_selectedText = textCursor().selectedText();

  return c;
}
