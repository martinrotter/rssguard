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

#include "network-web/webpage.h"

#include "network-web/webbrowsernetworkaccessmanager.h"
#include "network-web/webbrowser.h"

#include <QNetworkReply>
#include <QWebFrame>


WebPage::WebPage(QObject *parent)
  : QWebPage(parent) {
  // Setup global network access manager.
  // NOTE: This makes network settings easy for all web browsers.
  setNetworkAccessManager(WebBrowserNetworkAccessManager::instance());
}

WebPage::~WebPage() {
}

QString WebPage::toPlainText() const {
  return mainFrame()->toPlainText();
}

QString WebPage::toHtml() const {
  return mainFrame()->toHtml();
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame,
                                      const QNetworkRequest &request,
                                      QWebPage::NavigationType type) {
  if (type == QWebPage::NavigationTypeLinkClicked &&
      frame == mainFrame()) {
    // Make sure that appropriate signal is emitted even if
    // no delegation is enabled.
    emit linkClicked(request.url());
  }

  qDebug("Accepting request '%s'.", qPrintable(request.url().toString()));

  return QWebPage::acceptNavigationRequest(frame, request, type);
}
