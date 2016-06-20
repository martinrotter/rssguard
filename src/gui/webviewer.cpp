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


WebViewer::WebViewer(QWidget *parent) : QWebEngineView(parent) {
  WebPage *page = new WebPage(this);

  connect(page, &WebPage::messageStatusChangeRequested,
          this, &WebViewer::messageStatusChangeRequested);
  setPage(page);
}

void WebViewer::displayMessage() {
  setHtml(m_messageContents, QUrl(INTERNAL_URL_MESSAGE));
}

void WebViewer::loadMessages(const QList<Message> &messages) {
  Skin skin = qApp->skins()->currentSkin();
  QString messages_layout;
  QString single_message_layout = skin.m_layoutMarkup;

  foreach (const Message &message, messages) {
    QString enclosures;

    foreach (const Enclosure &enclosure, message.m_enclosures) {
      enclosures += skin.m_enclosureMarkup.arg(enclosure.m_url, tr("Attachment"), enclosure.m_mimeType);
    }

    messages_layout.append(single_message_layout.arg(message.m_title,
                                                     tr("Written by ") + (message.m_author.isEmpty() ?
                                                                            tr("uknown author") :
                                                                            message.m_author),
                                                     message.m_url,
                                                     message.m_contents,
                                                     message.m_created.toString(Qt::DefaultLocaleShortDate),
                                                     enclosures,
                                                     message.m_isRead ? "mark-unread" : "mark-read",
                                                     message.m_isImportant ? "mark-unstarred" : "mark-starred",
                                                     QString::number(message.m_id)));
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
  setHtml("<!DOCTYPE html><html><body</body></html>", QUrl(INTERNAL_URL_BLANK));
}
