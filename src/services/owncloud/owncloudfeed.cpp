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

#include "owncloudfeed.h"

#include "miscellaneous/iconfactory.h"
#include "services/owncloud/owncloudserviceroot.h"
#include "services/owncloud/network/owncloudnetworkfactory.h"


OwnCloudFeed::OwnCloudFeed(RootItem *parent) : Feed(parent) {
}

OwnCloudFeed::OwnCloudFeed(const QSqlRecord &record) : Feed(NULL) {
  setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  setId(record.value(FDS_DB_ID_INDEX).toInt());
  setIcon(qApp->icons()->fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  setAutoUpdateType(static_cast<Feed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
  setCustomId(record.value(FDS_DB_CUSTOM_ID_INDEX).toInt());
}

OwnCloudFeed::~OwnCloudFeed() {
}

OwnCloudServiceRoot *OwnCloudFeed::serviceRoot() const {
  return qobject_cast<OwnCloudServiceRoot*>(getParentServiceRoot());
}

int OwnCloudFeed::update() {
  OwnCloudGetMessagesResponse headlines = serviceRoot()->network()->getMessages(customId());

  if (serviceRoot()->network()->lastError() != QNetworkReply::NoError) {
    setStatus(Feed::Error);
    serviceRoot()->itemChanged(QList<RootItem*>() << this);
    return 0;
  }
  else {
    return 0;
    // TODO: TADY POKRACOVAT
    // Udělat změnu tuto v tabulkách které mají sloupec custom_id
    // Udělat to tak, že custom_id se bude vyplňovat pro všechny
    // položky v Feeds, Categories a Messages
    // taky tu property budou mít všechny příslušné objekty
    // u standardních Feeds, Categories a Message se custom_id == id
    //
    // toto pak umožní přesunout všechny metody, které budou s custom ID a ID
    // pracovat, do třídy předka a ušetřit kód.
    // - toto sql provede překopirovani hodnot z atributu id do custom_id, pokud
    // je custom_id prazdne, což plati pro standardní učet
    // bude potřeba překopirovat u zprav, kategorii a feedů
    /*
     *UPDATE Categories
SET custom_id = (SELECT id FROM Categories t WHERE t.id = Categories.id)
WHERE Categories.custom_id IS NULL;
     *
     *     //return updateMessages(headlines.messages());
  }
}

int OwnCloudFeed::messageForeignKeyId() const {
  return customId();
}
