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

#include "services/owncloud/owncloudcategory.h"

#include "services/owncloud/owncloudserviceroot.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"


OwnCloudCategory::OwnCloudCategory(RootItem *parent) : Category(parent) {
  // Categories in ownCloud have now icons etc. They just have titles.
  setIcon(qApp->icons()->fromTheme(QSL("folder")));
}

OwnCloudCategory::OwnCloudCategory(const QSqlRecord &record) : Category(nullptr) {
  setIcon(qApp->icons()->fromTheme(QSL("folder")));
  setId(record.value(CAT_DB_ID_INDEX).toInt());
  setTitle(record.value(CAT_DB_TITLE_INDEX).toString());
  setCustomId(record.value(CAT_DB_CUSTOM_ID_INDEX).toInt());
}

OwnCloudServiceRoot *OwnCloudCategory::serviceRoot() const {
  return qobject_cast<OwnCloudServiceRoot*>(getParentServiceRoot());
}

bool OwnCloudCategory::markAsReadUnread(RootItem::ReadStatus status) {
  serviceRoot()->addMessageStatesToCache(getParentServiceRoot()->customIDSOfMessagesForItem(this), status);
  return serviceRoot()->markFeedsReadUnread(getSubTreeFeeds(), status);
}

OwnCloudCategory::~OwnCloudCategory() {
}
