// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagepreviewer.h"

#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "gui/searchtextwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "network-web/webfactory.h"
#include "services/abstract/serviceroot.h"

#if defined (USE_WEBENGINE)
#include "gui/webbrowser.h"
#else
#include "gui/messagetextbrowser.h"
#endif

#include <QGridLayout>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QToolBar>
#include <QToolTip>

void MessagePreviewer::createConnections() {
  installEventFilter(this);

#if defined (USE_WEBENGINE)
#else
  connect(m_searchWidget, &SearchTextWidget::cancelSearch, this, [this]() {
    m_txtMessage->textCursor().clearSelection();
    m_txtMessage->moveCursor(QTextCursor::MoveOperation::Left);
  });
  connect(m_searchWidget, &SearchTextWidget::searchForText, this, [this](const QString& text, bool backwards) {
    if (backwards) {
      m_txtMessage->find(text, QTextDocument::FindFlag::FindBackward);
    }
    else {
      m_txtMessage->find(text);
    }
  });

  connect(m_txtMessage, &QTextBrowser::anchorClicked, [=](const QUrl& url) {
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
  connect(m_txtMessage,
          QOverload<const QUrl&>::of(&QTextBrowser::highlighted),
          [=](const QUrl& url) {
    Q_UNUSED(url)
    QToolTip::showText(QCursor::pos(), tr("Click this link to download it or open it with external browser."), this);
  });
#endif

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
}

MessagePreviewer::MessagePreviewer(QWidget* parent)
  : QWidget(parent), m_layout(new QGridLayout(this)), m_toolBar(new QToolBar(this)) {
#if defined (USE_WEBENGINE)
  m_txtMessage = new WebBrowser(this);
#else
  m_txtMessage = new MessageTextBrowser(this);
  m_searchWidget = new SearchTextWidget(this);
#endif

  m_toolBar->setOrientation(Qt::Vertical);
  m_layout->setContentsMargins(3, 3, 3, 3);
  m_layout->addWidget(m_txtMessage, 0, 1, 1, 1);

#if !defined (USE_WEBENGINE)
  m_layout->addWidget(m_searchWidget, 1, 1, 1, 1);
#endif

  m_layout->addWidget(m_toolBar, 0, 0, -1, 1);

  createConnections();
  m_actionSwitchImportance->setCheckable(true);

#if defined (USE_WEBENGINE)
#else
  m_searchWidget->hide();
#endif
  reloadFontSettings();
  clear();
}

void MessagePreviewer::reloadFontSettings() {
  m_txtMessage->reloadFontSettings();
}

#if defined (USE_WEBENGINE)

WebBrowser* MessagePreviewer::webBrowser() const {
  return m_txtMessage;
}

#endif

void MessagePreviewer::clear() {
  m_txtMessage->clear();
  hide();
}

void MessagePreviewer::hideToolbar() {
  m_toolBar->setVisible(false);
}

void MessagePreviewer::loadMessage(const Message& message, RootItem* root) {
  m_message = message;
  m_root = root;

  if (!m_root.isNull()) {
    updateButtons();
    show();
    m_actionSwitchImportance->setChecked(m_message.m_isImportant);
    m_txtMessage->loadMessage(message, root);

#if !defined (USE_WEBENGINE)
    m_searchWidget->hide();
    m_txtMessage->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
#endif
  }
}

void MessagePreviewer::markMessageAsRead() {
  markMessageAsReadUnread(RootItem::ReadStatus::Read);
}

void MessagePreviewer::markMessageAsUnread() {
  markMessageAsReadUnread(RootItem::ReadStatus::Unread);
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
      m_message.m_isRead = read == RootItem::ReadStatus::Read;
      emit markMessageRead(m_message.m_id, read);

      updateButtons();
    }
  }
}

void MessagePreviewer::switchMessageImportance(bool checked) {
  if (!m_root.isNull()) {
    if (m_root->getParentServiceRoot()->onBeforeSwitchMessageImportance(m_root.data(),
                                                                        QList<ImportanceChange>()
                                                                        << ImportanceChange(m_message,
                                                                                            m_message.
                                                                                            m_isImportant
                                                                                            ? RootItem::Importance::NotImportant
                                                                                            : RootItem::Importance::Important))) {
      DatabaseQueries::switchMessagesImportance(qApp->database()->connection(objectName(), DatabaseFactory::DesiredType::FromSettings),
                                                QStringList() << QString::number(m_message.m_id));
      m_root->getParentServiceRoot()->onAfterSwitchMessageImportance(m_root.data(),
                                                                     QList<ImportanceChange>()
                                                                     << ImportanceChange(m_message,
                                                                                         m_message.m_isImportant
                                                                                         ? RootItem::Importance::NotImportant
                                                                                         : RootItem::Importance::Important));
      emit markMessageImportant(m_message.m_id, checked
                                ? RootItem::Importance::Important
                                : RootItem::Importance::NotImportant);

      m_message.m_isImportant = checked;
    }
  }
}

bool MessagePreviewer::eventFilter(QObject* watched, QEvent* event) {
  Q_UNUSED(watched)

#if !defined (USE_WEBENGINE)
  if (event->type() == QEvent::Type::KeyPress) {
    auto* key_event = static_cast<QKeyEvent*>(event);

    if (key_event->matches(QKeySequence::StandardKey::Find)) {

      m_searchWidget->clear();
      m_searchWidget->show();
      m_searchWidget->setFocus();
      return true;
    }
  }
#endif

  return false;
}

void MessagePreviewer::updateButtons() {
  m_actionMarkRead->setEnabled(!m_message.m_isRead);
  m_actionMarkUnread->setEnabled(m_message.m_isRead);
}
