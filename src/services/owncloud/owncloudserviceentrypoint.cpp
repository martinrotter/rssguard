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

#include "services/owncloud/owncloudserviceentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/owncloud/definitions.h"
#include "services/owncloud/owncloudserviceroot.h"
#include "services/owncloud/network/owncloudnetworkfactory.h"
#include "services/owncloud/gui/formeditowncloudaccount.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/textfactory.h"

#include <QSqlQuery>
#include <QSqlError>


OwnCloudServiceEntryPoint::OwnCloudServiceEntryPoint() {
}

OwnCloudServiceEntryPoint::~OwnCloudServiceEntryPoint() {
}

ServiceRoot *OwnCloudServiceEntryPoint::createNewRoot() const {
  QScopedPointer<FormEditOwnCloudAccount> form_acc(new FormEditOwnCloudAccount(qApp->mainForm()));
  return form_acc->execForCreate();
}

QList<ServiceRoot*> OwnCloudServiceEntryPoint::initializeSubtree() const {
  // Check DB if standard account is enabled.
  QSqlDatabase database = qApp->database()->connection(QSL("OwnCloudServiceEntryPoint"), DatabaseFactory::FromSettings);
  QSqlQuery query(database);
  QList<ServiceRoot*> roots;

  if (query.exec("SELECT * FROM OwnCloudAccounts;")) {
    while (query.next()) {
      OwnCloudServiceRoot *root = new OwnCloudServiceRoot();
      root->setId(query.value(0).toInt());
      root->setAccountId(query.value(0).toInt());
      root->network()->setAuthUsername(query.value(1).toString());
      root->network()->setAuthPassword(TextFactory::decrypt(query.value(2).toString()));
      root->network()->setUrl(query.value(3).toString());
      root->network()->setForceServerSideUpdate(query.value(4).toBool());

      root->updateTitle();
      roots.append(root);
    }
  }
  else {
    qWarning("OwnCloud: Getting list of activated accounts failed: '%s'.", qPrintable(query.lastError().text()));
  }

  return roots;
}

bool OwnCloudServiceEntryPoint::isSingleInstanceService() const {
  return false;
}

QString OwnCloudServiceEntryPoint::name() const {
  return QSL("ownCloud News");
}

QString OwnCloudServiceEntryPoint::code() const {
  return SERVICE_CODE_OWNCLOUD;
}

QString OwnCloudServiceEntryPoint::description() const {
  return QObject::tr("The News app is an RSS/Atom feed aggregator. It is part of ownCloud suite. This plugin implements %1 API.").arg(API_VERSION);
}

QString OwnCloudServiceEntryPoint::version() const {
  return APP_VERSION;
}

QString OwnCloudServiceEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon OwnCloudServiceEntryPoint::icon() const {
  return qApp->icons()->fromTheme(QSL("application-owncloud"));
}

