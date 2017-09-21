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

#include "services/inoreader/network/inoreadernetworkfactory.h"

InoreaderNetworkFactory::InoreaderNetworkFactory(QObject* parent) : QObject(parent) {}

/*
   QOAuth2AuthorizationCodeFlow* oauth2 = new QOAuth2AuthorizationCodeFlow("1000000604",
                                                                          QUrl("https://www.inoreader.com/oauth2/auth"),
                                                                          QUrl("https://www.inoreader.com/oauth2/token"),
                                                                          new SilentNetworkAccessManager(),
                                                                          this);
   auto replyHandler = new QOAuthHttpServerReplyHandler(8080, this);

   replyHandler->setCallbackPath("");

   oauth2->setReplyHandler(replyHandler);
   oauth2->setClientIdentifierSharedKey("gsStoZ3aAoQJCgQxoFSuXkWI7Sly87yK");
   oauth2->setContentType(QAbstractOAuth::ContentType::Json);
   oauth2->setScope("read write");

   connect(oauth2, &QOAuth2AuthorizationCodeFlow::statusChanged, [=](
            QAbstractOAuth::Status status) {
    if (status == QAbstractOAuth::Status::Granted) {
      int a = 5;
    }
   });

   oauth2->setModifyParametersFunction([&](QAbstractOAuth::Stage stage, QVariantMap* parameters) {
    if (stage == QAbstractOAuth::Stage::RequestingAuthorization) {
      int b = 6;
    }
   });
   connect(oauth2, &QOAuth2AuthorizationCodeFlow::granted, [ = ] {
    int c = 45;

    auto* reply = oauth2->get(QUrl("https://www.inoreader.com/reader/api/0/subscription/list"));

    connect(reply, &QNetworkReply::finished, [=]() {
      const auto json = reply->readAll();
      const auto document = QJsonDocument::fromJson(json);
    });
   });
   connect(oauth2, &QOAuth2AuthorizationCodeFlow::error, [](const QString& error, const QString& errorDescription, const QUrl& uri) {
    int d = 5;
   });
   connect(oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);
   oauth2->grant();
 */
