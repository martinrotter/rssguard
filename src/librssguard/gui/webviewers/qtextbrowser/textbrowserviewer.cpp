// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"

#include "gui/messagebox.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/downloader.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"

#include <QContextMenuEvent>
#include <QFileIconProvider>
#include <QScrollBar>

TextBrowserViewer::TextBrowserViewer(QWidget* parent) : QTextBrowser(parent), m_downloader(new Downloader(this)) {
  setAutoFillBackground(true);
  setFrameShape(QFrame::Shape::NoFrame);
  setFrameShadow(QFrame::Shadow::Plain);
  setTabChangesFocus(true);
  setOpenLinks(false);
  viewport()->setAutoFillBackground(true);

  connect(this, &QTextBrowser::anchorClicked, this, &TextBrowserViewer::onAnchorClicked);
  connect(this, QOverload<const QUrl&>::of(&QTextBrowser::highlighted), this, &TextBrowserViewer::linkMouseHighlighted);
}

QVariant TextBrowserViewer::loadResource(int type, const QUrl& name) {
  return {};
}

QSize TextBrowserViewer::sizeHint() const {
  auto doc_size = document()->size().toSize();

  doc_size.setHeight(doc_size.height() + contentsMargins().top() + contentsMargins().bottom());
  return doc_size;
}

QPair<QString, QUrl> TextBrowserViewer::prepareHtmlForMessage(const QList<Message>& messages,
                                                              RootItem* selected_item) const {
  QString html;

  for (const Message& message : messages) {
    html += QString("<h2 align=\"center\">%1</h2>").arg(message.m_title);

    if (!message.m_url.isEmpty()) {
      html += QString("[url] <a href=\"%1\">%1</a><br/>").arg(message.m_url);
    }

    for (const Enclosure& enc : message.m_enclosures) {
      html += QString("[%2] <a href=\"%1\">%1</a><br/>").arg(enc.m_url, enc.m_mimeType);
    }

    static QRegularExpression img_tag_rgx("\\<img[^\\>]*src\\s*=\\s*[\"\']([^\"\']*)[\"\'][^\\>]*\\>",
                                          QRegularExpression::PatternOption::CaseInsensitiveOption |
                                            QRegularExpression::PatternOption::InvertedGreedinessOption);
    QRegularExpressionMatchIterator i = img_tag_rgx.globalMatch(message.m_contents);
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

    html += cnts;
    // html += cnts.replace(imgTagRegex, QString());

    //}

    html += pictures_html;
  }

  QColor a_color = qApp->skins()->currentSkin().colorForModel(SkinEnums::PaletteColors::FgInteresting).value<QColor>();

  if (!a_color.isValid()) {
    a_color = qApp->palette().color(QPalette::ColorRole::Highlight);
  }

  QString base_url;
  auto* feed = selected_item->getParentServiceRoot()
                 ->getItemFromSubTree([messages](const RootItem* it) {
                   return it->kind() == RootItem::Kind::Feed && it->customId() == messages.at(0).m_feedId;
                 })
                 ->toFeed();

  if (feed != nullptr) {
    QUrl url(NetworkFactory::sanitizeUrl(feed->source()));

    if (url.isValid()) {
      base_url = url.scheme() + QSL("://") + url.host();
    }
  }

  return {QSL("<html>"
              "<head><style>"
              "a { color: %2; }"
              "</style></head>"
              "<body>%1</body>"
              "</html>")
            .arg(html, a_color.name()),
          base_url};
}

void TextBrowserViewer::bindToBrowser(WebBrowser* browser) {
  installEventFilter(browser);

  browser->m_actionBack = new QAction(this);
  browser->m_actionForward = new QAction(this);
  browser->m_actionReload = new QAction(this);
  browser->m_actionStop = new QAction(this);

  browser->m_actionBack->setEnabled(false);
  browser->m_actionForward->setEnabled(false);
  browser->m_actionReload->setEnabled(false);
  browser->m_actionStop->setEnabled(false);
}

void TextBrowserViewer::findText(const QString& text, bool backwards) {
  if (!text.isEmpty()) {
    QTextBrowser::find(text, backwards ? QTextDocument::FindFlag::FindBackward : QTextDocument::FindFlag(0));
  }
  else {
    textCursor().clearSelection();
    moveCursor(QTextCursor::MoveOperation::Left);
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

    // TODO: Zjednodušeně.
    html_str = qApp->skins()->adBlockedPage(url.toString(), block_result.m_blockedByFilter);
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
      // TODO: lepší hlaška.
      html_str = "Error!";
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

void TextBrowserViewer::setHtml(const QString& html, const QUrl& base_url) {
  m_currentUrl = base_url;

  QTextBrowser::setHtml(html);

  setZoomFactor(m_zoomFactor);

  emit pageTitleChanged(documentTitle());
  emit pageUrlChanged(base_url);
}

QString TextBrowserViewer::html() const {
  return QTextBrowser::toHtml();
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

  auto html_messages = prepareHtmlForMessage(messages, root);

  setHtml(html_messages.first, html_messages.second);
  emit loadingFinished(true);
}

double TextBrowserViewer::verticalScrollBarPosition() const {
  return verticalScrollBar()->value();
}

void TextBrowserViewer::setVerticalScrollBarPosition(double pos) {
  verticalScrollBar()->setValue(int(pos));
}

void TextBrowserViewer::applyFont(const QFont& fon) {
  setFont(fon);
}

qreal TextBrowserViewer::zoomFactor() const {
  return font().pointSizeF() / 8.0;
}

void TextBrowserViewer::setZoomFactor(qreal zoom_factor) {
  m_zoomFactor = zoom_factor;

  auto fon = font();
  fon.setPointSizeF(8.0 * zoom_factor);

  applyFont(fon);
}

void TextBrowserViewer::contextMenuEvent(QContextMenuEvent* event) {
  event->accept();

  auto* menu = createStandardContextMenu(event->pos());

  if (menu == nullptr) {
    return;
  }

  auto anchor = anchorAt(event->pos());

  if (!anchor.isEmpty()) {
    QFileIconProvider icon_provider;
    QMenu* menu_ext_tools = new QMenu(tr("Open with external tool"), menu);
    auto tools = ExternalTool::toolsFromSettings();

    menu_ext_tools->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

    for (const ExternalTool& tool : qAsConst(tools)) {
      QAction* act_tool = new QAction(QFileInfo(tool.executable()).fileName(), menu_ext_tools);

      act_tool->setIcon(icon_provider.icon(QFileInfo(tool.executable())));
      act_tool->setToolTip(tool.executable());
      act_tool->setData(QVariant::fromValue(tool));
      menu_ext_tools->addAction(act_tool);

      connect(act_tool, &QAction::triggered, this, [act_tool, anchor]() {
        act_tool->data().value<ExternalTool>().run(anchor);
      });
    }

    if (menu_ext_tools->actions().isEmpty()) {
      QAction* act_not_tools = new QAction("No external tools activated");

      act_not_tools->setEnabled(false);
      menu_ext_tools->addAction(act_not_tools);
    }

    menu->addMenu(menu_ext_tools);
  }

  connect(menu, &QMenu::aboutToHide, this, [menu] {
    menu->deleteLater();
  });

  menu->popup(event->globalPos());
}

void TextBrowserViewer::resizeEvent(QResizeEvent* event) {
  // Notify parents about changed geometry.
  updateGeometry();
  QTextBrowser::resizeEvent(event);
}

void TextBrowserViewer::wheelEvent(QWheelEvent* event) {
  QAbstractScrollArea::wheelEvent(event);
  updateMicroFocus();
}

void TextBrowserViewer::onAnchorClicked(const QUrl& url) {
  if (!url.isEmpty()) {
    bool open_externally_now =
      qApp->settings()->value(GROUP(Browser), SETTING(Browser::OpenLinksInExternalBrowserRightAway)).toBool();

    if (open_externally_now) {
      qApp->web()->openUrlInExternalBrowser(url.toString());
    }
    else {
      // User clicked some URL. Open it in external browser or download?
      MsgBox box(qApp->mainFormWidget());

      box.setText(tr("You clicked some link. You can download the link contents or open it in external web browser."));
      box.setInformativeText(tr("What action do you want to take?"));
      box.setDetailedText(url.toString());

      QAbstractButton* btn_open = box.addButton(tr("Open in external browser"), QMessageBox::ButtonRole::ActionRole);
      QAbstractButton* btn_download = box.addButton(tr("Download"), QMessageBox::ButtonRole::ActionRole);
      QAbstractButton* btn_cancel = box.addButton(QMessageBox::StandardButton::Cancel);
      bool always = false;

      MsgBox::setCheckBox(&box, tr("Always open links in external browser."), &always);

      box.setDefaultButton(QMessageBox::StandardButton::Cancel);
      box.exec();

      if (box.clickedButton() != box.button(QMessageBox::StandardButton::Cancel)) {
        // Store selected checkbox value.
        qApp->settings()->setValue(GROUP(Browser), Browser::OpenLinksInExternalBrowserRightAway, always);
      }

      if (box.clickedButton() == btn_open) {
        qApp->web()->openUrlInExternalBrowser(url.toString());
      }
      else if (box.clickedButton() == btn_download) {
        qApp->downloadManager()->download(url);
      }

      btn_download->deleteLater();
      btn_open->deleteLater();
      btn_cancel->deleteLater();
    }
  }
  else {
    MsgBox::show(qApp->mainFormWidget(),
                 QMessageBox::Warning,
                 tr("Incorrect link"),
                 tr("Selected hyperlink is invalid."));
  }
}
