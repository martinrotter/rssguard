// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

// You may use this file under the terms of the BSD license as follows:
//
// "Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in
//     the documentation and/or other materials provided with the
//     distribution.
//   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
//     of its contributors may be used to endorse or promote products derived
//     from this software without specific prior written permission.
//
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."

#include "network-web/googlesuggest.h"

#include "definitions/definitions.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "gui/locationlineedit.h"

#include <QListWidget>
#include <QXmlStreamReader>
#include <QTimer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QKeyEvent>
#include <QDomDocument>
#include <QTextCodec>


GoogleSuggest::GoogleSuggest(LocationLineEdit *editor, QObject *parent) : QObject(parent), editor(editor) {
  popup = new QListWidget();
  popup->setWindowFlags(Qt::Popup);
  popup->setFocusPolicy(Qt::NoFocus);
  popup->setFocusProxy(editor);
  popup->setMouseTracking(true);
  popup->setSelectionBehavior(QAbstractItemView::SelectRows);
  popup->setFrameStyle(QFrame::Box | QFrame::Plain);
  popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  popup->installEventFilter(this);

  timer = new QTimer(this);
  timer->setSingleShot(true);
  timer->setInterval(500);

  connect(popup, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(doneCompletion()));
  connect(timer, SIGNAL(timeout()), SLOT(autoSuggest()));
  connect(editor, SIGNAL(textEdited(QString)), timer, SLOT(start()));
}

GoogleSuggest::~GoogleSuggest() {
  delete popup;
}

bool GoogleSuggest::eventFilter(QObject *object, QEvent *event) {
  if (object != popup) {
    return false;
  }

  if (event->type() == QEvent::MouseButtonPress) {
    popup->hide();
    editor->setFocus();
    return true;
  }

  if (event->type() == QEvent::KeyPress) {
    bool consumed = false;
    int key = static_cast<QKeyEvent*>(event)->key();

    switch (key) {
      case Qt::Key_Enter:
      case Qt::Key_Return:
        doneCompletion();
        consumed = true;

      case Qt::Key_Escape:
        editor->setFocus();
        popup->hide();
        consumed = true;

      case Qt::Key_Up:
      case Qt::Key_Down:
      case Qt::Key_Home:
      case Qt::Key_End:
      case Qt::Key_PageUp:
      case Qt::Key_PageDown:
        break;

      default:
        editor->setFocus();
        editor->event(event);
        popup->hide();
        break;
    }

    return consumed;
  }

  return false;
}

void GoogleSuggest::showCompletion(const QStringList &choices) {
  if (choices.isEmpty()) {
    return;
  }

  popup->setUpdatesEnabled(false);
  popup->clear();

  foreach (const QString &choice, choices) {
    new QListWidgetItem(choice, popup);
  }

  popup->setCurrentItem(popup->item(0));
  popup->adjustSize();
  popup->setUpdatesEnabled(true);
  popup->resize(editor->width(), popup->sizeHintForRow(0) * qMin(7, choices.count()) + 3);
  popup->move(editor->mapToGlobal(QPoint(0, editor->height())));
  popup->setFocus();
  popup->show();
}

void GoogleSuggest::doneCompletion() {
  timer->stop();
  popup->hide();
  editor->setFocus();

  QListWidgetItem *item = popup->currentItem();

  if (item != NULL) {
    editor->submit(QString(GOOGLE_SEARCH_URL).arg(item->text()));
  }
}

void GoogleSuggest::preventSuggest() {
  timer->stop();
}

void GoogleSuggest::autoSuggest() {
  QString str = QUrl::toPercentEncoding(editor->text());
  QString url = QString(GOOGLE_SUGGEST_URL).arg(str);

  connect(SilentNetworkAccessManager::instance()->get(QNetworkRequest(QString(url))), SIGNAL(finished()),
          this, SLOT(handleNetworkData()));
}

void GoogleSuggest::handleNetworkData() {
  QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

  if (!reply->error()) {
    QStringList choices;
    QDomDocument xml;
    QByteArray response = reply->readAll();

    QTextCodec *c = QTextCodec::codecForUtfText(response);
    xml.setContent(c->toUnicode(response));

    QDomNodeList suggestions = xml.elementsByTagName(QSL("suggestion"));

    for (int i = 0; i < suggestions.size(); i++) {
      QDomElement element = suggestions.at(i).toElement();

      if (element.attributes().contains(QSL("data"))) {
        choices.append(element.attribute(QSL("data")));
      }
    }

    showCompletion(choices);
  }

  reply->deleteLater();
}
