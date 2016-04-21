// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "gui/messagepreviewer.h"

#include "miscellaneous/application.h"
#include "network-web/webfactory.h"
#include "miscellaneous/databasequeries.h"
#include "gui/messagebox.h"
#include "gui/dialogs/formmain.h"
#include "services/abstract/serviceroot.h"

#include <QScrollBar>
#include <QToolBar>
#include <QToolTip>


void MessagePreviewer::createConnections() {
  connect(m_ui->m_txtMessage, &QTextBrowser::anchorClicked, [=](const QUrl &url) {
    if (!url.isEmpty()) {
      // User clicked some URL. Open it in external browser or download?
      MessageBox box(qApp->mainForm());

      box.setText(tr("You clicked some link. You can download the link contents or open it in external web browser."));
      box.setInformativeText(tr("What action do you want to take?"));
      box.setDetailedText(url.toString());
      QAbstractButton *btn_open = box.addButton(tr("Open in external browser"), QMessageBox::ActionRole);
      QAbstractButton *btn_download = box.addButton(tr("Download"), QMessageBox::ActionRole);
      QAbstractButton *btn_cancel = box.addButton(QMessageBox::Cancel);

      box.setDefaultButton(QMessageBox::Cancel);
      box.exec();

      if (box.clickedButton() == btn_open) {
        WebFactory::instance()->openUrlInExternalBrowser(url.toString());
      }
      else if (box.clickedButton() == btn_download) {
        qApp->downloadManager()->download(url);
      }

      btn_download->deleteLater();
      btn_open->deleteLater();
      btn_cancel->deleteLater();
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
  connect(m_actionSwitchImportance = m_toolBar->addAction(qApp->icons()->fromTheme("mail-mark-favorite"), tr("Switch message importance")),
          &QAction::triggered,
          this,
          &MessagePreviewer::switchMessageImportance);
  connect(m_ui->m_txtMessage,
          static_cast<void (QTextBrowser::*)(const QString&)>(&QTextBrowser::highlighted),
          [=](const QString &text) {
    Q_UNUSED(text)

    QToolTip::showText(QCursor::pos(), tr("Click this link to download it or open it with external browser."), this);
  });
}

MessagePreviewer::MessagePreviewer(QWidget *parent) : QWidget(parent),
  m_ui(new Ui::MessagePreviewer), m_pictures(QStringList()) {
  m_ui->setupUi(this);
  m_ui->m_txtMessage->viewport()->setAutoFillBackground(true);
  m_toolBar = new QToolBar(this);
  m_toolBar->setOrientation(Qt::Vertical);
  m_ui->m_layout->addWidget(m_toolBar, 0, 0, -1, 1);

  createConnections();

  m_actionSwitchImportance->setCheckable(true);

  reloadFontSettings();
  clear();
}

MessagePreviewer::~MessagePreviewer() {
}

void MessagePreviewer::reloadFontSettings() {
  const Settings *settings = qApp->settings();
  QFont fon;

  fon.fromString(settings->value(GROUP(Messages),
                                 SETTING(Messages::PreviewerFontStandard)).toString());

  m_ui->m_txtMessage->setFont(fon);
}

void MessagePreviewer::clear() {
  m_ui->m_txtMessage->clear();
  m_pictures.clear();
  hide();
}

void MessagePreviewer::loadMessage(const Message &message, RootItem *root) {
  m_message = message;
  m_root = root;

  if (!m_root.isNull()) {
    m_actionSwitchImportance->setChecked(m_message.m_isImportant);
    m_ui->m_txtMessage->setHtml(prepareHtmlForMessage(m_message));

    updateButtons();
    show();

    m_ui->m_txtMessage->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
  }
}

void MessagePreviewer::markMessageAsRead() {
  if (!m_root.isNull()) {
    if (m_root->getParentServiceRoot()->onBeforeSetMessagesRead(m_root.data(),
                                                                QList<Message>() << m_message,
                                                                RootItem::Read)) {
      DatabaseQueries::markMessagesReadUnread(qApp->database()->connection(objectName(), DatabaseFactory::FromSettings),
                                              QStringList() << QString::number(m_message.m_id),
                                              RootItem::Read);
      m_root->getParentServiceRoot()->onAfterSetMessagesRead(m_root.data(),
                                                             QList<Message>() << m_message,
                                                             RootItem::Read);

      emit requestMessageListReload(false);
      m_message.m_isRead = true;
      updateButtons();
    }
  }
}

void MessagePreviewer::markMessageAsUnread() {
  if (!m_root.isNull()) {
    if (m_root->getParentServiceRoot()->onBeforeSetMessagesRead(m_root.data(),
                                                                QList<Message>() << m_message,
                                                                RootItem::Unread)) {
      DatabaseQueries::markMessagesReadUnread(qApp->database()->connection(objectName(), DatabaseFactory::FromSettings),
                                              QStringList() << QString::number(m_message.m_id),
                                              RootItem::Unread);
      m_root->getParentServiceRoot()->onAfterSetMessagesRead(m_root.data(),
                                                             QList<Message>() << m_message,
                                                             RootItem::Unread);

      emit requestMessageListReload(false);
      m_message.m_isRead = false;
      updateButtons();
    }
  }
}

void MessagePreviewer::switchMessageImportance(bool checked) {
  if (!m_root.isNull()) {
    if (m_root->getParentServiceRoot()->onBeforeSwitchMessageImportance(m_root.data(),
                                                                        QList<ImportanceChange>() << ImportanceChange(m_message,
                                                                                                                      m_message.m_isImportant ?
                                                                                                                      RootItem::NotImportant :
                                                                                                                      RootItem::Important))) {
      DatabaseQueries::switchMessagesImportance(qApp->database()->connection(objectName(), DatabaseFactory::FromSettings),
                                                QStringList() << QString::number(m_message.m_id));

      m_root->getParentServiceRoot()->onBeforeSwitchMessageImportance(m_root.data(),
                                                                      QList<ImportanceChange>() << ImportanceChange(m_message,
                                                                                                                    m_message.m_isImportant ?
                                                                                                                      RootItem::NotImportant :
                                                                                                                      RootItem::Important));

      emit requestMessageListReload(false);
      m_message.m_isImportant = checked;
    }
  }
}

void MessagePreviewer::updateButtons() {
  m_actionMarkRead->setEnabled(!m_message.m_isRead);
  m_actionMarkUnread->setEnabled(m_message.m_isRead);
}

QString MessagePreviewer::prepareHtmlForMessage(const Message &message) {
  QString html = QString("<h2 align=\"center\">%1</h2>").arg(message.m_title);

  html += QString("[url] <a href=\"%1\">%1</a><br/>").arg(message.m_url);

  foreach (const Enclosure &enc, message.m_enclosures) {
    html += QString("[%2] <a href=\"%1\">%1</a><br/>").arg(enc.m_url, enc.m_mimeType);
  }

  int offset = 0;
  QRegExp imgTagRegex("\\<img[^\\>]*src\\s*=\\s*\"([^\"]*)\"[^\\>]*\\>", Qt::CaseInsensitive);

  imgTagRegex.setMinimal(true);

  while( (offset = imgTagRegex.indexIn(message.m_contents, offset)) != -1){
    m_pictures.append(imgTagRegex.cap(1));

    offset += imgTagRegex.matchedLength();
    html += QString("[%2] <a href=\"%1\">%1</a><br/>").arg(imgTagRegex.cap(1), tr("image"));
  }

  html += "<hr/>";
  html += message.m_contents;

  return html;
}
