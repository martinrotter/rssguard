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

#include "services/standard/standardserviceentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "services/standard/standardserviceroot.h"

StandardServiceEntryPoint::StandardServiceEntryPoint() {}

StandardServiceEntryPoint::~StandardServiceEntryPoint() {}

bool StandardServiceEntryPoint::isSingleInstanceService() const {
  return true;
}

QString StandardServiceEntryPoint::name() const {
  return QObject::tr("Standard online feeds (RSS/RDF/ATOM)");
}

QString StandardServiceEntryPoint::description() const {
  return QObject::tr("This service offers integration with standard online RSS/RDF/ATOM feeds and podcasts.");
}

QString StandardServiceEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon StandardServiceEntryPoint::icon() const {
  return QIcon(APP_ICON_PATH);
}

QString StandardServiceEntryPoint::code() const {
  return SERVICE_CODE_STD_RSS;
}

ServiceRoot* StandardServiceEntryPoint::createNewRoot() const {
  // Switch DB.
  QSqlDatabase database = qApp->database()->connection(QSL("StandardServiceEntryPoint"), DatabaseFactory::FromSettings);
  bool ok;
  int new_id = DatabaseQueries::createAccount(database, code(), &ok);

  if (ok) {
    StandardServiceRoot* root = new StandardServiceRoot();

    root->setAccountId(new_id);
    return root;
  }
  else {
    return nullptr;
  }
}

QList<ServiceRoot*> StandardServiceEntryPoint::initializeSubtree() const {
  // Check DB if standard account is enabled.
  QSqlDatabase database = qApp->database()->connection(QSL("StandardServiceEntryPoint"), DatabaseFactory::FromSettings);

  return DatabaseQueries::getAccounts(database);
}
