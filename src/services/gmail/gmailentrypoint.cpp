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

#include "services/gmail/gmailentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailserviceroot.h"
#include "services/gmail/gui/formeditgmailaccount.h"

ServiceRoot* GmailEntryPoint::createNewRoot() const {
  FormEditGmailAccount form_acc(qApp->mainFormWidget());

  return form_acc.execForCreate();
}

QList<ServiceRoot*> GmailEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->connection(QSL("GmailEntryPoint"), DatabaseFactory::FromSettings);

  return DatabaseQueries::getGmailAccounts(database);
}

bool GmailEntryPoint::isSingleInstanceService() const {
  return false;
}

QString GmailEntryPoint::name() const {
  return QSL("Gmail (not yet implemented)");
}

QString GmailEntryPoint::code() const {
  return SERVICE_CODE_GMAIL;
}

QString GmailEntryPoint::description() const {
  return QObject::tr("Simple Gmail integration via JSON API. Allows sending e-mails too.");
}

QString GmailEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon GmailEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("gmail"));
}
