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

#include "services/tt-rss/ttrssfeed.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/gui/formttrssfeeddetails.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrsscategory.h"
#include "services/tt-rss/ttrssserviceroot.h"

#include <QPointer>

TtRssFeed::TtRssFeed(RootItem* parent)
  : Feed(parent) {}

TtRssFeed::TtRssFeed(const QSqlRecord& record) : Feed(nullptr) {
  setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  setId(record.value(FDS_DB_ID_INDEX).toInt());
  setIcon(qApp->icons()->fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  setAutoUpdateType(static_cast<Feed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
  setCustomId(record.value(FDS_DB_CUSTOM_ID_INDEX).toInt());
  qDebug("Custom ID of TT-RSS feed when loading from DB is '%s'.", qPrintable(record.value(FDS_DB_CUSTOM_ID_INDEX).toString()));
}

TtRssFeed::~TtRssFeed() {}

TtRssServiceRoot* TtRssFeed::serviceRoot() const {
  return qobject_cast<TtRssServiceRoot*>(getParentServiceRoot());
}

bool TtRssFeed::canBeEdited() const {
  return true;
}

bool TtRssFeed::editViaGui() {
  QPointer<FormTtRssFeedDetails> form_pointer = new FormTtRssFeedDetails(serviceRoot(), qApp->mainFormWidget());
  form_pointer.data()->addEditFeed(this, nullptr);
  delete form_pointer.data();
  return false;
}

bool TtRssFeed::canBeDeleted() const {
  return true;
}

bool TtRssFeed::deleteViaGui() {
  TtRssUnsubscribeFeedResponse response = serviceRoot()->network()->unsubscribeFeed(customId());

  if (response.code() == UFF_OK && removeItself()) {
    serviceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    qWarning("TT-RSS: Unsubscribing from feed failed, received JSON: '%s'", qPrintable(response.toString()));
    return false;
  }
}

bool TtRssFeed::markAsReadUnread(RootItem::ReadStatus status) {
  serviceRoot()->addMessageStatesToCache(getParentServiceRoot()->customIDSOfMessagesForItem(this), status);
  return getParentServiceRoot()->markFeedsReadUnread(QList<Feed*>() << this, status);
}

bool TtRssFeed::cleanMessages(bool clear_only_read) {
  return getParentServiceRoot()->cleanFeeds(QList<Feed*>() << this, clear_only_read);
}

bool TtRssFeed::editItself(TtRssFeed* new_feed_data) {
  QSqlDatabase database = qApp->database()->connection("aa", DatabaseFactory::FromSettings);

  if (DatabaseQueries::editBaseFeed(database, id(), new_feed_data->autoUpdateType(),
                                    new_feed_data->autoUpdateInitialInterval())) {
    setAutoUpdateType(new_feed_data->autoUpdateType());
    setAutoUpdateInitialInterval(new_feed_data->autoUpdateInitialInterval());
    return true;
  }
  else {
    return false;
  }
}

QList<Message> TtRssFeed::obtainNewMessages(bool* error_during_obtaining) {
  QList<Message> messages;
  int newly_added_messages = 0;
  int limit = MAX_MESSAGES;
  int skip = 0;

  do {
    TtRssGetHeadlinesResponse headlines = serviceRoot()->network()->getHeadlines(customId(), limit, skip,
                                                                                 true, true, false);

    if (serviceRoot()->network()->lastError() != QNetworkReply::NoError) {
      setStatus(Feed::NetworkError);
      *error_during_obtaining = true;
      serviceRoot()->itemChanged(QList<RootItem*>() << this);
      return QList<Message>();
    }
    else {
      QList<Message> new_messages = headlines.messages();
      messages.append(new_messages);
      newly_added_messages = new_messages.size();
      skip += newly_added_messages;
    }
  }
  while (newly_added_messages > 0);

  *error_during_obtaining = false;
  return messages;
}

bool TtRssFeed::removeItself() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  return DatabaseQueries::deleteFeed(database, customId(), serviceRoot()->accountId());
}
