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

#include "services/inoreader/inoreaderfeed.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/inoreader/inoreaderserviceroot.h"

InoreaderFeed::InoreaderFeed(RootItem* parent) : Feed(parent) {}

InoreaderFeed::InoreaderFeed(const QSqlRecord& record) : InoreaderFeed(nullptr) {
  setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  setId(record.value(FDS_DB_ID_INDEX).toInt());
  setCustomId(record.value(FDS_DB_CUSTOM_ID_INDEX).toString());
  setIcon(qApp->icons()->fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  setAutoUpdateType(static_cast<Feed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
  qDebug("Custom ID of Inoreader feed when loading from DB is '%s'.", qPrintable(customId()));
}

InoreaderServiceRoot* InoreaderFeed::serviceRoot() const {
  return qobject_cast<InoreaderServiceRoot*>(getParentServiceRoot());
}

QList<Message> InoreaderFeed::obtainNewMessages(bool* error_during_obtaining) {
  return QList<Message>();
}
