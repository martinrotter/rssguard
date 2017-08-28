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

#include "network-web/webpage.h"

#include "definitions/definitions.h"
#include "gui/webviewer.h"

#include <QStringList>
#include <QString>


WebPage::WebPage(QObject* parent) : QWebEnginePage(parent) {
	setBackgroundColor(Qt::transparent);
}

WebViewer* WebPage::view() const {
	return qobject_cast<WebViewer*>(QWebEnginePage::view());
}

void WebPage::javaScriptAlert(const QUrl& securityOrigin, const QString& msg) {
	QStringList parts = msg.split(QL1C('-'));

	if (parts.size() == 2) {
		int message_id = parts.at(0).toInt();
		QString action = parts.at(1);

		if (action == QSL("read")) {
			emit messageStatusChangeRequested(message_id, MarkRead);
		}
		else if (action == QSL("unread")) {
			emit messageStatusChangeRequested(message_id, MarkUnread);
		}
		else if (action == QSL("starred")) {
			emit messageStatusChangeRequested(message_id, MarkStarred);
		}
		else if (action == QSL("unstarred")) {
			emit messageStatusChangeRequested(message_id, MarkUnstarred);
		}
		else {
			QWebEnginePage::javaScriptAlert(securityOrigin, msg);
		}
	}
	else {
		QWebEnginePage::javaScriptAlert(securityOrigin, msg);
	}
}

bool WebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame) {
	if (url.host() == INTERNAL_URL_MESSAGE_HOST) {
		setHtml(view()->messageContents(), QUrl(INTERNAL_URL_MESSAGE));
		return true;
	}
	else {
		return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
	}
}
