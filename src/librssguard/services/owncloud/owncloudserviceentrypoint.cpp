// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/owncloud/owncloudserviceentrypoint.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/owncloud/definitions.h"
#include "services/owncloud/gui/formeditowncloudaccount.h"
#include "services/owncloud/owncloudserviceroot.h"

ServiceRoot* OwnCloudServiceEntryPoint::createNewRoot() const {
  FormEditOwnCloudAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<OwnCloudServiceRoot>();
}

QList<ServiceRoot*> OwnCloudServiceEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->driver()->connection(QSL("OwnCloudServiceEntryPoint"));

  return DatabaseQueries::getAccounts<OwnCloudServiceRoot>(database, code());
}

QString OwnCloudServiceEntryPoint::name() const {
  return QSL("Nextcloud News");
}

QString OwnCloudServiceEntryPoint::code() const {
  return QSL(SERVICE_CODE_OWNCLOUD);
}

QString OwnCloudServiceEntryPoint::description() const {
  return QObject::tr("The News app is an RSS/Atom feed aggregator. "
                     "It is part of Nextcloud suite. This plugin implements %1 API.").arg(QSL(OWNCLOUD_API_VERSION));
}

QString OwnCloudServiceEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon OwnCloudServiceEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("nextcloud"));
}
