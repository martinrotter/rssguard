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

#include "miscellaneous/feedreader.h"

#include "services/standard/standardserviceentrypoint.h"
#include "services/owncloud/owncloudserviceentrypoint.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"

#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "core/feeddownloader.h"


FeedReader::FeedReader(QObject *parent) : QObject(parent), m_feedServices(QList<ServiceEntryPoint*>()) {
  m_feedDownloader = new FeedDownloader(this);
  m_feedsModel = new FeedsModel(this);
  m_feedProxyModel = new FeedsProxyModel(m_feedsModel, this);
  m_messagesModel = new MessagesModel(this);
  m_messagesProxyModel = new MessagesProxyModel(m_messagesModel, this);
}

FeedReader::~FeedReader() {
  qDebug("Destroying FeedReader instance.");
  qDeleteAll(m_feedServices);
}

QList<ServiceEntryPoint*> FeedReader::feedServices() {
  if (m_feedServices.isEmpty()) {
    // NOTE: All installed services create their entry points here.
    m_feedServices.append(new StandardServiceEntryPoint());
    m_feedServices.append(new TtRssServiceEntryPoint());
    m_feedServices.append(new OwnCloudServiceEntryPoint());
  }

  return m_feedServices;
}

FeedDownloader *FeedReader::feedDownloader() const {
  return m_feedDownloader;
}

FeedsModel *FeedReader::feedsModel() const {
  return m_feedsModel;
}

MessagesModel *FeedReader::messagesModel() const {
  return m_messagesModel;
}

void FeedReader::start() {
}

void FeedReader::stop() {
}

MessagesProxyModel *FeedReader::messagesProxyModel() const {
  return m_messagesProxyModel;
}

FeedsProxyModel *FeedReader::feedProxyModel() const {
  return m_feedProxyModel;
}
