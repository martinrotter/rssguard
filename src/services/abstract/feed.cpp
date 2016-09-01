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

#include "services/abstract/feed.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/databasequeries.h"
#include "services/abstract/recyclebin.h"
#include "services/abstract/serviceroot.h"

#include <QThread>


Feed::Feed(RootItem *parent)
  : RootItem(parent), m_url(QString()), m_status(Normal), m_autoUpdateType(DefaultAutoUpdate),
    m_autoUpdateInitialInterval(DEFAULT_AUTO_UPDATE_INTERVAL), m_autoUpdateRemainingInterval(DEFAULT_AUTO_UPDATE_INTERVAL),
    m_totalCount(0), m_unreadCount(0) {
  setKind(RootItemKind::Feed);
  setAutoDelete(false);
}

Feed::~Feed() {
}

QList<Message> Feed::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  return DatabaseQueries::getUndeletedMessagesForFeed(database, customId(), getParentServiceRoot()->accountId());
}

QVariant Feed::data(int column, int role) const {
  switch (role) {
    case Qt::ForegroundRole:
      switch (status()) {
        case NewMessages:
          return QColor(Qt::blue);
          
        case Error:
        case ParsingError:
        case OtherError:
          return QColor(Qt::red);
          
        default:
          return QVariant();
      }
      
    default:
      return RootItem::data(column, role);
  }
}

int Feed::autoUpdateInitialInterval() const {
  return m_autoUpdateInitialInterval;
}

int Feed::countOfAllMessages() const {
  return m_totalCount;
}

int Feed::countOfUnreadMessages() const {
  return m_unreadCount;
}

void Feed::setCountOfAllMessages(int count_all_messages) {
  m_totalCount = count_all_messages;
}

void Feed::setCountOfUnreadMessages(int count_unread_messages) {
  if (status() == NewMessages && count_unread_messages < countOfUnreadMessages()) {
    setStatus(Normal);
  }
  
  m_unreadCount = count_unread_messages;
}

void Feed::setAutoUpdateInitialInterval(int auto_update_interval) {
  // If new initial auto-update interval is set, then
  // we should reset time that remains to the next auto-update.
  m_autoUpdateInitialInterval = auto_update_interval;
  m_autoUpdateRemainingInterval = auto_update_interval;
}

Feed::AutoUpdateType Feed::autoUpdateType() const {
  return m_autoUpdateType;
}

void Feed::setAutoUpdateType(Feed::AutoUpdateType auto_update_type) {
  m_autoUpdateType = auto_update_type;
}

int Feed::autoUpdateRemainingInterval() const {
  return m_autoUpdateRemainingInterval;
}

void Feed::setAutoUpdateRemainingInterval(int auto_update_remaining_interval) {
  m_autoUpdateRemainingInterval = auto_update_remaining_interval;
}

Feed::Status Feed::status() const {
  return m_status;
}

void Feed::setStatus(const Feed::Status &status) {
  m_status = status;
}

QString Feed::url() const {
  return m_url;
}

void Feed::setUrl(const QString &url) {
  m_url = url;
}

void Feed::updateCounts(bool including_total_count) {
  bool is_main_thread = QThread::currentThread() == qApp->thread();
  QSqlDatabase database = is_main_thread ?
                            qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings) :
                            qApp->database()->connection(QSL("feed_upd"), DatabaseFactory::FromSettings);
  int account_id = getParentServiceRoot()->accountId();
  
  if (including_total_count) {
    setCountOfAllMessages(DatabaseQueries::getMessageCountsForFeed(database, customId(), account_id, true));
  }
  
  setCountOfUnreadMessages(DatabaseQueries::getMessageCountsForFeed(database, customId(), account_id, false));
}

void Feed::run() {
  qDebug().nospace() << "Downloading new messages for feed "
                     << customId() << " in thread: \'"
                     << QThread::currentThreadId() << "\'.";
  
  QList<Message> msgs = obtainNewMessages();
  emit messagesObtained(msgs);
}

int Feed::updateMessages(const QList<Message> &messages) {
  bool is_main_thread = QThread::currentThread() == qApp->thread();

  qDebug("Updating messages in DB. Main thread: '%s'.", qPrintable(is_main_thread ? "true." : "false."));
  
  int custom_id = customId();
  int account_id = getParentServiceRoot()->accountId();
  bool anything_updated = false;
  bool ok;
  QSqlDatabase database = is_main_thread ?
                            qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings) :
                            qApp->database()->connection(QSL("feed_upd"), DatabaseFactory::FromSettings);
  int updated_messages = DatabaseQueries::updateMessages(database, messages, custom_id, account_id, url(), &anything_updated, &ok);
  
  if (ok) {
    if (updated_messages > 0) {
      setStatus(NewMessages);
    }
    else {
      setStatus(Normal);
    }
    
    QList<RootItem*> items_to_update;
    
    updateCounts(true);
    items_to_update.append(this);
    
    if (getParentServiceRoot()->recycleBin() != nullptr && anything_updated) {
      getParentServiceRoot()->recycleBin()->updateCounts(true);
      items_to_update.append(getParentServiceRoot()->recycleBin());
    }
    
    getParentServiceRoot()->itemChanged(items_to_update);
  }
  
  return updated_messages;
}
