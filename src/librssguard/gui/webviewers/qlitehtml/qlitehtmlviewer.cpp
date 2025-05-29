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

#include <QClipboard>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QScrollBar>

QLiteHtmlViewer::QLiteHtmlViewer(QWidget* parent)
  : QLiteHtmlWidget(parent), m_root(nullptr), m_network(new SilentNetworkAccessManager(this)) {
  setAutoFillBackground(false);
  viewport()->setAutoFillBackground(false);
  setFrameShape(QFrame::Shape::NoFrame);
  setFrameShadow(QFrame::Shadow::Plain);

  horizontalScrollBar()->setSingleStep(5);
  verticalScrollBar()->setSingleStep(5);

  connect(this, &QLiteHtmlWidget::linkHighlighted, this, &QLiteHtmlViewer::linkMouseHighlighted);

  setResourceHandler([this](const QUrl& url) {
    qDebugNN << LOGSEC_HTMLVIEWER << "Resource requested:" << QUOTE_W_SPACE_DOT(url);

    QEventLoop loop;
    QByteArray data;
    QNetworkReply* reply = m_network->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [&data, &loop, reply] {
      qDebugNN << LOGSEC_HTMLVIEWER << "Resource" << QUOTE_W_SPACE(reply->url())
               << "finished:" << QUOTE_W_SPACE_DOT(reply->error());

      if (reply->error() == QNetworkReply::NetworkError::NoError) {
        data = reply->readAll();
      }

      reply->deleteLater();
      loop.exit();
    });

    loop.exec(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents);
    return data;
  });
}

QLiteHtmlViewer::~QLiteHtmlViewer() {}

void QLiteHtmlViewer::bindToBrowser(WebBrowser* browser) {
  installEventFilter(browser);
}

void QLiteHtmlViewer::findText(const QString& text, bool backwards) {
  QLiteHtmlWidget::findText(text,
                            backwards ? QTextDocument::FindFlag::FindBackward : QTextDocument::FindFlag(0),
                            false);
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
  m_root = root;

  auto url = urlForMessage(message, root);
  auto html = htmlForMessage(message, root);

  setHtml(html, url);

  // TODO: RTL

  emit loadingFinished(true);
}

QString QLiteHtmlViewer::prepareLegacyHtmlForMessage(const Message& message, RootItem* selected_item) const {
  QString html;
  bool acc_displays_enclosures =
    selected_item == nullptr || selected_item->getParentServiceRoot()->displaysEnclosures();
  bool is_plain = !TextFactory::couldBeHtml(message.m_contents);

  // Add title.
  if (!message.m_url.isEmpty()) {
    html += QSL("<h2 align=\"center\"><a href=\"%2\">%1</a></h2>").arg(message.m_title, message.m_url);
  }
  else {
    html += QSL("<h2 align=\"center\">%1</h2>").arg(message.m_title);
  }

  // Start contents.
  html += QSL("<div>");

  // Add links to enclosures.
  if (acc_displays_enclosures) {
    for (const Enclosure& enc : message.m_enclosures) {
      html += QSL("[<a href=\"%1\">%2</a>]").arg(enc.m_url, enc.m_mimeType);
    }
  }

  // Display enclosures which are pictures if user has it enabled.
  auto first_enc_break_added = false;

  if (acc_displays_enclosures &&
      qApp->settings()->value(GROUP(Messages), SETTING(Messages::DisplayEnclosuresInMessage)).toBool()) {
    for (const Enclosure& enc : message.m_enclosures) {
      if (enc.m_mimeType.startsWith(QSL("image/"))) {
        if (!first_enc_break_added) {
          html += QSL("<br/>");
          first_enc_break_added = true;
        }

        html += QSL("<img src=\"%1\" /><br/>").arg(enc.m_url);
      }
    }
  }

  // Append actual contents of article and convert to HTML if needed.
  html +=
    is_plain ? Qt::convertFromPlainText(message.m_contents, Qt::WhiteSpaceMode::WhiteSpaceNormal) : message.m_contents;

  static QRegularExpression img_tag_rgx(QSL("\\<img[^\\>]*src\\s*=\\s*[\"\']([^\"\']*)[\"\'][^\\>]*\\>"),
                                        QRegularExpression::PatternOption::CaseInsensitiveOption);

  // Extract all images links from article to be appended to end of article.
  QRegularExpressionMatchIterator i = img_tag_rgx.globalMatch(html);
  QString pictures_html;

  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    auto captured_url = match.captured(1);

    pictures_html += QSL("<br/>[%1] <a href=\"%2\">%2</a>").arg(tr("image"), captured_url);
  }

  // Make alla images clickable as links and also resize them if user has it setup.
  auto forced_img_size = qApp->settings()->value(GROUP(Messages), SETTING(Messages::LimitArticleImagesHeight)).toInt();

  // Fixup all "img" tags.
  html = html.replace(img_tag_rgx,
                      QSL("<a href=\"\\1\"><img height=\"%1\" src=\"\\1\" /></a>")
                        .arg(forced_img_size <= 0 ? QString() : QString::number(forced_img_size)));

  // Append generated list of images.
  html += pictures_html;

  // Close contents.
  html += QSL("</div>");

  return html;
}

QString QLiteHtmlViewer::htmlForMessage(const Message& message, RootItem* root) const {
  auto html_message =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::UseLegacyArticleFormat)).toBool()
      ? prepareLegacyHtmlForMessage(message, root)
      : qApp->skins()->generateHtmlOfArticle(message, root, width() * ACCEPTABLE_IMAGE_PERCENTUAL_WIDTH);

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
  setDefaultFont(fon);
  setZoomFactor(zoomFactor());
}

qreal QLiteHtmlViewer::zoomFactor() const {
  return QLiteHtmlWidget::zoomFactor();
}

void QLiteHtmlViewer::setZoomFactor(qreal zoom_factor) {
  QLiteHtmlWidget::setZoomFactor(zoom_factor);
}

void QLiteHtmlViewer::setHtml(const QString& html, const QUrl& url) {
  IOFactory::debugWriteFile("a.html", html.toUtf8());

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

  QString anchor = d->documentContainer.linkAt(pos, viewportPos).toString();

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
