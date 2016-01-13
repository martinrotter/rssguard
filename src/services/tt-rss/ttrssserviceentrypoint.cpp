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
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "gui/dialogs/formmain.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/gui/formeditaccount.h"
#include "services/tt-rss/ttrssserviceroot.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"

#include <QPointer>
#include <QSqlQuery>
#include <QSqlError>


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
  return APP_VERSION;
}

QString TtRssServiceEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon TtRssServiceEntryPoint::icon() const {
  return qApp->icons()->fromTheme(QSL("application-ttrss"));
}

QString TtRssServiceEntryPoint::code() const {
  return SERVICE_CODE_TT_RSS;
}

ServiceRoot *TtRssServiceEntryPoint::createNewRoot() {
  QPointer<FormEditAccount> form_acc = new FormEditAccount(qApp->mainForm());
  TtRssServiceRoot *new_root = form_acc.data()->execForCreate();
  delete form_acc.data();

  return new_root;
}

QList<ServiceRoot*> TtRssServiceEntryPoint::initializeSubtree() const {
  // Check DB if standard account is enabled.
  QSqlDatabase database = qApp->database()->connection(QSL("TtRssServiceEntryPoint"), DatabaseFactory::FromSettings);
  QSqlQuery query(database);
  QList<ServiceRoot*> roots;

  if (query.exec("SELECT * FROM TtRssAccounts;")) {
    while (query.next()) {
      TtRssServiceRoot *root = new TtRssServiceRoot();
      root->setId(query.value(0).toInt());
      root->setAccountId(query.value(0).toInt());
      root->network()->setUsername(query.value(1).toString());
      root->network()->setPassword(TextFactory::decrypt(query.value(2).toString()));
      root->network()->setAuthIsUsed(query.value(3).toBool());
      root->network()->setAuthUsername(query.value(4).toString());
      root->network()->setAuthPassword(TextFactory::decrypt(query.value(5).toString()));
      root->network()->setUrl(query.value(6).toString());
      root->network()->setForceServerSideUpdate(query.value(7).toBool());

      root->updateTitle();
      roots.append(root);
    }
  }
  else {
    qWarning("TT-RSS: Getting list of activated accounts failed: '%s'.", qPrintable(query.lastError().text()));
  }

  return roots;
}
