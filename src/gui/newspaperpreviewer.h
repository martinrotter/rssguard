// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef NEWSPAPERPREVIEWER_H
#define NEWSPAPERPREVIEWER_H

#include <QWidget>

#include "gui/tabcontent.h"

#include "ui_newspaperpreviewer.h"

#include "core/message.h"
#include "services/abstract/rootitem.h"

#include <QPointer>

namespace Ui {
  class NewspaperPreviewer;
}

class RootItem;

class NewspaperPreviewer : public TabContent {
  Q_OBJECT

  public:
    explicit NewspaperPreviewer(RootItem* root, QList<Message> messages, QWidget* parent = 0);
    virtual ~NewspaperPreviewer();

  private slots:
    void showMoreMessages();

  signals:
    void requestMessageListReload(bool mark_current_as_read);

  private:
    QScopedPointer<Ui::NewspaperPreviewer> m_ui;
    QPointer<RootItem> m_root;
    QList<Message> m_messages;
};

#endif // NEWSPAPERPREVIEWER_H
