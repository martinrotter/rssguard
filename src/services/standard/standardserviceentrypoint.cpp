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


#include "services/standard/standardserviceentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "services/standard/standardserviceroot.h"

#include <QSqlQuery>


StandardServiceEntryPoint::StandardServiceEntryPoint() {
}

StandardServiceEntryPoint::~StandardServiceEntryPoint() {
}

bool StandardServiceEntryPoint::isSingleInstanceService() const {
  return true;
}

QString StandardServiceEntryPoint::name() const {
  return QSL("Standard online feeds (RSS/RDF/ATOM)");
}

QString StandardServiceEntryPoint::description() const {
  return QSL("This service offers integration with standard online RSS/RDF/ATOM feeds and podcasts.");
}

QString StandardServiceEntryPoint::version() const {
  return APP_VERSION;
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

ServiceRoot *StandardServiceEntryPoint::createNewRoot() const {
  // Switch DB.
  QSqlDatabase database = qApp->database()->connection(QSL("StandardServiceEntryPoint"), DatabaseFactory::FromSettings);
  QSqlQuery query(database);

  // First obtain the ID, which can be assigned to this new account.
  if (!query.exec("SELECT max(id) FROM Accounts;") || !query.next()) {
    return NULL;
  }

  int id_to_assign = query.value(0).toInt() + 1;

  query.prepare(QSL("INSERT INTO Accounts (id, type) VALUES (:id, :type);"));
  query.bindValue(QSL(":id"), id_to_assign);
  query.bindValue(QSL(":type"), SERVICE_CODE_STD_RSS);

  if (query.exec()) {
    StandardServiceRoot *root = new StandardServiceRoot();
    root->setId(NO_PARENT_CATEGORY);
    root->setAccountId(id_to_assign);
    return root;
  }
  else {
    return NULL;
  }
}

QList<ServiceRoot*> StandardServiceEntryPoint::initializeSubtree() const {
  // Check DB if standard account is enabled.
  QSqlDatabase database = qApp->database()->connection(QSL("StandardServiceEntryPoint"), DatabaseFactory::FromSettings);
  QSqlQuery query(database);
  QList<ServiceRoot*> roots;

  query.setForwardOnly(true);
  query.prepare(QSL("SELECT id FROM Accounts WHERE type = :type;"));
  query.bindValue(QSL(":type"), SERVICE_CODE_STD_RSS);

  if (query.exec()) {
    while (query.next()) {
      StandardServiceRoot *root = new StandardServiceRoot();
      root->setId(NO_PARENT_CATEGORY);
      root->setAccountId(query.value(0).toInt());
      roots.append(root);
    }
  }

  return roots;
}
