// This file is part of RSS Guard.

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

#include "services/inoreader/inoreaderserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "services/inoreader/inoreaderentrypoint.h"
#include "services/inoreader/network/inoreadernetworkfactory.h"

InoreaderServiceRoot::InoreaderServiceRoot(RootItem* parent) : ServiceRoot(parent) {}

InoreaderServiceRoot::~InoreaderServiceRoot() {}

void InoreaderServiceRoot::updateTitle() {
  setTitle(m_network->username() + QSL(" (Inoreader)"));
}

void InoreaderServiceRoot::saveAccountDataToDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (accountId() != NO_PARENT_CATEGORY) {
    if (DatabaseQueries::overwriteInoreaderAccount(database, m_network->username(), m_network->accessToken(),
                                                   m_network->refreshToken(), m_network->batchSize(),
                                                   accountId())) {
      updateTitle();
      itemChanged(QList<RootItem*>() << this);
    }
  }
  else {
    bool saved;
    int id_to_assign = DatabaseQueries::createAccount(database, code(), &saved);

    if (saved) {
      if (DatabaseQueries::createInoreaderAccount(database, id_to_assign,
                                                  m_network->username(), m_network->accessToken(),
                                                  m_network->refreshToken(), m_network->batchSize())) {
        setId(id_to_assign);
        setAccountId(id_to_assign);
        updateTitle();
      }
    }
  }
}

bool InoreaderServiceRoot::supportsFeedAdding() const {
  return true;
}

bool InoreaderServiceRoot::supportsCategoryAdding() const {
  return false;
}

void InoreaderServiceRoot::start(bool freshly_activated) {}

void InoreaderServiceRoot::stop() {}

QString InoreaderServiceRoot::code() const {
  return InoreaderEntryPoint().code();
}

void InoreaderServiceRoot::addNewFeed(const QString& url) {}

void InoreaderServiceRoot::addNewCategory() {}
