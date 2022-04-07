// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/litehtml/litehtmlviewer.h"

#include "core/message.h"
#include "gui/dialogs/formmain.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/skinfactory.h"
#include "network-web/adblock/adblockicon.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/downloader.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"

#include <QAction>
#include <QClipboard>
#include <QFileIconProvider>
#include <QMenu>
#include <QScrollBar>
#include <QTimer>
#include <QWheelEvent>

LiteHtmlViewer::LiteHtmlViewer(QWidget* parent) : QLiteHtmlWidget(parent), m_downloader(new Downloader(this)),
  m_reloadingWithImages(false) {
  setResourceHandler([this](const QUrl& url) {
    emit loadProgress(-1);
    return m_reloadingWithImages ? handleResource(url) : QByteArray{};
  });

  setFrameShape(QFrame::Shape::NoFrame);

  connect(this, &LiteHtmlViewer::linkClicked, this, &LiteHtmlViewer::onLinkClicked);
  connect(this, &LiteHtmlViewer::copyAvailable, this, &LiteHtmlViewer::selectedTextChanged);
  connect(this, &LiteHtmlViewer::contextMenuRequested, this, &LiteHtmlViewer::showContextMenu);
}

void LiteHtmlViewer::bindToBrowser(WebBrowser* browser) {
  installEventFilter(browser);

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

  // TODO: add "Open in new tab" to context menu.
}

void LiteHtmlViewer::findText(const QString& text, bool backwards) {
  QLiteHtmlWidget::findText(text, backwards
                            ? QTextDocument::FindFlag::FindBackward
                            : QTextDocument::FindFlag(0x0), false);
}

void LiteHtmlViewer::setUrl(const QUrl& url) {
  emit loadStarted();
  QString html_str;
  QUrl nonconst_url = url;
  bool is_error = false;
  auto block_result = blockedWithAdblock(url);

  if (block_result.m_blocked) {
    is_error = true;
    nonconst_url = QUrl::fromUserInput(QSL(INTERNAL_URL_ADBLOCKED));
    html_str = qApp->skins()->adBlockedPage(url.toString(), block_result.m_blockedByFilter);
  }
  else {
    QEventLoop loop;

    connect(m_downloader.data(), &Downloader::completed, &loop, &QEventLoop::quit);
    m_downloader->manipulateData(url.toString(),
                                 QNetworkAccessManager::Operation::GetOperation,
                                 {},
                                 5000);

    loop.exec();

    const auto net_error = m_downloader->lastOutputError();
    const QString content_type = m_downloader->lastContentType().toString();

    if (net_error != QNetworkReply::NetworkError::NoError) {
      is_error = true;
      html_str = "Error!";
    }
    else {
      if (content_type.startsWith(QSL("image/"))) {
        html_str =
          "<img src=\"data:image/gif;base64,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\">";
      }
      else {
        html_str = QString::fromUtf8(m_downloader->lastOutputData());
      }
    }
  }

  setHtml(html_str, nonconst_url);

  emit loadFinished(is_error);
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
  auto html_messages = qApp->skins()->generateHtmlOfArticles(messages, root);

  setHtml(html_messages.first, html_messages.second);
  emit loadFinished(true);
}

double LiteHtmlViewer::verticalScrollBarPosition() const {
  return verticalScrollBar()->value();
}

void LiteHtmlViewer::setVerticalScrollBarPosition(double pos) {
  verticalScrollBar()->setValue(pos);
}

void LiteHtmlViewer::applyFont(const QFont& fon) {
  QLiteHtmlWidget::setDefaultFont(fon);
}

qreal LiteHtmlViewer::zoomFactor() const {
  return QLiteHtmlWidget::zoomFactor();
}

void LiteHtmlViewer::setZoomFactor(qreal zoom_factor) {
  if (zoom_factor == 0.0) {
    QLiteHtmlWidget::setZoomFactor(MIN_ZOOM_FACTOR);
  }
  else {
    QLiteHtmlWidget::setZoomFactor(zoom_factor);
  }
}

void LiteHtmlViewer::selectedTextChanged(bool available) {
  if (!available) {
    return;
  }

  QString sel_text = QLiteHtmlWidget::selectedText();

  if (!sel_text.isEmpty()) {
    QGuiApplication::clipboard()->setText(sel_text, QClipboard::Mode::Selection);
  }
}

void LiteHtmlViewer::onLinkClicked(const QUrl& link) {
  if ((QApplication::queryKeyboardModifiers() & Qt::KeyboardModifier::ControlModifier) > 0) {
    LiteHtmlViewer* viewer = new LiteHtmlViewer(this);
    emit newWindowRequested(viewer);

    viewer->setUrl(link);
  }
  else {
    setUrl(link);
  }
}

void LiteHtmlViewer::reloadPageWithImages() {
  m_reloadingWithImages = true;

  auto scroll = verticalScrollBar()->value();

  setHtml(html(), url());

  if (scroll > 0) {
    verticalScrollBar()->setValue(scroll);
  }

  m_reloadingWithImages = false;
}

void LiteHtmlViewer::showContextMenu(const QPoint& pos, const QUrl& url) {
  if (m_contextMenu.isNull()) {
    m_contextMenu.reset(new QMenu("Context menu for web browser", this));

    m_actionCopyUrl.reset(new QAction(qApp->icons()->fromTheme(QSL("edit-copy")),
                                      tr("Copy URL"),
                                      this));

    connect(m_actionCopyUrl.data(), &QAction::triggered, this, [url]() {
      QGuiApplication::clipboard()->setText(url.toString(), QClipboard::Mode::Clipboard);
    });

    m_actionCopyText.reset(new QAction(qApp->icons()->fromTheme(QSL("edit-copy")),
                                       tr("Copy selection"),
                                       this));

    connect(m_actionCopyText.data(), &QAction::triggered, this, [this]() {
      QGuiApplication::clipboard()->setText(QLiteHtmlWidget::selectedText(), QClipboard::Mode::Clipboard);
    });

    // Add option to open link in external viewe
    m_actionOpenLinkExternally.reset(new QAction(qApp->icons()->fromTheme(QSL("document-open")),
                                                 tr("Open link in external browser"),
                                                 this));

    connect(m_actionOpenLinkExternally.data(), &QAction::triggered, this, [url]() {
      qApp->web()->openUrlInExternalBrowser(url.toString());

      if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally)).toBool()) {
        QTimer::singleShot(1000, qApp, []() {
          qApp->mainForm()->display();
        });
      }
    });

    m_actionReloadWithImages.reset(new QAction(qApp->icons()->fromTheme(QSL("viewimage"), QSL("view-refresh")),
                                               tr("Reload with images"),
                                               this));

    connect(m_actionReloadWithImages.data(), &QAction::triggered, this, &LiteHtmlViewer::reloadPageWithImages);
  }

  m_actionCopyUrl->setEnabled(url.isValid());
  m_actionCopyText->setEnabled(!QLiteHtmlWidget::selectedText().isEmpty());
  m_actionOpenLinkExternally->setEnabled(url.isValid());

  m_contextMenu->clear();
  m_contextMenu->addActions({ m_actionCopyUrl.data(),
                              m_actionCopyText.data(),
                              m_actionOpenLinkExternally.data(),
                              m_actionReloadWithImages.data() });

  if (url.isValid()) {
    QFileIconProvider icon_provider;
    QMenu* menu_ext_tools = new QMenu(tr("Open with external tool"), this);
    auto tools = ExternalTool::toolsFromSettings();

    menu_ext_tools->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

    for (const ExternalTool& tool : qAsConst(tools)) {
      QAction* act_tool = new QAction(QFileInfo(tool.executable()).fileName(), menu_ext_tools);

      act_tool->setIcon(icon_provider.icon(QFileInfo(tool.executable())));
      act_tool->setToolTip(tool.executable());
      act_tool->setData(QVariant::fromValue(tool));
      menu_ext_tools->addAction(act_tool);

      connect(act_tool, &QAction::triggered, this, [act_tool, url]() {
        act_tool->data().value<ExternalTool>().run(url.toString());
      });
    }

    if (menu_ext_tools->actions().isEmpty()) {
      QAction* act_not_tools = new QAction(tr("No external tools activated"));

      act_not_tools->setEnabled(false);
      menu_ext_tools->addAction(act_not_tools);
    }

    m_contextMenu->addMenu(menu_ext_tools);
  }

  m_contextMenu->addAction(qApp->web()->adBlock()->adBlockIcon());
  m_contextMenu->popup(mapToGlobal(pos));
}

BlockingResult LiteHtmlViewer::blockedWithAdblock(const QUrl& url) {
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

QByteArray LiteHtmlViewer::handleResource(const QUrl& url) {
  if (blockedWithAdblock(url).m_blocked) {
    return {};
  }
  else {
    QEventLoop loop;

    connect(m_downloader.data(), &Downloader::completed, &loop, &QEventLoop::quit);
    m_downloader->manipulateData(url.toString(),
                                 QNetworkAccessManager::Operation::GetOperation,
                                 {},
                                 5000);

    loop.exec();
    return m_downloader->lastOutputData();
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
