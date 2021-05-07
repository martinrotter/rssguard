// For license of this file, see <project-root-folder>/LICENSE.md.

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
#include "gui/reusable/locationlineedit.h"
#include "network-web/downloader.h"

#include <QDomDocument>
#include <QKeyEvent>
#include <QListWidget>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTextCodec>
#include <QTimer>
#include <QXmlStreamReader>

GoogleSuggest::GoogleSuggest(LocationLineEdit* editor, QObject* parent)
  : QObject(parent), editor(editor), m_downloader(new Downloader(this)), popup(new QListWidget()), m_enteredText(QString()) {
  popup->setWindowFlags(Qt::WindowType::Popup);
  popup->setFocusPolicy(Qt::FocusPolicy::NoFocus);
  popup->setFocusProxy(editor);
  popup->setMouseTracking(true);
  popup->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  popup->setFrameStyle(QFrame::Shape::Box | QFrame::Shadow::Plain);
  popup->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
  popup->installEventFilter(this);
  timer = new QTimer(this);
  timer->setSingleShot(true);
  timer->setInterval(500);

  connect(popup.data(), &QListWidget::itemClicked, this, &GoogleSuggest::doneCompletion);
  connect(timer, &QTimer::timeout, this, &GoogleSuggest::autoSuggest);
  connect(editor, &LocationLineEdit::textEdited, timer, static_cast<void (QTimer::*)()>(&QTimer::start));
  connect(m_downloader.data(), &Downloader::completed, this, &GoogleSuggest::handleNetworkData);
}

bool GoogleSuggest::eventFilter(QObject* object, QEvent* event) {
  if (object != popup.data()) {
    return false;
  }

  if (event->type() == QEvent::MouseButtonPress) {
    popup->hide();
    editor->setFocus();
    return true;
  }

  if (event->type() == QEvent::KeyPress) {
    bool consumed = false;
    const int key = static_cast<QKeyEvent*>(event)->key();

    switch (key) {
      case Qt::Key_Enter:
      case Qt::Key_Return:
        doneCompletion();
        consumed = true;
        break;

      case Qt::Key_Escape:
        editor->setFocus();
        popup->hide();
        consumed = true;
        break;

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

void GoogleSuggest::showCompletion(const QStringList& choices) {
  if (choices.isEmpty()) {
    return;
  }

  popup->setUpdatesEnabled(false);
  popup->clear();

  for (const QString& choice : choices) {
    new QListWidgetItem(choice, popup.data());
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
  QListWidgetItem* item = popup->currentItem();

  if (item != nullptr) {
    editor->submit(QString(GOOGLE_SEARCH_URL).arg(item->text()));
  }
}

void GoogleSuggest::preventSuggest() {
  timer->stop();
}

void GoogleSuggest::autoSuggest() {
  m_enteredText = QUrl::toPercentEncoding(editor->text());
  QString url = QString(GOOGLE_SUGGEST_URL).arg(m_enteredText);

  m_downloader->downloadFile(url);
}

void GoogleSuggest::handleNetworkData(QNetworkReply::NetworkError status, const QByteArray& contents) {
  if (status == QNetworkReply::NetworkError::NoError) {
    QStringList choices;
    QDomDocument xml;
    const QTextCodec* c = QTextCodec::codecForUtfText(contents);

    xml.setContent(c->toUnicode(contents));
    QDomNodeList suggestions = xml.elementsByTagName(QSL("suggestion"));

    for (int i = 0; i < suggestions.size(); i++) {
      const QDomElement element = suggestions.at(i).toElement();

      if (element.attributes().contains(QSL("data"))) {
        choices.append(element.attribute(QSL("data")));
      }
    }

    if (choices.isEmpty()) {
      choices.append(m_enteredText);
    }

    showCompletion(choices);
  }
}
