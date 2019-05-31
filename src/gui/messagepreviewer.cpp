// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagepreviewer.h"

#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "network-web/webfactory.h"
#include "services/abstract/serviceroot.h"

#include <QScrollBar>
#include <QToolBar>
#include <QToolTip>

void MessagePreviewer::createConnections() {
  installEventFilter(this);

  connect(m_ui.m_searchWidget, &SearchTextWidget::cancelSearch, this, [this]() {
    m_ui.m_txtMessage->textCursor().clearSelection();
    m_ui.m_txtMessage->moveCursor(QTextCursor::MoveOperation::Left);
  });
  connect(m_ui.m_searchWidget, &SearchTextWidget::searchForText, this, [this](const QString& text, bool backwards) {
    if (backwards) {
      m_ui.m_txtMessage->find(text, QTextDocument::FindFlag::FindBackward);
    }
    else {
      m_ui.m_txtMessage->find(text);
    }
  });

  connect(m_ui.m_txtMessage, &QTextBrowser::anchorClicked, [=](const QUrl& url) {
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
        MessageBox box(qApp->mainForm());
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
      MessageBox::show(qApp->mainForm(), QMessageBox::Warning, tr("Incorrect link"),
                       tr("Selected hyperlink is invalid."));
    }
  });
  connect(m_actionMarkRead = m_toolBar->addAction(qApp->icons()->fromTheme("mail-mark-read"), tr("Mark message as read")),
          &QAction::triggered,
          this,
          &MessagePreviewer::markMessageAsRead);
  connect(m_actionMarkUnread = m_toolBar->addAction(qApp->icons()->fromTheme("mail-mark-unread"), tr("Mark message as unread")),
          &QAction::triggered,
          this,
          &MessagePreviewer::markMessageAsUnread);
  connect(m_actionSwitchImportance = m_toolBar->addAction(qApp->icons()->fromTheme("mail-mark-important"), tr("Switch message importance")),
          &QAction::triggered,
          this,
          &MessagePreviewer::switchMessageImportance);
  connect(m_ui.m_txtMessage,
          static_cast<void (QTextBrowser::*)(const QString&)>(&QTextBrowser::highlighted),
          [=](const QString& text) {
    Q_UNUSED(text)
    QToolTip::showText(QCursor::pos(), tr("Click this link to download it or open it with external browser."), this);
  });
}

MessagePreviewer::MessagePreviewer(QWidget* parent) : QWidget(parent), m_pictures(QStringList()) {
  m_ui.setupUi(this);
  m_ui.m_txtMessage->viewport()->setAutoFillBackground(true);
  m_toolBar = new QToolBar(this);
  m_toolBar->setOrientation(Qt::Vertical);
  m_ui.m_layout->addWidget(m_toolBar, 0, 0, -1, 1);
  createConnections();

  m_actionSwitchImportance->setCheckable(true);
  m_ui.m_searchWidget->hide();

  reloadFontSettings();
  clear();
}

void MessagePreviewer::reloadFontSettings() {
  const Settings* settings = qApp->settings();
  QFont fon;

  fon.fromString(settings->value(GROUP(Messages), SETTING(Messages::PreviewerFontStandard)).toString());
  m_ui.m_txtMessage->setFont(fon);
}

void MessagePreviewer::clear() {
  m_ui.m_txtMessage->clear();
  m_pictures.clear();
  hide();
}

void MessagePreviewer::hideToolbar() {
  m_toolBar->setVisible(false);
}

void MessagePreviewer::loadMessage(const Message& message, RootItem* root) {
  m_message = message;
  m_root = root;

  if (!m_root.isNull()) {
    m_ui.m_searchWidget->hide();
    m_actionSwitchImportance->setChecked(m_message.m_isImportant);
    m_ui.m_txtMessage->setHtml(prepareHtmlForMessage(m_message));
    updateButtons();
    show();
    m_ui.m_txtMessage->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
  }
}

void MessagePreviewer::markMessageAsRead() {
  markMessageAsReadUnread(RootItem::Read);
}

void MessagePreviewer::markMessageAsUnread() {
  markMessageAsReadUnread(RootItem::Unread);
}

void MessagePreviewer::markMessageAsReadUnread(RootItem::ReadStatus read) {
  if (!m_root.isNull()) {
    if (m_root->getParentServiceRoot()->onBeforeSetMessagesRead(m_root.data(),
                                                                QList<Message>() << m_message,
                                                                read)) {
      DatabaseQueries::markMessagesReadUnread(qApp->database()->connection(objectName(), DatabaseFactory::DesiredType::FromSettings),
                                              QStringList() << QString::number(m_message.m_id),
                                              read);
      m_root->getParentServiceRoot()->onAfterSetMessagesRead(m_root.data(),
                                                             QList<Message>() << m_message,
                                                             read);
      m_message.m_isRead = read == RootItem::Read;
      emit markMessageRead(m_message.m_id, read);

      updateButtons();
    }
  }
}

void MessagePreviewer::switchMessageImportance(bool checked) {
  if (!m_root.isNull()) {
    if (m_root->getParentServiceRoot()->onBeforeSwitchMessageImportance(m_root.data(),
                                                                        QList<ImportanceChange>() << ImportanceChange(m_message,
                                                                                                                      m_message.
                                                                                                                      m_isImportant ?
                                                                                                                      RootItem::NotImportant
                                                                                                                                    :
                                                                                                                      RootItem::Important)))
    {
      DatabaseQueries::switchMessagesImportance(qApp->database()->connection(objectName(), DatabaseFactory::DesiredType::FromSettings),
                                                QStringList() << QString::number(m_message.m_id));
      m_root->getParentServiceRoot()->onAfterSwitchMessageImportance(m_root.data(),
                                                                     QList<ImportanceChange>() << ImportanceChange(m_message,
                                                                                                                   m_message.m_isImportant ?
                                                                                                                   RootItem::NotImportant :
                                                                                                                   RootItem::Important));
      emit markMessageImportant(m_message.m_id, checked ? RootItem::Important : RootItem::NotImportant);

      m_message.m_isImportant = checked;
    }
  }
}

bool MessagePreviewer::eventFilter(QObject* watched, QEvent* event) {
  Q_UNUSED(watched)

  if (event->type() == QEvent::KeyPress) {
    QKeyEvent* key_event = static_cast<QKeyEvent*>(event);

    if (key_event->matches(QKeySequence::StandardKey::Find)) {
      m_ui.m_searchWidget->clear();
      m_ui.m_searchWidget->show();
      m_ui.m_searchWidget->setFocus();
      return true;
    }
  }

  return false;
}

void MessagePreviewer::updateButtons() {
  m_actionMarkRead->setEnabled(!m_message.m_isRead);
  m_actionMarkUnread->setEnabled(m_message.m_isRead);
}

QString MessagePreviewer::prepareHtmlForMessage(const Message& message) {
  QString html = QString("<h2 align=\"center\">%1</h2>").arg(message.m_title);

  if (!message.m_url.isEmpty()) {
    html += QString("[url] <a href=\"%1\">%1</a><br/>").arg(message.m_url);
  }

  foreach (const Enclosure& enc, message.m_enclosures) {
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

  return html;
}
