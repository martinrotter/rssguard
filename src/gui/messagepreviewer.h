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

#ifndef MESSAGEPREVIEWER_H
#define MESSAGEPREVIEWER_H

#include "gui/tabcontent.h"

#include "ui_messagepreviewer.h"

#include "core/message.h"
#include "services/abstract/rootitem.h"

#include <QPointer>


namespace Ui {
  class MessagePreviewer;
}

class QToolBar;

class MessagePreviewer : public TabContent {
    Q_OBJECT

  public:
    explicit MessagePreviewer(QWidget *parent = 0);
    virtual ~MessagePreviewer();

    void reloadFontSettings();

  public slots:
    void clear();
    void loadMessages(const QList<Message> &messages, RootItem *root);
    void loadMessage(const Message &message, RootItem *root);

  private slots:
    void receiveMessageStatusChangeRequest(int message_id, MessageBrowserPage::MessageStatusChange change);

  signals:
    void requestMessageListReload(bool mark_current_as_read);

  private:
    Message *findMessage(int id);
    void markMessageAsRead(int id, bool read);
    void switchMessageImportance(int id, bool checked);
    void createConnections();

    QScopedPointer<Ui::MessagePreviewer> m_ui;
    QList<Message> m_messages;
    QPointer<RootItem> m_root;
};

#endif // MESSAGEPREVIEWER_H
