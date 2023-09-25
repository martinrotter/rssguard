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
  : QObject(parent), m_editor(editor), m_downloader(new Downloader(this)), m_popup(new QListWidget()),
    m_enteredText(QString()) {
  m_popup->setWindowFlags(Qt::WindowType::Popup);
  m_popup->setFocusPolicy(Qt::FocusPolicy::NoFocus);
  m_popup->setFocusProxy(editor);
  m_popup->setMouseTracking(true);
  m_popup->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  m_popup->setFrameStyle(QFrame::Shape::Box | QFrame::Shadow::Plain);
  m_popup->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
  m_popup->installEventFilter(this);
  m_timer = new QTimer(this);
  m_timer->setSingleShot(true);
  m_timer->setInterval(1500);

  connect(m_popup.data(), &QListWidget::itemClicked, this, &GoogleSuggest::doneCompletion);
  connect(m_timer, &QTimer::timeout, this, &GoogleSuggest::autoSuggest);
  connect(editor, &LocationLineEdit::textEdited, m_timer, static_cast<void (QTimer::*)()>(&QTimer::start));
  connect(m_downloader.data(), &Downloader::completed, this, &GoogleSuggest::handleNetworkData);
}

bool GoogleSuggest::eventFilter(QObject* object, QEvent* event) {
  if (object != m_popup.data()) {
    return false;
  }

  if (event->type() == QEvent::MouseButtonPress) {
    m_popup->hide();
    m_editor->setFocus();
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
        m_editor->setFocus();
        m_popup->hide();
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
        m_editor->setFocus();
        m_editor->event(event);
        m_popup->hide();
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

  m_popup->setUpdatesEnabled(false);
  m_popup->clear();

  for (const QString& choice : choices) {
    new QListWidgetItem(choice, m_popup.data());
  }

  m_popup->setCurrentItem(m_popup->item(0));
  m_popup->adjustSize();
  m_popup->setUpdatesEnabled(true);
  m_popup->resize(m_editor->width(), m_popup->sizeHintForRow(0) * qMin(7, choices.count()) + 3);
  m_popup->move(m_editor->mapToGlobal(QPoint(0, m_editor->height())));
  // m_popup->setFocus();
  m_popup->show();
}

void GoogleSuggest::doneCompletion() {
  m_timer->stop();
  m_popup->hide();
  m_editor->setFocus();
  QListWidgetItem* item = m_popup->currentItem();

  if (item != nullptr) {
    m_editor->submit(QSL(GOOGLE_SEARCH_URL).arg(item->text()));
  }
}

void GoogleSuggest::preventSuggest() {
  m_timer->stop();
}

void GoogleSuggest::autoSuggest() {
  QUrl entered_url = QUrl::fromUserInput(m_editor->text());

  if (m_editor->text().size() < 3 || m_editor->text().startsWith(QSL("http")) ||
      m_editor->text().startsWith(QSL("www")) ||
      (entered_url.isValid() && !entered_url.isLocalFile() &&
       (!entered_url.scheme().isEmpty() || entered_url.host().contains(QL1C('.'))))) {
    // Do not suggest when entered URL.
    preventSuggest();
    return;
  }

  m_enteredText = QUrl::toPercentEncoding(m_editor->text());
  QString url = QSL(GOOGLE_SUGGEST_URL).arg(m_enteredText);

  m_downloader->downloadFile(url);
}

void GoogleSuggest::handleNetworkData(const QUrl& url,
                                      QNetworkReply::NetworkError status,
                                      int http_code,
                                      const QByteArray& contents) {
  Q_UNUSED(url)
  Q_UNUSED(http_code)

  if (status == QNetworkReply::NetworkError::NoError) {
    const QTextCodec* c = QTextCodec::codecForUtfText(contents);

    QDomDocument xml;
    xml.setContent(c->toUnicode(contents));
    QDomNodeList suggestions = xml.elementsByTagName(QSL("suggestion"));
    QStringList choices;

    choices.reserve(suggestions.size());

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
