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

#include "services/tt-rss/ttrssserviceentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/databasequeries.h"
#include "gui/dialogs/formmain.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssserviceroot.h"
#include "services/tt-rss/gui/formeditaccount.h"

#include <QPointer>


TtRssServiceEntryPoint::TtRssServiceEntryPoint(){
}


TtRssServiceEntryPoint::~TtRssServiceEntryPoint() {
}

bool TtRssServiceEntryPoint::isSingleInstanceService() const {
  return false;
}

QString TtRssServiceEntryPoint::name() const {
  return QSL("Tiny Tiny RSS");
}

QString TtRssServiceEntryPoint::description() const {
  return QObject::tr("This service offers integration with Tiny Tiny RSS.\n\n"
                     "Tiny Tiny RSS is an open source web-based news feed (RSS/Atom) reader and aggregator, "
                     "designed to allow you to read news from any location, while feeling as close to a real "
                     "desktop application as possible.\n\nAt least API level %1 is required.").arg(MINIMAL_API_LEVEL);
}

QString TtRssServiceEntryPoint::version() const {
  return STRFY(APP_VERSION);
}

QString TtRssServiceEntryPoint::author() const {
  return STRFY(APP_AUTHOR);
}

QIcon TtRssServiceEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("tt-rss"));
}

QString TtRssServiceEntryPoint::code() const {
  return SERVICE_CODE_TT_RSS;
}

ServiceRoot *TtRssServiceEntryPoint::createNewRoot() const {
  QScopedPointer<FormEditAccount> form_acc(new FormEditAccount(qApp->mainForm()));
  return form_acc->execForCreate();
}

QList<ServiceRoot*> TtRssServiceEntryPoint::initializeSubtree() const {
  // Check DB if standard account is enabled.
  QSqlDatabase database = qApp->database()->connection(QSL("TtRssServiceEntryPoint"), DatabaseFactory::FromSettings);

  return DatabaseQueries::getTtRssAccounts(database);
}
