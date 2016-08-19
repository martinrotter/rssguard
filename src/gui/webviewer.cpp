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

#include "gui/webviewer.h"

#include "miscellaneous/skinfactory.h"
#include "miscellaneous/application.h"
#include "definitions/definitions.h"
#include "network-web/webpage.h"
#include "gui/dialogs/formmain.h"
#include "gui/tabwidget.h"
#include "gui/webbrowser.h"

#include <QWheelEvent>


WebViewer::WebViewer(QWidget *parent) : QWebEngineView(parent) {
  WebPage *page = new WebPage(this);

  connect(page, &WebPage::messageStatusChangeRequested, this, &WebViewer::messageStatusChangeRequested);
  setPage(page);
}

bool WebViewer::canIncreaseZoom() {
  return zoomFactor() <= MAX_ZOOM_FACTOR - ZOOM_FACTOR_STEP;
}

bool WebViewer::canDecreaseZoom() {
  return zoomFactor() >= MIN_ZOOM_FACTOR + ZOOM_FACTOR_STEP;
}

void WebViewer::displayMessage() {
  //load(QUrl(INTERNAL_URL_MESSAGE));
  setHtml(m_messageContents, QUrl::fromUserInput(INTERNAL_URL_MESSAGE));
}

bool WebViewer::increaseWebPageZoom() {
  if (canIncreaseZoom()) {
    setZoomFactor(zoomFactor() + ZOOM_FACTOR_STEP);
    return true;
  }
  else {
    return false;
  }
}

bool WebViewer::decreaseWebPageZoom() {
  if (canDecreaseZoom()) {
    setZoomFactor(zoomFactor() - ZOOM_FACTOR_STEP);
    return true;
  }
  else {
    return false;
  }
}

bool WebViewer::resetWebPageZoom() {
  const qreal new_factor = 1.0;

  if (new_factor != zoomFactor()) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}

void WebViewer::loadMessages(const QList<Message> &messages) {
  Skin skin = qApp->skins()->currentSkin();
  QString messages_layout;
  QString single_message_layout = skin.m_layoutMarkup;

  foreach (const Message &message, messages) {
    QString enclosures;
    QString enclosure_images;

    foreach (const Enclosure &enclosure, message.m_enclosures) {
      enclosures += skin.m_enclosureMarkup.arg(enclosure.m_url, tr("Attachment"), enclosure.m_mimeType);

      if (enclosure.m_mimeType.startsWith(QSL("image/"))) {
        // Add thumbnail image.
        enclosure_images += skin.m_enclosureImageMarkup.arg(enclosure.m_url, enclosure.m_mimeType)  ;
      }
    }

    messages_layout.append(single_message_layout
                           .arg(message.m_title,
                                tr("Written by ") + (message.m_author.isEmpty() ?
                                                       tr("unknown author") :
                                                       message.m_author),
                                message.m_url,
                                message.m_contents,
                                message.m_created.toString(Qt::DefaultLocaleShortDate),
                                enclosures,
                                message.m_isRead ? "mark-unread" : "mark-read",
                                message.m_isImportant ? "mark-unstarred" : "mark-starred",
                                QString::number(message.m_id))
                           .arg(enclosure_images));
  }

  m_messageContents = skin.m_layoutMarkupWrapper.arg(messages.size() == 1 ? messages.at(0).m_title : tr("Newspaper view"),
                                                     messages_layout);
  bool previously_enabled = isEnabled();

  setEnabled(false);
  displayMessage();
  setEnabled(previously_enabled);
}

void WebViewer::loadMessage(const Message &message) {
  loadMessages(QList<Message>() << message);
}

void WebViewer::clear() {
  bool previously_enabled = isEnabled();

  setEnabled(false);
  setHtml("<!DOCTYPE html><html><body</body></html>", QUrl(INTERNAL_URL_BLANK));
  setEnabled(previously_enabled);
}

QWebEngineView *WebViewer::createWindow(QWebEnginePage::WebWindowType type) {
  Q_UNUSED(type)

  int index = qApp->mainForm()->tabWidget()->addBrowser(false, false);

  if (index >= 0) {
    return qApp->mainForm()->tabWidget()->widget(qApp->mainForm()->tabWidget()->addBrowser(false, false))->webBrowser()->viewer();
  }
  else {
    return nullptr;
  }
}

void WebViewer::wheelEvent(QWheelEvent *event) {
  QWebEngineView::wheelEvent(event);

  if ((event->modifiers() & Qt::ControlModifier) > 0) {
    if (event->delta() > 0) {
      increaseWebPageZoom();
    }
    else if (event->delta() < 0) {
      decreaseWebPageZoom();
    }
  }
}
