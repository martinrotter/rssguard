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

#include "gui/newspaperpreviewer.h"

#include "gui/messagepreviewer.h"


NewspaperPreviewer::NewspaperPreviewer(RootItem *root, QList<Message> messages, QWidget *parent)
  : TabContent(parent), m_ui(new Ui::NewspaperPreviewer), m_root(root), m_messages(messages) {
  m_ui->setupUi(this);
  connect(m_ui->m_btnShowMoreMessages, SIGNAL(clicked(bool)), this, SLOT(showMoreMessages()));
  showMoreMessages();
}

NewspaperPreviewer::~NewspaperPreviewer() {
}

void NewspaperPreviewer::showMoreMessages() {
  if (!m_root.isNull()) {
    for (int i = 0; i < 10 && !m_messages.isEmpty(); i++) {
      Message msg = m_messages.takeFirst();
      MessagePreviewer *prev = new MessagePreviewer(this);

      connect(prev, SIGNAL(requestMessageListReload(bool)), this, SIGNAL(requestMessageListReload(bool)));

      prev->setFixedHeight(300);
      prev->loadMessage(msg, m_root);
      m_ui->m_layout->insertWidget(m_ui->m_layout->count() - 2, prev);
    }

    m_ui->m_btnShowMoreMessages->setText(tr("Show more messages (%n remaining)", "", m_messages.size()));
    m_ui->m_btnShowMoreMessages->setEnabled(!m_messages.isEmpty());

    // TODO: pokračovat, pridat signal void requestMessageListReload(bool mark_current_as_read);
    // ktery bude forwardovar tentyz signal z toho message previeweru kazdeho
    // a ten signal navazat na obnoveni seznamu zprav
    //
    // taky opravit spojeni v pripade ze se zada o novinovy nahled z feedviewu
  }
  else {
    // TODO: ukazat chybu
  }
}
