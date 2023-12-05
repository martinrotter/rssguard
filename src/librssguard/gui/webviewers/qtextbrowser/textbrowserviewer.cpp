// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/globals.h"
#include "gui/dialogs/formmain.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/downloader.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"

#include <QContextMenuEvent>
#include <QFileIconProvider>
#include <QScrollBar>
#include <QTimer>

TextBrowserViewer::TextBrowserViewer(QWidget* parent)
  : QTextBrowser(parent), m_resourcesEnabled(false), m_resourceDownloader(new Downloader(this)), m_loadedResources({}),
    m_placeholderImage(qApp->icons()->miscPixmap(QSL("image-placeholder"))),
    m_placeholderImageError(qApp->icons()->miscPixmap(QSL("image-placeholder-error"))),
    m_downloader(new Downloader(this)), m_document(new TextBrowserDocument(this)) {
  setAutoFillBackground(false);
  setFrameShape(QFrame::Shape::NoFrame);
  setFrameShadow(QFrame::Shadow::Plain);
  setTabChangesFocus(true);
  setOpenLinks(false);
  setWordWrapMode(QTextOption::WrapMode::WordWrap);

  viewport()->setAutoFillBackground(false);

  setResourcesEnabled(qApp->settings()->value(GROUP(Messages), SETTING(Messages::ShowResourcesInArticles)).toBool());
  setDocument(m_document.data());

  connect(this, &TextBrowserViewer::reloadDocument, this, [this]() {
    const auto scr = verticalScrollBarPosition();
    setHtmlPrivate(html(), m_currentUrl);
    setVerticalScrollBarPosition(scr);
  });

  connect(m_resourceDownloader.data(), &Downloader::completed, this, &TextBrowserViewer::resourceDownloaded);
  connect(this, &QTextBrowser::anchorClicked, this, &TextBrowserViewer::onAnchorClicked);
  connect(this, QOverload<const QUrl&>::of(&QTextBrowser::highlighted), this, &TextBrowserViewer::linkMouseHighlighted);
}

QSize TextBrowserViewer::sizeHint() const {
  auto doc_size = document()->size().toSize();

  doc_size.setHeight(doc_size.height() + contentsMargins().top() + contentsMargins().bottom());
  return doc_size;
}

QVariant TextBrowserViewer::loadOneResource(int type, const QUrl& name) {
  if (type != QTextDocument::ResourceType::ImageResource) {
    return {};
  }

  auto resolved_name = (m_currentUrl.isValid() && name.isRelative()) ? m_currentUrl.resolved(name) : name;

  if (!m_resourcesEnabled || !m_loadedResources.contains(resolved_name)) {
    // Resources are not enabled.
    return m_placeholderImage;
  }

  // Resources are enabled and we already have the resource.
  QByteArray resource_data = m_loadedResources.value(resolved_name);
  QImage img;

  if (resource_data.isEmpty()) {
    img = m_placeholderImageError.toImage();
  }
  else {
    img = QImage::fromData(m_loadedResources.value(resolved_name));
  }

  int acceptable_width = int(width() * ACCEPTABLE_IMAGE_PERCENTUAL_WIDTH);

  if (img.width() > acceptable_width) {
    QElapsedTimer tmr;

    tmr.start();
    img = img.scaledToWidth(acceptable_width, Qt::TransformationMode::SmoothTransformation);

    qWarningNN << LOGSEC_GUI << "Picture" << QUOTE_W_SPACE(name)
               << "is too wide, down-scaling to prevent horizontal scrollbars. Scaling took"
               << NONQUOTE_W_SPACE(tmr.elapsed()) << "miliseconds.";
  }

  return img;
}

void TextBrowserViewer::bindToBrowser(WebBrowser* browser) {
  installEventFilter(browser);

  browser->m_actionBack = nullptr;
  browser->m_actionForward = nullptr;
  browser->m_actionReload = nullptr;
  browser->m_actionStop = nullptr;
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

BlockingResult TextBrowserViewer::blockedWithAdblock(const QUrl& url) {
  AdblockRequestInfo block_request(url);

  if (url.path().endsWith(QSL("css"))) {
    block_request.setResourceType(QSL("stylesheet"));
  }
  else {
    block_request.setResourceType(QSL("image"));
  }

  auto block_result = qApp->web()->adBlock()->block(block_request);

  if (block_result.m_blocked) {
    qWarningNN << LOGSEC_ADBLOCK << "Blocked request:" << QUOTE_W_SPACE_DOT(block_request.requestUrl().toString());
    return block_result;
  }
  else {
    return block_result;
  }
}

void TextBrowserViewer::setUrl(const QUrl& url) {
  emit loadingStarted();

  QString html_str;
  QUrl nonconst_url = url;
  bool is_error = false;
  auto block_result = blockedWithAdblock(url);

  if (block_result.m_blocked) {
    is_error = true;
    nonconst_url = QUrl::fromUserInput(QSL(INTERNAL_URL_ADBLOCKED));

    html_str = QSL("Blocked!!!<br/>%1").arg(url.toString());
  }
  else {
    QEventLoop loop;

    connect(m_downloader.data(), &Downloader::completed, &loop, &QEventLoop::quit);
    m_downloader->manipulateData(url.toString(), QNetworkAccessManager::Operation::GetOperation, {}, 5000);

    loop.exec();

    const auto net_error = m_downloader->lastOutputError();
    const QString content_type = m_downloader->lastContentType().toString();

    if (net_error != QNetworkReply::NetworkError::NoError) {
      is_error = true;
      html_str = QSL("Error!<br/>%1").arg(NetworkFactory::networkErrorText(net_error));
    }
    else {
      if (content_type.startsWith(QSL("image/"))) {
        html_str = QSL("<img src=\"%1\">").arg(nonconst_url.toString());
      }
      else {
        html_str = QString::fromUtf8(m_downloader->lastOutputData());
      }
    }
  }

  setHtml(html_str, nonconst_url);

  emit loadingFinished(!is_error);
}

QString TextBrowserViewer::html() const {
  return m_currentHtml;
}

QUrl TextBrowserViewer::url() const {
  return m_currentUrl;
}

void TextBrowserViewer::clear() {
  setHtml({});
}

void TextBrowserViewer::loadMessages(const QList<Message>& messages, RootItem* root) {
  emit loadingStarted();
  m_root = root;

  auto html_messages =
    qApp->skins()->generateHtmlOfArticles(messages, root, width() * ACCEPTABLE_IMAGE_PERCENTUAL_WIDTH);

  // Remove other characters which cannot be displayed properly.
  static QRegularExpression exp_symbols("&#x1F[0-9A-F]{3};");

  html_messages.m_html = html_messages.m_html.replace(exp_symbols, QString());

#if !defined(NDEBUG)
  // IOFactory::writeFile("aaa.html", html_messages.m_html.toUtf8());
#endif

  setHtml(html_messages.m_html, html_messages.m_baseUrl);

  QTextOption op;
  op.setTextDirection(messages.at(0).m_isRtl ? Qt::LayoutDirection::RightToLeft : Qt::LayoutDirection::LeftToRight);
  document()->setDefaultTextOption(op);

  emit loadingFinished(true);
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

  auto* menu = createStandardContextMenu(event->pos());

  if (menu == nullptr) {
    return;
  }

  /*
  connect(menu, &QMenu::aboutToHide, this, [menu] {
    menu->deleteLater();
  });*/

  if (m_actionEnableResources.isNull()) {
    m_actionEnableResources.reset(new QAction(qApp->icons()->fromTheme(QSL("viewimage"), QSL("image-x-generic")),
                                              tr("Enable external resources"),
                                              this));
    m_actionDownloadLink.reset(new QAction(qApp->icons()->fromTheme(QSL("download")), tr("Download"), this));

    m_actionEnableResources.data()->setCheckable(true);
    m_actionEnableResources.data()->setChecked(resourcesEnabled());

    connect(m_actionDownloadLink.data(), &QAction::triggered, this, &TextBrowserViewer::downloadLink);
    connect(m_actionEnableResources.data(), &QAction::toggled, this, &TextBrowserViewer::enableResources);
  }

  menu->addAction(m_actionEnableResources.data());
  menu->addAction(m_actionDownloadLink.data());

  auto anchor = anchorAt(event->pos());

  m_lastContextMenuPos = event->pos();
  m_actionDownloadLink.data()->setEnabled(!anchor.isEmpty());

  processContextMenu(menu, event);

  menu->popup(event->globalPos());
}

void TextBrowserViewer::resizeEvent(QResizeEvent* event) {
  // Notify parents about changed geometry.
  updateGeometry();
  QTextBrowser::resizeEvent(event);
}

void TextBrowserViewer::wheelEvent(QWheelEvent* event) {
  // NOTE: Skip base class implemetation.
  QAbstractScrollArea::wheelEvent(event);
  updateMicroFocus();
}

void TextBrowserViewer::enableResources(bool enable) {
  qApp->settings()->setValue(GROUP(Messages), Messages::ShowResourcesInArticles, enable);
  setResourcesEnabled(enable);
}

void TextBrowserViewer::downloadLink() {
  auto url = QUrl(anchorAt(m_lastContextMenuPos));

  if (url.isValid()) {
    const QUrl resolved_url = (m_currentUrl.isValid() && url.isRelative()) ? m_currentUrl.resolved(url) : url;

    qApp->downloadManager()->download(resolved_url);
  }
}

void TextBrowserViewer::onAnchorClicked(const QUrl& url) {
  if (!url.isEmpty()) {
    const QUrl resolved_url = (m_currentUrl.isValid() && url.isRelative()) ? m_currentUrl.resolved(url) : url;
    const bool ctrl_pressed =
      Globals::hasFlag(QGuiApplication::keyboardModifiers(), Qt::KeyboardModifier::ControlModifier);

    if (ctrl_pressed) {
      // Open in new tab.
      qApp->mainForm()->tabWidget()->addLinkedBrowser(resolved_url);
    }
    else {
      bool open_externally_now =
        qApp->settings()->value(GROUP(Browser), SETTING(Browser::OpenLinksInExternalBrowserRightAway)).toBool();

      if (open_externally_now) {
        qApp->web()->openUrlInExternalBrowser(resolved_url.toString());

        if (qApp->settings()
              ->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally))
              .toBool()) {
          QTimer::singleShot(1000, qApp, []() {
            qApp->mainForm()->display();
          });
        }
      }
      else {
        setUrl(resolved_url);
      }
    }
  }
}

void TextBrowserViewer::setHtml(const QString& html, const QUrl& base_url) {
  if (m_resourcesEnabled) {
    static QRegularExpression img_tag_rgx("\\<img[^\\>]*src\\s*=\\s*[\"\']([^\"\']*)[\"\'][^\\>]*\\>",
                                          QRegularExpression::PatternOption::CaseInsensitiveOption |
                                            QRegularExpression::PatternOption::InvertedGreedinessOption);
    QRegularExpressionMatchIterator i = img_tag_rgx.globalMatch(html);
    QList<QUrl> found_resources;

    while (i.hasNext()) {
      QRegularExpressionMatch match = i.next();
      auto captured_url = QUrl(match.captured(1));
      auto resolved_captured_url =
        (base_url.isValid() && captured_url.isRelative()) ? base_url.resolved(captured_url) : captured_url;

      if (!found_resources.contains(resolved_captured_url)) {
        found_resources.append(resolved_captured_url);
      }
    }

    auto really_needed_resources = boolinq::from(found_resources)
                                     .where([this](const QUrl& res) {
                                       return !m_loadedResources.contains(res);
                                     })
                                     .toStdList();

    m_neededResources = FROM_STD_LIST(QList<QUrl>, really_needed_resources);
  }
  else {
    m_neededResources = {};
  }

  setHtmlPrivate(html, base_url);

  /*
  QTextCursor cr(m_document.data());

  cr.movePosition(QTextCursor::MoveOperation::Start);

  // this can be used instead of regexps, just browse document and collect resource addresses directly
  while (!cr.atEnd()) {
    if (!cr.movePosition(QTextCursor::MoveOperation::NextBlock)) {
      break;
    }

    QTextBlock::iterator it;
    for (it = cr.block().begin(); !(it.atEnd()); ++it) {
      QTextFragment currentFragment = it.fragment();
      if (currentFragment.isValid()) {
        auto aa = currentFragment.charFormat().anchorHref();

        if (!aa.isEmpty()) {
          auto xx = 5;
        }
        else if (currentFragment.charFormat().isImageFormat()) {
          aa = currentFragment.charFormat().toImageFormat().name();
        }
      }
    }
  }
  */

  if (!m_neededResources.isEmpty()) {
    QTimer::singleShot(20, this, &TextBrowserViewer::reloadHtmlDelayed);
  }

  setVerticalScrollBarPosition(0.0);
}

void TextBrowserViewer::setReadabledHtml(const QString& html, const QUrl& base_url) {
  setHtml(html, base_url);
}

void TextBrowserViewer::setHtmlPrivate(const QString& html, const QUrl& base_url) {
  m_currentUrl = base_url;
  m_currentHtml = html;

  QTextBrowser::setHtml(html);
  setZoomFactor(m_zoomFactor);

  emit pageTitleChanged(documentTitle());
  emit pageUrlChanged(base_url);
}

TextBrowserDocument::TextBrowserDocument(TextBrowserViewer* parent) : QTextDocument(parent) {
  m_viewer = parent;
}

QVariant TextBrowserDocument::loadResource(int type, const QUrl& name) {
  return m_viewer->loadOneResource(type, name);
}

void TextBrowserViewer::reloadHtmlDelayed() {
  if (!m_neededResources.isEmpty()) {
    downloadNextNeededResource();
  }
}

void TextBrowserViewer::downloadNextNeededResource() {
  if (m_neededResources.isEmpty()) {
    // Everything is downloaded.
    emit reloadDocument();
  }
  else {
    QUrl res = m_neededResources.takeFirst();

    m_resourceDownloader.data()->manipulateData(qApp->web()->unescapeHtml(res.toString()),
                                                QNetworkAccessManager::Operation::GetOperation,
                                                {},
                                                5000);
  }
}

void TextBrowserViewer::resourceDownloaded(const QUrl& url,
                                           QNetworkReply::NetworkError status,
                                           int http_code,
                                           QByteArray contents) {
  Q_UNUSED(http_code)

  if (status == QNetworkReply::NetworkError::NoError) {
    m_loadedResources.insert(url, contents);
  }
  else {
    m_loadedResources.insert(url, {});
  }

  downloadNextNeededResource();
}

bool TextBrowserViewer::resourcesEnabled() const {
  return m_resourcesEnabled;
}

void TextBrowserViewer::setResourcesEnabled(bool enabled) {
  m_resourcesEnabled = enabled;
}

ContextMenuData TextBrowserViewer::provideContextMenuData(QContextMenuEvent* event) const {
  ContextMenuData c;

  QString anchor = anchorAt(event->pos());

  if (!anchor.isEmpty()) {
    c.m_linkUrl = anchor;
  }

  return c;
}
