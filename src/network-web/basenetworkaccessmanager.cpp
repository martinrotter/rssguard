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

#include "network-web/basenetworkaccessmanager.h"

#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"

#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>

BaseNetworkAccessManager::BaseNetworkAccessManager(QObject* parent)
  : QNetworkAccessManager(parent) {
  connect(this, &BaseNetworkAccessManager::sslErrors, this, &BaseNetworkAccessManager::onSslErrors);
  loadSettings();
}

BaseNetworkAccessManager::~BaseNetworkAccessManager() {}

void BaseNetworkAccessManager::loadSettings() {
  QNetworkProxy new_proxy;
  const QNetworkProxy::ProxyType selected_proxy_type = static_cast<QNetworkProxy::ProxyType>(qApp->settings()->value(GROUP(Proxy),
                                                                                                                     SETTING(Proxy::Type)).
                                                                                             toInt());

  if (selected_proxy_type == QNetworkProxy::NoProxy) {
    // No extra setting is needed, set new proxy and exit this method.
    setProxy(QNetworkProxy::NoProxy);
  }
  else if (selected_proxy_type == QNetworkProxy::DefaultProxy) {
    setProxy(QNetworkProxy::applicationProxy());
  }
  else {
    const Settings* settings = qApp->settings();

    // Custom proxy is selected, set it up.
    new_proxy.setType(selected_proxy_type);
    new_proxy.setHostName(settings->value(GROUP(Proxy), SETTING(Proxy::Host)).toString());
    new_proxy.setPort(settings->value(GROUP(Proxy), SETTING(Proxy::Port)).toInt());
    new_proxy.setUser(settings->value(GROUP(Proxy), SETTING(Proxy::Username)).toString());
    new_proxy.setPassword(TextFactory::decrypt(settings->value(GROUP(Proxy), SETTING(Proxy::Password)).toString()));
    setProxy(new_proxy);
  }

  qDebug("Settings of BaseNetworkAccessManager loaded.");
}

void BaseNetworkAccessManager::onSslErrors(QNetworkReply* reply, const QList<QSslError>& error) {
  qWarning("Ignoring SSL errors for '%s': '%s' (code %d).", qPrintable(reply->url().toString()), qPrintable(reply->errorString()),
           (int) reply->error());
  reply->ignoreSslErrors(error);
}

QNetworkReply* BaseNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                       const QNetworkRequest& request,
                                                       QIODevice* outgoingData) {
  QNetworkRequest new_request = request;

  // This rapidly speeds up loading of web sites.
  // NOTE: https://en.wikipedia.org/wiki/HTTP_pipelining
  new_request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

  // Setup custom user-agent.
  new_request.setRawHeader(USER_AGENT_HTTP_HEADER, QString(APP_USERAGENT).toLocal8Bit());
  return QNetworkAccessManager::createRequest(op, new_request, outgoingData);
}
