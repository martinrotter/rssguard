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

#include "network-web/silentnetworkaccessmanager.h"

#include "miscellaneous/application.h"

#include <QNetworkReply>
#include <QAuthenticator>


QPointer<SilentNetworkAccessManager> SilentNetworkAccessManager::s_instance;

SilentNetworkAccessManager::SilentNetworkAccessManager(QObject *parent)
  : BaseNetworkAccessManager(parent) {
  connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          this, SLOT(onAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
}

SilentNetworkAccessManager::~SilentNetworkAccessManager() {
  qDebug("Destroying SilentNetworkAccessManager instance.");
}

SilentNetworkAccessManager *SilentNetworkAccessManager::instance() {
  if (s_instance.isNull()) {
    s_instance = new SilentNetworkAccessManager(qApp);
  }

  return s_instance;
}

void SilentNetworkAccessManager::onAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator) {
  QList<QString> keys = authenticator->options().keys();

  if (reply->property("protected").toBool()) {
    // This feed contains authentication information, it is good.
    authenticator->setUser(reply->property("username").toString());
    authenticator->setPassword(reply->property("password").toString());
    reply->setProperty("authentication-given", true);

    qDebug("Item '%s' requested authentication and got it.", qPrintable(reply->url().toString()));
  }
  else {
    reply->setProperty("authentication-given", false);

    // Authentication is required but this feed does not contain it.
    qWarning("Item '%s' requested authentication but username/password is not available.", qPrintable(reply->url().toString()));
  }
}
