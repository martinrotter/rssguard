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

#include "network-web/adblock/requestinterceptor.h"

#include "network-web/adblock/adblockmanager.h"
#include "miscellaneous/application.h"


RequestInterceptor::RequestInterceptor(QObject *parent) : QWebEngineUrlRequestInterceptor(parent) {
}

RequestInterceptor::~RequestInterceptor() {
}

void RequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info) {
  if (AdBlockManager::instance()->shouldBlock(info.requestUrl(), info.firstPartyUrl().toString(), info.resourceType())) {
    info.block(true);
    qWarning("%s just blocked network resource:\n URL: '%s'.", APP_NAME, qPrintable(info.requestUrl().toString()));

    /*
    qApp->showGuiMessage(tr("Network resource blocked"),
                         tr("%1 just blocked network resource:\n URL: %2").arg(APP_NAME, info.requestUrl().toString()),
                         QSystemTrayIcon::Information);
    */
  }
}
