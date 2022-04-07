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
  m_reloadingWithImages(false), m_useSimpleArticleLayout(false) {
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
        html_str = QSL("<img src=\"data:%1;base64,%2\">").arg(content_type,
                                                              QString::fromLocal8Bit(m_downloader->lastOutputData().toBase64()));
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

QPair<QString, QUrl> LiteHtmlViewer::prepareHtmlForMessage(const QList<Message>& messages, RootItem* selected_item) const {
  QString html;

  for (const Message& message : messages) {
    html += QString("<h2 align=\"center\">%1</h2>").arg(message.m_title);

    if (!message.m_url.isEmpty()) {
      html += QString("[url] <a href=\"%1\">%1</a><br/>").arg(message.m_url);
    }

    for (const Enclosure& enc : message.m_enclosures) {
      html += QString("[%2] <a href=\"%1\">%1</a><br/>").arg(enc.m_url, enc.m_mimeType);
    }

    QRegularExpression imgTagRegex("\\<img[^\\>]*src\\s*=\\s*[\"\']([^\"\']*)[\"\'][^\\>]*\\>",
                                   QRegularExpression::PatternOption::CaseInsensitiveOption |
                                   QRegularExpression::PatternOption::InvertedGreedinessOption);
    QRegularExpressionMatchIterator i = imgTagRegex.globalMatch(message.m_contents);
    QString pictures_html;

    while (i.hasNext()) {
      QRegularExpressionMatch match = i.next();

      pictures_html += QString("<br/>[%1] <a href=\"%2\">%2</a>").arg(tr("image"), match.captured(1));
    }

    /*if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::DisplayImagePlaceholders)).toBool()) {
      html += message.m_contents;
    }
    else {*/
      QString cnts = message.m_contents;

      html += cnts.replace(imgTagRegex, QString());
    //}

    html += pictures_html;
  }

  // TODO: If FgInteresting not defined by the skin
  // use current pallette/Highlight color perhaps.
  return { QSL("<html>"
               "<head><style>"
               "a { color: %2; }"
               "</style></head>"
               "<body>%1</body>"
               "</html>").arg(html,
                              qApp->skins()->currentSkin()
                              .colorForModel(SkinEnums::PaletteColors::FgInteresting)
                              .value<QColor>().name()),
           QUrl() };
}

void LiteHtmlViewer::loadMessages(const QList<Message>& messages, RootItem* root) {
  auto html_messages = m_useSimpleArticleLayout
                       ? prepareHtmlForMessage(messages, root)
                       : qApp->skins()->generateHtmlOfArticles(messages, root);

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

void LiteHtmlViewer::simpleLayoutChanged(bool activated) {
  m_useSimpleArticleLayout = activated;
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

    m_actionSimpleLayout.reset(new QAction(qApp->icons()->fromTheme(QSL("view-list-details")),
                                           tr("Use simple article layout"),
                                           this));
    m_actionSimpleLayout->setCheckable(true);

    m_actionReloadWithImages.reset(new QAction(qApp->icons()->fromTheme(QSL("viewimage"), QSL("view-refresh")),
                                               tr("Reload with images"),
                                               this));

    connect(m_actionSimpleLayout.data(), &QAction::triggered, this, &LiteHtmlViewer::simpleLayoutChanged);
    connect(m_actionReloadWithImages.data(), &QAction::triggered, this, &LiteHtmlViewer::reloadPageWithImages);
  }

  m_actionCopyUrl->setEnabled(url.isValid());
  m_actionCopyText->setEnabled(!QLiteHtmlWidget::selectedText().isEmpty());
  m_actionOpenLinkExternally->setEnabled(url.isValid());

  m_contextMenu->clear();
  m_contextMenu->addActions({ m_actionCopyUrl.data(),
                              m_actionCopyText.data(),
                              m_actionOpenLinkExternally.data(),
                              m_actionSimpleLayout.data(),
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
