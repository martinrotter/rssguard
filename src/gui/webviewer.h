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

#ifndef WEBVIEWER_H
#define WEBVIEWER_H

#include <QWebEngineView>

#include "core/message.h"
#include "network-web/webpage.h"


class WebViewer : public QWebEngineView {
    Q_OBJECT

  public:
    explicit WebViewer(QWidget* parent = 0);

    bool canIncreaseZoom();
    bool canDecreaseZoom();

    inline QString messageContents() {
      return m_messageContents;
    }

  public slots:
    // Page zoom modifiers.
    bool increaseWebPageZoom();
    bool decreaseWebPageZoom();
    bool resetWebPageZoom();

    void displayMessage();
    void loadMessages(const QList<Message> &messages);
    void loadMessage(const Message &message);
    void clear();

  protected:
    QWebEngineView *createWindow(QWebEnginePage::WebWindowType type);
    void wheelEvent(QWheelEvent *event);

  signals:
    void messageStatusChangeRequested(int message_id, WebPage::MessageStatusChange change);

  private:
    QString m_messageContents;
};

#endif // WEBVIEWER_H
