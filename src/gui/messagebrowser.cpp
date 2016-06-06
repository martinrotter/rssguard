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

#include "gui/messagebrowser.h"

#include "miscellaneous/skinfactory.h"
#include "miscellaneous/application.h"
#include "definitions/definitions.h"
#include "network-web/messagebrowserpage.h"


MessageBrowser::MessageBrowser(QWidget *parent) : QWebEngineView(parent) {
  setPage(new MessageBrowserPage(this));
}

void MessageBrowser::loadMessages(const QList<Message> &messages) {
  Skin skin = qApp->skins()->currentSkin();
  QString messages_layout;
  QString single_message_layout = skin.m_layoutMarkup;

  foreach (const Message &message, messages) {
    QString enclosures;

    foreach (const Enclosure &enclosure, message.m_enclosures) {
      enclosures += skin.m_enclosureMarkup.arg(enclosure.m_url);

      if (!enclosure.m_mimeType.isEmpty()) {
        enclosures += QL1S(" [") + enclosure.m_mimeType + QL1S("]");
      }

      enclosures += QL1S("<br />");
    }

    if (!enclosures.isEmpty()) {
      enclosures = enclosures.prepend(QSL("<br />"));
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
                                                     message.m_isImportant ? "mark-unstarred" : "mark-starred"));
  }

  QString layout_wrapper = skin.m_layoutMarkupWrapper.arg(messages.size() == 1 ? messages.at(0).m_title : tr("Newspaper view"),
                                                          messages_layout);

  IOFactory::writeTextFile("aaa.html", layout_wrapper.toUtf8());

  setHtml(layout_wrapper, QUrl(INTERNAL_URL_MESSAGE));
}

void MessageBrowser::loadMessage(const Message &message) {
  loadMessages(QList<Message>() << message);
}
