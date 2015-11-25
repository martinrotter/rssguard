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

#include "network-web/webbrowsernetworkaccessmanager.h"

#include "miscellaneous/application.h"

#include "network-web/adblock/adblockmanager.h"

#include <QNetworkReply>


QPointer<WebBrowserNetworkAccessManager> WebBrowserNetworkAccessManager::s_instance;

WebBrowserNetworkAccessManager::WebBrowserNetworkAccessManager(WebPage *page, QObject *parent)
  : BaseNetworkAccessManager(parent), m_page(page) {
  connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          this, SLOT(onAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
}

WebBrowserNetworkAccessManager::~WebBrowserNetworkAccessManager() {
  qDebug("Destroying WebBrowserNetworkAccessManager instance.");
}

void WebBrowserNetworkAccessManager::onAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator) {
  Q_UNUSED(authenticator);

  // FIXME: Support authentication for web pages.
  qDebug("URL '%s' requested authentication but username/password is not available.", qPrintable(reply->url().toString()));
}

QNetworkReply *WebBrowserNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *outgoingData) {
  if (m_page != NULL) {
    QNetworkRequest pageRequest = request;
    m_page->populateNetworkRequest(pageRequest);
    return WebBrowserNetworkAccessManager::instance()->createRequest(op, pageRequest, outgoingData);
  }

  if (op == QNetworkAccessManager::GetOperation) {
    QNetworkReply *reply = AdBlockManager::instance()->block(request);

    if (reply != NULL) {
      return reply;
    }
  }

  return BaseNetworkAccessManager::createRequest(op, request, outgoingData);
}

WebBrowserNetworkAccessManager *WebBrowserNetworkAccessManager::instance() {
  if (s_instance.isNull()) {
    s_instance = new WebBrowserNetworkAccessManager(0, qApp);
  }

  return s_instance;
}
