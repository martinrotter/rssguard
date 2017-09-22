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

#include "services/inoreader/inoreaderentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "services/inoreader/definitions.h"
#include "services/inoreader/gui/formeditinoreaderaccount.h"
#include "services/inoreader/inoreaderserviceroot.h"
#include "services/inoreader/network/inoreadernetworkfactory.h"

ServiceRoot* InoreaderEntryPoint::createNewRoot() const {
  FormEditInoreaderAccount form_acc(qApp->mainFormWidget());

  return form_acc.execForCreate();
}

QList<ServiceRoot*> InoreaderEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->connection(QSL("InoreaderEntryPoint"), DatabaseFactory::FromSettings);

  return DatabaseQueries::getInoreaderAccounts(database);
}

bool InoreaderEntryPoint::isSingleInstanceService() const {
  return true;
}

QString InoreaderEntryPoint::name() const {
  return QSL("Inoreader");
}

QString InoreaderEntryPoint::code() const {
  return SERVICE_CODE_INOREADER;
}

QString InoreaderEntryPoint::description() const {
  return QObject::tr("This is integration of Inoreader.");
}

QString InoreaderEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon InoreaderEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("inoreader"));
}
