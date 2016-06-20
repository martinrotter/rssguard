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

#include "gui/webbrowser.h"

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


void WebBrowser::createConnections() {
  connect(m_ui->m_webMessage, &WebViewer::messageStatusChangeRequested, this, &WebBrowser::receiveMessageStatusChangeRequest);
}

WebBrowser::WebBrowser(QWidget *parent) : TabContent(parent),
  m_ui(new Ui::WebBrowser) {
  m_ui->setupUi(this);

  createConnections();
  reloadFontSettings();
  clear();
}

WebBrowser::~WebBrowser() {
}

void WebBrowser::reloadFontSettings() {
  QFont fon;
  fon.fromString(qApp->settings()->value(GROUP(Messages),
                                         SETTING(Messages::PreviewerFontStandard)).toString());

  QWebEngineSettings::globalSettings()->setFontFamily(QWebEngineSettings::StandardFont, fon.family());
  QWebEngineSettings::globalSettings()->setFontSize(QWebEngineSettings::DefaultFontSize, fon.pointSize());
}

void WebBrowser::clear() {
  m_ui->m_webMessage->clear();
  hide();
}

void WebBrowser::loadMessages(const QList<Message> &messages, RootItem *root) {
  if (m_messages.size() == messages.size()) {
    for (int i = 0; i < messages.size(); i++) {
      if (m_messages.at(i).m_customId != messages.at(i).m_customId) {
        break;
      }

      if (i == messages.size() - 1) {
        // We checked last items, both collections contain the same messages.
        return;
      }
    }
  }

  m_messages = messages;
  m_root = root;

  if (!m_root.isNull()) {
    m_ui->m_webMessage->loadMessages(messages);
    show();
  }
}

void WebBrowser::loadMessage(const Message &message, RootItem *root) {
  loadMessages(QList<Message>() << message, root);
}

void WebBrowser::receiveMessageStatusChangeRequest(int message_id, WebPage::MessageStatusChange change) {
  switch (change) {
    case WebPage::MarkRead:
      markMessageAsRead(message_id, true);
      break;

    case WebPage::MarkUnread:
      markMessageAsRead(message_id, false);
      break;

    case WebPage::MarkStarred:
      switchMessageImportance(message_id, true);
      break;

    case WebPage::MarkUnstarred:
      switchMessageImportance(message_id, false);
      break;

    default:
      break;
  }
}

void WebBrowser::markMessageAsRead(int id, bool read) {
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

      emit markMessageRead(msg->m_id, read ? RootItem::Read : RootItem::Unread);
      msg->m_isRead = read ? RootItem::Read : RootItem::Unread;
    }
  }
}

void WebBrowser::switchMessageImportance(int id, bool checked) {
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

      emit markMessageImportant(msg->m_id, msg->m_isImportant ? RootItem::NotImportant : RootItem::Important);
      msg->m_isImportant = checked;
    }
  }
}

Message *WebBrowser::findMessage(int id) {
  for (int i = 0; i < m_messages.size(); i++) {
    if (m_messages.at(i).m_id == id) {
      return &m_messages[i];
    }
  }

  return nullptr;
}
