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

MessagePreviewer::MessagePreviewer(QWidget *parent) : QWidget(parent),
  m_ui(new Ui::MessagePreviewer) {
  m_ui->setupUi(this);
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
  // TODO: Reload font settings.
  //QFont saved_font =
}

void MessagePreviewer::clear() {
  m_ui->m_webMessage->setHtml("<html><body</body></html>", QUrl(INTERNAL_URL_BLANK));
  hide();
}

void MessagePreviewer::loadMessage(const Message &message, RootItem *root) {
  m_message = message;
  m_root = root;

  if (!m_root.isNull()) {
    m_actionSwitchImportance->setChecked(m_message.m_isImportant);
    m_ui->m_webMessage->loadMessage(message);
    updateButtons();
    show();
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
