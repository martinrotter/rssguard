// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagebrowser.h"

#include "gui/messagebox.h"
#include "gui/messagetextbrowser.h"
#include "gui/searchtextwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "network-web/webfactory.h"
#include "services/abstract/serviceroot.h"

#include <QKeyEvent>
#include <QRegularExpression>
#include <QScrollBar>
#include <QToolTip>
#include <QVBoxLayout>

MessageBrowser::MessageBrowser(bool should_resize_to_fit, QWidget* parent)
  : QWidget(parent), m_txtBrowser(new MessageTextBrowser(this)), m_searchWidget(new SearchTextWidget(this)),
  m_layout(new QVBoxLayout(this)) {
  m_layout->setContentsMargins(3, 3, 3, 3);
  m_layout->addWidget(m_txtBrowser, 1);
  m_layout->addWidget(m_searchWidget);

  if (should_resize_to_fit) {
    m_txtBrowser->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::MinimumExpanding);
  }

  connect(m_searchWidget, &SearchTextWidget::searchCancelled, this, [this]() {
    m_txtBrowser->textCursor().clearSelection();
    m_txtBrowser->moveCursor(QTextCursor::MoveOperation::Left);
  });
  connect(m_searchWidget, &SearchTextWidget::searchForText, this, [this](const QString& text, bool backwards) {
    if (backwards) {
      m_txtBrowser->find(text, QTextDocument::FindFlag::FindBackward);
    }
    else {
      m_txtBrowser->find(text);
    }
  });

  connect(m_txtBrowser, &QTextBrowser::anchorClicked, [=](const QUrl& url) {
    if (url.toString().startsWith(INTERNAL_URL_PASSATTACHMENT) &&
        m_root != nullptr &&
        m_root->getParentServiceRoot()->downloadAttachmentOnMyOwn(url)) {
      return;
    }

    if (!url.isEmpty()) {
      bool open_externally_now = qApp->settings()->value(GROUP(Browser),
                                                         SETTING(Browser::OpenLinksInExternalBrowserRightAway)).toBool();

      if (open_externally_now) {
        qApp->web()->openUrlInExternalBrowser(url.toString());
      }
      else {
        // User clicked some URL. Open it in external browser or download?
        MessageBox box(qApp->mainFormWidget());
        box.setText(tr("You clicked some link. You can download the link contents or open it in external web browser."));
        box.setInformativeText(tr("What action do you want to take?"));
        box.setDetailedText(url.toString());

        QAbstractButton* btn_open = box.addButton(tr("Open in external browser"), QMessageBox::ActionRole);
        QAbstractButton* btn_download = box.addButton(tr("Download"), QMessageBox::ActionRole);
        QAbstractButton* btn_cancel = box.addButton(QMessageBox::Cancel);
        bool always;
        MessageBox::setCheckBox(&box, tr("Always open links in external browser."), &always);

        box.setDefaultButton(QMessageBox::Cancel);
        box.exec();

        if (box.clickedButton() != box.button(QMessageBox::Cancel)) {
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
      MessageBox::show(qApp->mainFormWidget(), QMessageBox::Warning, tr("Incorrect link"), tr("Selected hyperlink is invalid."));
    }
  });
  connect(m_txtBrowser,
          QOverload<const QUrl&>::of(&QTextBrowser::highlighted),
          [=](const QUrl& url) {
    Q_UNUSED(url)
    QToolTip::showText(QCursor::pos(), tr("Click this link to download it or open it with external browser."), this);
  });

  m_searchWidget->hide();
  installEventFilter(this);
}

void MessageBrowser::clear() {
  m_txtBrowser->clear();
  m_pictures.clear();
  m_searchWidget->hide();
}

QString MessageBrowser::prepareHtmlForMessage(const Message& message) {
  QString html = QString("<h2 align=\"center\">%1</h2>").arg(message.m_title);

  if (!message.m_url.isEmpty()) {
    html += QString("[url] <a href=\"%1\">%1</a><br/>").arg(message.m_url);
  }

  for (const Enclosure& enc : message.m_enclosures) {
    QString enc_url;

    if (!enc.m_url.contains(QRegularExpression(QSL("^(http|ftp|\\/)")))) {
      enc_url = QString(INTERNAL_URL_PASSATTACHMENT) + QL1S("/?") + enc.m_url;
    }
    else {
      enc_url = enc.m_url;
    }

    html += QString("[%2] <a href=\"%1\">%1</a><br/>").arg(enc_url, enc.m_mimeType);
  }

  QRegularExpression imgTagRegex("\\<img[^\\>]*src\\s*=\\s*[\"\']([^\"\']*)[\"\'][^\\>]*\\>",
                                 QRegularExpression::PatternOption::CaseInsensitiveOption |
                                 QRegularExpression::PatternOption::InvertedGreedinessOption);
  QRegularExpressionMatchIterator i = imgTagRegex.globalMatch(message.m_contents);
  QString pictures_html;

  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();

    m_pictures.append(match.captured(1));
    pictures_html += QString("<br/>[%1] <a href=\"%2\">%2</a>").arg(tr("image"), match.captured(1));
  }

  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::DisplayImagePlaceholders)).toBool()) {
    html += message.m_contents;
  }
  else {
    QString cnts = message.m_contents;

    html += cnts.replace(imgTagRegex, QString());
  }

  html += pictures_html;
  html = html
         .replace(QSL("\r\n"), QSL("\n"))
         .replace(QL1C('\r'), QL1C('\n'))
         .remove(QL1C('\n'));

  return html;
}

bool MessageBrowser::eventFilter(QObject* watched, QEvent* event) {
  Q_UNUSED(watched)

  if (event->type() == QEvent::Type::KeyPress) {
    auto* key_event = static_cast<QKeyEvent*>(event);

    if (key_event->matches(QKeySequence::StandardKey::Find)) {

      m_searchWidget->clear();
      m_searchWidget->show();
      m_searchWidget->setFocus();
      return true;
    }
    else if (key_event->key() == Qt::Key::Key_Escape) {
      m_searchWidget->cancelSearch();
      return true;
    }
  }

  return false;
}

void MessageBrowser::reloadFontSettings() {
  const Settings* settings = qApp->settings();
  QFont fon;

  fon.fromString(settings->value(GROUP(Messages), SETTING(Messages::PreviewerFontStandard)).toString());
  m_txtBrowser->setFont(fon);
}

void MessageBrowser::loadMessage(const Message& message, RootItem* root) {
  Q_UNUSED(root)

  m_txtBrowser->setHtml(prepareHtmlForMessage(message));
  m_txtBrowser->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
  m_searchWidget->hide();
}

double MessageBrowser::verticalScrollBarPosition() const {
  return m_txtBrowser->verticalScrollBar()->value();
}

void MessageBrowser::setVerticalScrollBarPosition(double pos) {
  m_txtBrowser->verticalScrollBar()->setValue(pos);
}
