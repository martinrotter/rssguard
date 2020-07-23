// For license of this file, see <project-root-folder>/LICENSE.md.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
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

#include "network-web/rssguardschemehandler.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/skinfactory.h"

#include <QBuffer>
#include <QUrlQuery>
#include <QWebEngineUrlRequestJob>

RssGuardSchemeHandler::RssGuardSchemeHandler(QObject* parent) : QWebEngineUrlSchemeHandler(parent) {}

RssGuardSchemeHandler::~RssGuardSchemeHandler() = default;

void RssGuardSchemeHandler::requestStarted(QWebEngineUrlRequestJob* job) {
  // Decide which data we want.
  QByteArray data = targetData(job->requestUrl());

  if (data.isEmpty()) {
    job->fail(QWebEngineUrlRequestJob::UrlNotFound);
  }
  else {
    auto* buf = new QBuffer(job);

    buf->setData(data);
    job->reply(QByteArray("text/html"), buf);
  }
}

QByteArray RssGuardSchemeHandler::targetData(const QUrl& url) {
  const QString& url_string = url.toString();

  if (url_string.contains(QSL(ADBLOCK_ADBLOCKED_PAGE))) {
    QUrlQuery query(url);
    const QString& subscription = query.queryItemValue(QSL("subscription"));
    const QString& rule = query.queryItemValue(QSL("rule"));

    return qApp->skins()->adBlockedPage(subscription, rule).toUtf8();
  }
  else {
    return QByteArray();
  }
}
