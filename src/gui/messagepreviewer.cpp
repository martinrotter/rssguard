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
#include <QWebEngineSettings>
#include <QToolTip>


void MessagePreviewer::createConnections() {
  connect(m_ui->m_webMessage, &MessageBrowser::messageStatusChangeRequested,
          this, &MessagePreviewer::receiveMessageStatusChangeRequest);
}

MessagePreviewer::MessagePreviewer(QWidget *parent) : TabContent(parent),
  m_ui(new Ui::MessagePreviewer) {
  m_ui->setupUi(this);

  createConnections();
  reloadFontSettings();
  clear();
}

MessagePreviewer::~MessagePreviewer() {
}

void MessagePreviewer::reloadFontSettings() {
  QFont fon;
  fon.fromString(qApp->settings()->value(GROUP(Messages),
                                         SETTING(Messages::PreviewerFontStandard)).toString());

  QWebEngineSettings::globalSettings()->setFontFamily(QWebEngineSettings::StandardFont, fon.family());
  QWebEngineSettings::globalSettings()->setFontSize(QWebEngineSettings::DefaultFontSize, fon.pointSize());
}

void MessagePreviewer::clear() {
  m_ui->m_webMessage->clear();
  hide();
}

void MessagePreviewer::loadMessages(const QList<Message> &messages, RootItem *root) {
  m_messages = messages;
  m_root = root;

  if (!m_root.isNull()) {
    m_ui->m_webMessage->loadMessages(messages);
    show();
  }
}

void MessagePreviewer::loadMessage(const Message &message, RootItem *root) {
  loadMessages(QList<Message>() << message, root);
}

void MessagePreviewer::receiveMessageStatusChangeRequest(int message_id, MessageBrowserPage::MessageStatusChange change) {
  switch (change) {
    case MessageBrowserPage::MarkRead:
      markMessageAsRead(message_id, true);
      break;

    case MessageBrowserPage::MarkUnread:
      markMessageAsRead(message_id, false);
      break;

    case MessageBrowserPage::MarkStarred:
      switchMessageImportance(message_id, true);
      break;

    case MessageBrowserPage::MarkUnstarred:
      switchMessageImportance(message_id, false);
      break;

    default:
      break;
  }
}

void MessagePreviewer::markMessageAsRead(int id, bool read) {
  if (!m_root.isNull()) {
    Message *msg = findMessage(id);

    if (msg != nullptr && m_root->getParentServiceRoot()->onBeforeSetMessagesRead(m_root.data(),
                                                                                  QList<Message>() << *msg,
                                                                                  read ? RootItem::Read : RootItem::Unread)) {
      DatabaseQueries::markMessagesReadUnread(qApp->database()->connection(objectName(), DatabaseFactory::FromSettings),
                                              QStringList() << QString::number(msg->m_id),
                                              read ? RootItem::Read : RootItem::Unread);
      m_root->getParentServiceRoot()->onAfterSetMessagesRead(m_root.data(),
                                                             QList<Message>() << *msg,
                                                             read ? RootItem::Read : RootItem::Unread);

      emit requestMessageListReload(false);
      msg->m_isRead = read ? RootItem::Read : RootItem::Unread;
    }
  }
}

void MessagePreviewer::switchMessageImportance(int id, bool checked) {
  if (!m_root.isNull()) {
    Message *msg = findMessage(id);

    if (msg != nullptr && m_root->getParentServiceRoot()->onBeforeSwitchMessageImportance(m_root.data(),
                                                                                          QList<ImportanceChange>() << ImportanceChange(*msg,
                                                                                                                                        msg->m_isImportant ?
                                                                                                                                        RootItem::NotImportant :
                                                                                                                                        RootItem::Important))) {
      DatabaseQueries::switchMessagesImportance(qApp->database()->connection(objectName(), DatabaseFactory::FromSettings),
                                                QStringList() << QString::number(msg->m_id));

      m_root->getParentServiceRoot()->onBeforeSwitchMessageImportance(m_root.data(),
                                                                      QList<ImportanceChange>() << ImportanceChange(*msg,
                                                                                                                    msg->m_isImportant ?
                                                                                                                      RootItem::NotImportant :
                                                                                                                      RootItem::Important));

      emit requestMessageListReload(false);
      msg->m_isImportant = checked;
    }
  }
}

Message *MessagePreviewer::findMessage(int id) {
  for (int i = 0; i < m_messages.size(); i++) {
    if (m_messages.at(i).m_id == id) {
      return &m_messages[i];
    }
  }

  return nullptr;
}
